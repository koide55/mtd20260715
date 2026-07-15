/*
 * victim.c -- a "trusted" program whose system calls go through ITS OWN
 * `syscall` instructions (inline asm), so e9patch instruments them and the
 * in-process MTD gate can monitor them.
 *
 * Why inline syscalls?  Under Docker Desktop's Rosetta 2 (Apple Silicon),
 * instrumenting the many syscalls in a *statically linked* glibc's early
 * start-up crashes the Rosetta-translated trampolines. A *dynamically* linked
 * program keeps libc's syscalls in libc.so (not instrumented), so we issue the
 * syscalls we want to monitor from the program's own code via inline `syscall`.
 * On a native x86_64 host you would instead instrument libc.so (or a static
 * binary) so that ordinary libc calls are covered; see README "real deployment".
 *
 *   ./victim              -> benign  (inline write; allowed by the gate)
 *   ./victim pwn          -> spawns /bin/sh via inline execve (attack succeeds)
 *   ./victim.mtd pwn      -> the execve is blocked at the MTD syscall gate
 *
 * Built dynamically (default), so glibc start-up syscalls stay in libc.so.
 */
#include <string.h>

#define SYS_write   1
#define SYS_execve 59

/* Issue a syscall through the program's OWN `syscall` instruction. */
static long sys3(long n, long a, long b, long c)
{
    long r;
    asm volatile ("syscall"
                  : "=a"(r)
                  : "a"(n), "D"(a), "S"(b), "d"(c)
                  : "rcx", "r11", "memory");
    return r;
}

int main(int argc, char **argv)
{
    const char *msg = "victim: doing benign work (inline write syscall)\n";
    sys3(SYS_write, 1, (long)msg, (long)strlen(msg));   /* instrumented, allowed */

    if (argc > 1 && strcmp(argv[1], "pwn") == 0) {
        /* Simulated payload: spawn a shell. In a real attack this point is
         * reached by hijacked control flow funnelling into the syscall gate. */
        static char *a[] = { "/bin/sh", "-c",
                             "echo '### SHELL OBTAINED (uid='$(id -u)')'", 0 };
        static char *e[] = { 0 };
        sys3(SYS_execve, (long)a[0], (long)a, (long)e);
        const char *f = "execve failed\n";
        sys3(SYS_write, 2, (long)f, (long)strlen(f));
        return 1;
    }
    return 0;
}
