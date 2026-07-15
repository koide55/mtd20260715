/*
 * victim.c -- a "trusted" program with a shell-spawning code path.
 *
 * Normal run: does benign work (a write syscall) and exits.
 *
 * When invoked as `victim pwn`, it reaches an execve("/bin/sh") -- this stands
 * in for a successful exploit (buffer overflow / ROP) that has redirected
 * control flow to spawn a shell. Crucially, the execve goes through the
 * program's OWN syscall instruction (inside libc), which e9patch has
 * instrumented. So the in-process MTD monitor sees it.
 *
 *   ./victim              -> benign
 *   ./victim pwn          -> spawns /bin/sh   (attack succeeds)
 *   ./victim.mtd pwn      -> blocked by the MTD syscall gate (no shell)
 *
 * Built -static -no-pie so all syscall sites live in the binary (and get
 * instrumented) and there is no dynamic loader.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("victim: doing benign work (this printf is a write syscall)\n");
    fflush(stdout);

    if (argc > 1 && strcmp(argv[1], "pwn") == 0) {
        /* Simulated payload: spawn a shell via the program's own syscall gate. */
        char *a[] = { "/bin/sh", "-c",
                      "echo '### SHELL OBTAINED (uid='$(id -u)')'", 0 };
        char *e[] = { 0 };
        execve(a[0], a, e);
        perror("execve");
        return 1;
    }
    return 0;
}
