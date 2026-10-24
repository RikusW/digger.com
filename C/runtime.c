/*
 * C runtime helpers used by the original - reconstructed from the binary.
 *
 *   __exit        CS:0x0172 (file 0x0072)  stack reset + far return to DOS
 *   parse_cmdline CS:0x017B (file 0x007B)  fills the argv array at DS:0x0004
 *   iabs          CS:0x0227 (file 0x0127)
 *   digit_value   CS:0x80F5 (file 0x7FF5) '1'..'9' -> 1..9, else 0
 *   toupper_c     CS:0x8155 (file 0x8055)
 *   lmul          CS:0x8BFC (file 0x8AFC) signed 32x32 -> low 32
 *   exit_process  CS:0x8C6D (file 0x8B6D)
 *   ldivmod       CS:0x8C7D (file 0x8B7D) signed 32/32, quotient + remainder
 *   ultoa         CS:0x8D4C (file 0x8C4C) unsigned long -> decimal string
 *   lcmp          CS:0x8E23 (file 0x8D23) signed 32-bit compare
 *
 * The original ABI passes long values in AX:BX (high:low) and the divisor
 * in CX:DX.  The C signatures below use fixed-width types and a remainder
 * pointer; the exact register mapping is noted per function.
 */

#include "symbols.h"

extern u8 dsseg[0x10000];

/* CS:0x0172 (file 0x0072) - the C startup's exit trampoline: restore SP
 * from DS:0x0000 and far-return to DOS.  Not expressible as a normal C
 * call; documented here. */
void __exit(void)
{
    /* sp = *(u16 *)0; __far_return(); */
}

/* CS:0x017B (file 0x007B) - parse the command tail into DS:0x0004. */
/* Signature is compiler-specific; see ANALYSIS.md. */

/* CS:0x0227 (file 0x0127) */
int iabs(int v)
{
    return v < 0 ? -v : v;
}

/* CS:0x80F5 (file 0x7FF5) */
int digit_value(int c)
{
    if (c >= '1' && c <= '9')
        return c - '0';
    return 0;
}

/* CS:0x8155 (file 0x8055) */
int toupper_c(int c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

/* CS:0x8BFC (file 0x8AFC) - signed multiply; AX:BX * CX:DX, result AX:BX
 * (the high 32 bits are discarded). */
s32 lmul(s32 a, s32 b)
{
    return a * b;
}

/* CS:0x8C7D (file 0x8B7D) - signed divide; AX:BX / CX:DX, quotient AX:BX,
 * remainder DX (and the high word in CX for the 32-bit path). */
s32 ldivmod(s32 a, s32 b, s32 *rem)
{
    *rem = (b != 0) ? (a % b) : 0;
    return (b != 0) ? (a / b) : 0;
}

/* CS:0x8E23 (file 0x8D23) - signed 32-bit compare, returning -1/0/1. */
int lcmp(s32 a, s32 b)
{
    if (a < b)
        return -1;
    if (a > b)
        return 1;
    return 0;
}

/* CS:0x8C6D (file 0x8B6D) - exit(status); pushes the status and tails into
 * the CS:0x0172 trampoline. */
void exit_process(int status)
{
    (void)status;
    __exit();
}

/* CS:0x8D4C (file 0x8C4C) - unsigned long to decimal with a field width.
 * The original writes at DS:[bp+0x18] and uses [bp+0x1C] as the width. */
void ultoa(u32 value, int buf, int width)
{
    char tmp[12];
    int n = 0, i;

    do {
        tmp[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0 && n < 11);
    while (n < width && n < 11)
        tmp[n++] = '0';
    for (i = 0; i < n; i++)
        dsseg[buf + i] = (u8)tmp[n - 1 - i];
    dsseg[buf + n] = 0;
}
