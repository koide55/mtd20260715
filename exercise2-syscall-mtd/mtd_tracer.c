/*
 * mtd_tracer.c  --  ptrace supervisor for System-call-number MTD.
 *
 * Companion to the e9patch hook `syscall_mtd.c`. It:
 *   1. picks a per-run random KEY and exports it as MTD_KEY;
 *   2. fork()s and exec()s the (e9patch-instrumented) target under ptrace;
 *   3. at every syscall entry, inspects the syscall number in orig_rax:
 *        - shifted number (>= KEY): a trusted, instrumented syscall.
 *          Subtract KEY to recover the real number, then let it run.
 *        - un-shifted number (< KEY): code that the MTD system did NOT
 *          instrument (loader startup, or INJECTED SHELLCODE).
 *          A sensitive syscall (execve/execveat) here is treated as an
 *          intrusion and BLOCKED; benign startup syscalls are allowed.
 *
 * This is a teaching implementation: clear over clever. It is x86_64-only,
 * matching e9patch. Build with `make`; run as:  ./mtd_tracer ./hello.mtd
 */

#if !defined(__x86_64__)
#error "mtd_tracer requires x86_64 (e9patch only rewrites x86_64 ELF binaries)."
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/random.h>

#define MAX_SYS      512        /* highest plausible real syscall number      */
#define SYS_execve    59        /* x86_64 syscall numbers                     */
#define SYS_execveat 322

static const char *syscall_name(long n)
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

static int is_sensitive(long n)
{
    return (n == SYS_execve || n == SYS_execveat);
}

/* Pick a per-run KEY: aligned, larger than any real syscall number, and small
 * enough that KEY + MAX_SYS never overflows. Range: [0x10000, 0x7fff0000]. */
static long make_key(void)
{
    unsigned int r = 0;
    if (getrandom(&r, sizeof(r), 0) != (ssize_t)sizeof(r))
        r = (unsigned int)(getpid() * 2654435761u);
    return (long)(((r & 0x7fffu) + 1u) << 16);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }

    long key = make_key();
    char keybuf[32];
    snprintf(keybuf, sizeof(keybuf), "%ld", key);
    setenv("MTD_KEY", keybuf, 1);
    fprintf(stderr, "[mtd] MTD_KEY = 0x%lx (per-run random)\n", key);

    pid_t child = fork();
    if (child < 0) { perror("fork"); return 1; }

    if (child == 0) {
        /* Child: request tracing, then become the target. */
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) { perror("PTRACE_TRACEME"); _exit(127); }
        execvp(argv[1], &argv[1]);
        perror("execvp");
        _exit(127);
    }

    /* Parent (supervisor). */
    int status;
    if (waitpid(child, &status, 0) < 0) { perror("waitpid"); return 1; }
    /* Report syscall stops as SIGTRAP|0x80 and kill child if we die. */
    ptrace(PTRACE_SETOPTIONS, child, 0,
           (void *)(PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL));

    int in_syscall = 0;          /* toggles entry/exit                        */
    long translated = 0;         /* count of trusted (shifted) syscalls       */
    int  intrusion = 0;

    for (;;) {
        if (ptrace(PTRACE_SYSCALL, child, 0, 0) < 0) break;
        if (waitpid(child, &status, 0) < 0) break;

        if (WIFEXITED(status)) {
            fprintf(stderr, "[mtd] %ld syscalls translated, 0 intrusions detected\n",
                    translated);
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "[mtd] target killed by signal %d\n", WTERMSIG(status));
            return 1;
        }
        if (!WIFSTOPPED(status)) continue;

        int sig = WSTOPSIG(status);
        if (sig != (SIGTRAP | 0x80)) {
            /* Not a syscall stop (real signal): forward it to the child. */
            ptrace(PTRACE_SYSCALL, child, 0, (void *)(long)sig);
            waitpid(child, &status, 0);
            if (WIFEXITED(status) || WIFSIGNALED(status)) return 0;
            continue;
        }

        in_syscall = !in_syscall;
        if (!in_syscall)
            continue;            /* act only on syscall ENTRY                 */

        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, child, 0, &regs) < 0) break;
        long num = (long)regs.orig_rax;

        if (num >= key) {
            /* Trusted, instrumented syscall: undo the MTD shift. */
            regs.orig_rax = (unsigned long long)(num - key);
            ptrace(PTRACE_SETREGS, child, 0, &regs);
            translated++;
        } else if (is_sensitive(num)) {
            /* Un-shifted sensitive syscall => not from instrumented code. */
            fprintf(stderr,
                "[mtd] Invalid system call detected (raw %s, rax=%ld): INTRUSION BLOCKED\n",
                syscall_name(num), num);
            /* Neutralize the syscall so it cannot run, then stop the target. */
            regs.orig_rax = (unsigned long long)-1;
            ptrace(PTRACE_SETREGS, child, 0, &regs);
            intrusion = 1;
            kill(child, SIGKILL);
            break;
        }
        /* else: benign un-shifted syscall (loader/startup) -> allow as-is. */
    }

    if (intrusion) {
        fprintf(stderr, "[mtd] target stopped -- shell denied\n");
        return 2;
    }
    return 0;
}
