/*
 * evil.c -- stands in for INJECTED SHELLCODE.
 *
 * It issues a raw execve("/bin/sh") via a direct syscall, exactly like the
 * shellcode a memory-corruption exploit would drop onto the stack. Because it
 * is NOT instrumented by the MTD system, its syscall numbers are never
 * KEY-shifted -- so under mtd_tracer the raw execve is detected and blocked.
 *
 * Run WITHOUT the tracer:  spawns a shell (attack succeeds).
 * Run WITH mtd_tracer:      "INTRUSION BLOCKED" (attack detected).
 *
 * Compiled with -static so no dynamic loader syscalls appear.
 */
#include <unistd.h>

int main(void)
{
    char *argv[] = { "/bin/sh", "-c", "echo '### SHELL OBTAINED (uid='$(id -u)')'", 0 };
    char *envp[] = { 0 };
    /* Raw syscall: exactly what injected shellcode does. */
    syscall(59 /* execve */, argv[0], argv, envp);
    write(2, "execve failed\n", 14);
    return 1;
}
