/*
 * hello.c -- a trusted, benign program.
 *
 * After e9patch instrumentation (-> hello.mtd) all of its syscalls are
 * KEY-shifted by the syscall_mtd hook, and the mtd_tracer translates them
 * back, so it runs exactly as normal. This is the "legitimate" baseline.
 */
#include <stdio.h>

int main(void)
{
    printf("Hello, world\n");
    return 0;
}
