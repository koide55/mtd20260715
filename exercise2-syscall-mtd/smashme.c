/*
 * smashme.c -- classic stack buffer overflow (from the original exercise).
 *
 * gets() has no bounds checking, so an input longer than 64 bytes overflows
 * `input` and overwrites the saved return address. The accompanying exploit.py
 * redirects control to shellcode via a `jmp rsp` gadget.
 *
 * Build (deliberately vulnerable, executable stack, no ASLR-friendly PIE):
 *   gcc -fno-stack-protector -z execstack -no-pie -std=c99 -static \
 *       smashme.c -o smashme
 *
 * NOTE: This is the *advanced / authentic* demo. A working exploit depends on
 * gadget addresses and a disabled ASLR, so it is environment-specific. For a
 * deterministic demonstration of the detection mechanism, use `evil` instead.
 */
#include <stdio.h>

int main(void)
{
    char input[64];
    printf("Welcome to the Dr. Phil Show. Wanna smash?\n");
    fflush(stdout);
    gets(input);            /* overflow at >= 0x40 (64) bytes */
    printf("You said: %s\n", input);
    return 0;
}
