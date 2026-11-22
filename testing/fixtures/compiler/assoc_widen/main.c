/*
 * A 32-bit mask of a 32-bit 0 / 1 chosen by a one-bit test (2026-11-22).
 *
 *     u32 v = (a & 1) ? 1UL : 0UL;   ...   v & 63
 *
 * gvn replaces the 32-bit phi of 1 and 0 by the 16-bit `a & 1` its block
 * branches on (phicopyref), so `v & 63` became an `l` and whose operand is
 * a `w` and — legal IR on w65816 (it reads zero-extended), refused by
 * upstream QBE. assoccon(), which then merges the two constants of
 * `(a & 1) & 63`, asserted upstream's typing (`KWIDE(i2->cls) >=
 * KWIDE(i1->cls)`) and the compiler stopped (difftest_stmt --batch idiom,
 * seed 62339485). It now leaves a pair whose inner operation is narrower
 * alone: merging the constants across the widening is wrong for add, sub
 * and mul.
 *
 * Building this ROM is the first check (the old compiler stops on every
 * function below). test_assoc_widen.py then checks the values, high half
 * included, for a = 0, 1, 2, 3, 0x8001, 0xFFFF read at run time.
 */
#include <snes.h>

u16 av[6] = { 0, 1, 2, 3, 0x8001, 0xFFFF };
u32 r_and[6];    /* ((a & 1) ? 1UL : 0UL) & 63       = a & 1 */
u32 r_high[6];   /* ((a & 1) ? 1UL : 0UL) & 0x10001  = a & 1 */
u32 r_neg[6];    /* ((a & 1) ? 1UL : 0UL) & ~0UL     = a & 1 */
u32 score;       /* sum of (a & 1) over av[] = 4 */
u16 done;

__attribute__((noinline)) u32 f_and(u16 a)
{
    return ((a & 1) ? 1UL : 0UL) & 63;
}

__attribute__((noinline)) u32 f_high(u16 a)
{
    return ((a & 1) ? 1UL : 0UL) & 0x10001UL;
}

__attribute__((noinline)) u32 f_neg(u16 a)
{
    return ((a & 1) ? 1UL : 0UL) & ~0UL;
}

__attribute__((noinline)) void f_bonus(u16 pad)
{
    u32 bonus = (pad & 1) ? 1UL : 0UL;
    score += bonus & 1;
}

int main(void)
{
    u16 k;

    consoleInit();
    for (k = 0; k < 6; k++) {
        r_and[k] = f_and(av[k]);
        r_high[k] = f_high(av[k]);
        r_neg[k] = f_neg(av[k]);
        f_bonus(av[k]);
    }
    done = 0xBEEF;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
