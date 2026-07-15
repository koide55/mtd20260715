/*
 * syscall_mtd.c  --  e9patch v1.0.1 in-process System-call MTD monitor.
 *
 * Compiled with e9compile.sh and injected by e9tool BEFORE every `syscall`
 * instruction of a trusted (dynamically linked) binary:
 *
 *   $ $E9PATCH/e9compile.sh syscall_mtd.c -I $E9PATCH/examples
 *   $ $E9PATCH/e9tool -M 'asm=/syscall/' \
 *         -P 'before entry(state)@syscall_mtd' victim -o victim.mtd
 *
 * WHY THIS DESIGN (in-process, no ptrace)
 * --------------------------------------
 * The classic "system-call number randomization" MTD needs a runtime component
 * that observes the syscall numbers the process actually issues. On a native
 * x86_64 host that component can be a ptrace supervisor. But under Docker
 * Desktop's Rosetta 2 (Apple Silicon), x86_64 user code runs on an ARM64
 * kernel: Rosetta translates each x86_64 `syscall` into the equivalent ARM64
 * syscall, so a ptrace/seccomp observer at the kernel boundary never sees the
 * x86_64 syscall number (orig_rax reads as 0). Kernel-boundary syscall-number
 * MTD is therefore impossible under Rosetta.
 *
 * e9patch instrumentation, however, runs entirely in user space (Rosetta
 * translates it like any other x86_64 code). So we move the MTD enforcement
 * point INTO the process: the hook sits on every `syscall` gate of the trusted
 * binary and enforces a per-deployment syscall policy. The trusted program
 * issues the monitored syscalls through its OWN inline `syscall` instructions
 * (see victim.c), which e9patch instruments; a hijacked control flow that
 * funnels into such a gate to spawn a shell (execve) is observed and blocked.
 * (On a native x86_64 host you would instead instrument libc.so or a static
 * binary so ordinary libc syscalls are covered -- see the README.)
 *
 * The "moving target" element: the monitored/blocked syscall set is diversified
 * per deployment (configurable via the MTD_BLOCK environment variable) and can
 * be rotated over time, so an attacker cannot rely on a fixed syscall ABI being
 * permitted. By default the shell-spawning syscalls execve/execveat are blocked.
 *
 * The hook performs no syscalls on the fast path (getenv() only reads the
 * environ array), so it never re-enters itself.
 */

#include "stdlib.c"

/* Blocked ("sensitive") syscalls. Shell-spawning execve/execveat by default. */
#define MAX_BLOCK 16
static long blocked[MAX_BLOCK];
static int  n_blocked = 0;
static unsigned nonce = 0;

static const char *name_of(long n)
{
    switch (n) {
        case SYS_execve:   return "execve";
        case SYS_execveat: return "execveat";
        case 0:            return "read";
        case 1:            return "write";
        case 2:            return "open";
        case 257:          return "openat";
        default:           return "syscall";
    }
}

/* Parse a comma-separated list of syscall numbers from MTD_BLOCK, e.g. "59,322". */
static void parse_block(const char *s)
{
    while (s != NULL && *s != '\0' && n_blocked < MAX_BLOCK) {
        while (*s == ',' || *s == ' ') s++;
        if (*s == '\0') break;
        long v = 0; int any = 0;
        while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; any = 1; }
        if (any) blocked[n_blocked++] = v;
        while (*s != '\0' && *s != ',') s++;
    }
}

void init(int argc, char **argv, char **envp)
{
    environ = envp;

    const char *b = getenv("MTD_BLOCK");
    if (b != NULL && *b != '\0') {
        parse_block(b);
    } else {
        blocked[n_blocked++] = SYS_execve;      /* 59  */
        blocked[n_blocked++] = SYS_execveat;    /* 322 */
    }

    /* Per-run instance nonce -- illustrates the diversified/moving policy. */
    (void)getrandom(&nonce, sizeof(nonce), 0);

    fprintf(stderr, "[mtd] syscall gate active (instance 0x%08x); blocking:", nonce);
    for (int i = 0; i < n_blocked; i++)
        fprintf(stderr, " %s(%ld)", name_of(blocked[i]), blocked[i]);
    fprintf(stderr, "\n");
}

/*
 * Runs BEFORE each `syscall` instruction of the trusted binary. If the syscall
 * number is on the blocked list, this is an anomaly (e.g. a code-reuse payload
 * spawning a shell through the program's own syscall gate): report and stop the
 * process so the syscall never executes. Otherwise return and let it proceed.
 */
void entry(struct STATE *state)
{
    long num = state->rax;
    for (int i = 0; i < n_blocked; i++) {
        if (num == blocked[i]) {
            fprintf(stderr,
                "[mtd] INTRUSION BLOCKED: disallowed system call %s (rax=%ld) "
                "at syscall gate 0x%lx\n",
                name_of(num), num, (unsigned long)state->rip);
            exit(42);           /* terminate before the syscall runs */
        }
    }
}
