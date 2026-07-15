/*
 * syscall_mtd.c  --  e9patch v1.0.1 instrumentation for System-call-number MTD.
 *
 * This file is compiled with e9compile.sh and injected by e9tool BEFORE every
 * `syscall` instruction of a trusted (legitimate) binary:
 *
 *   $ $E9PATCH/e9compile.sh syscall_mtd.c
 *   $ $E9PATCH/e9tool -M 'asm=/syscall/' \
 *         -P 'before entry(state)@syscall_mtd' hello -o hello.mtd
 *
 * At run time the hook adds a per-run random KEY (supplied by the mtd_tracer
 * ptrace supervisor via the MTD_KEY environment variable) to the syscall
 * number in %rax. The trusted binary's syscalls therefore leave with a
 * "shifted" (randomized) number; the tracer subtracts KEY to recover the real
 * number before the kernel runs it.
 *
 * Injected shellcode is NOT instrumented (it appears on the stack at run time,
 * after rewriting), so its syscalls carry raw, un-shifted numbers. The tracer
 * treats an un-shifted sensitive syscall (e.g. execve) as an intrusion.
 *
 * The hook itself performs NO syscalls (getenv() only reads the environ array),
 * so it never confuses the tracer.
 *
 * API used (all provided by e9patch v1.0.1 examples/stdlib.c):
 *   - void init(int argc, char **argv, char **envp)  : one-time init
 *   - char **environ ; char *getenv(const char *)    : read MTD_KEY
 *   - long long atoll(const char *)                  : parse KEY
 *   - struct STATE { ... int64_t rax; ... }           : full register state
 */

#include "stdlib.c"

static long key = 0;

/*
 * One-time initialization, run before the target's entry point.
 * environ must be set from envp before getenv() can be used.
 */
void init(int argc, char **argv, char **envp)
{
    environ = envp;
    const char *k = getenv("MTD_KEY");
    key = (k != NULL ? (long)atoll(k) : 0);
}

/*
 * Called immediately BEFORE each `syscall` instruction of the trusted binary.
 * Shift the syscall number by the per-run KEY. Modifying state->rax updates the
 * real %rax register (per the E9Tool user guide), so the syscall leaves with a
 * randomized number that only the cooperating tracer knows how to undo.
 */
void entry(struct STATE *state)
{
    state->rax += key;
}
