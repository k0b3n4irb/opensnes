/*
 * An integer divide or remainder of two constants is folded (2026-11-07).
 * QBE pins an integer div / rem so that gcm never moves it (it may trap),
 * and gvn skipped every pinned instruction before folding it: `19391 / 3584`
 * called __sdiv16 at run time while `7 * 32` folded to 224.
 *
 * fold16() and fold32() hold only constant divisions, signed and unsigned,
 * 16 and 32 bits, negative operands on either side, limits, and a numerator
 * that becomes constant only once the computation before it is folded
 * (`(7 * 32 + 3584 - 1) / 3584`). test_const_div.py checks that neither
 * calls a division helper and that every result equals the value written by
 * hand beside it. edges() holds the divisions the folder must refuse:
 * INT_MIN / -1 and INT_MIN % -1 at both widths (they stay run-time
 * operations, the helper's wrap is checked) and a zero divisor (the call is
 * kept, its value is not checked: C leaves it undefined).
 */
#include <snes.h>

u16 ures[8];
s16 sres[12];
u32 lres[4];
s32 slres[6];
s16 edge16[2];
s32 edge32[2];
u16 edgez;
u16 done;

void fold16(void) {
    ures[0] = 19391u / 3584u;               /* 5 */
    ures[1] = 19391u % 3584u;               /* 1471 */
    ures[2] = (7 * 32 + 3584 - 1) / 3584;   /* 3807 / 3584 = 1 */
    ures[3] = 65535u / 3u;                  /* 21845 (0 if read signed) */
    ures[4] = 65535u % 256u;                /* 255 */
    ures[5] = 40000u / 7u;                  /* 5714 */
    ures[6] = 40000u % 7u;                  /* 2 */
    ures[7] = 19391u % 8u;                  /* 7 */
    sres[0] = -7 / 2;                       /* -3 */
    sres[1] = -7 % 2;                       /* -1 */
    sres[2] = 7 / -2;                       /* -3 */
    sres[3] = 7 % -2;                       /* 1 */
    sres[4] = -7 / -2;                      /* 3 */
    sres[5] = -7 % -2;                      /* -1 */
    sres[6] = -19391 / 3584;                /* -5 */
    sres[7] = -19391 % 3584;                /* -1471 */
    sres[8] = (-32767 - 1) / 2;             /* -16384 */
    sres[9] = (-32767 - 1) % 3;             /* -2 */
    sres[10] = 32767 / -1;                  /* -32767 */
    sres[11] = (-32767 - 1) / 1;            /* -32768 */
}

void fold32(void) {
    lres[0] = 100000UL / 7UL;               /* 14285 */
    lres[1] = 100000UL % 7UL;               /* 5 */
    lres[2] = 0xFFFFFFFFUL / 3UL;           /* 0x55555555 */
    lres[3] = 0xFFFFFFFFUL % 10UL;          /* 5 */
    slres[0] = -100000L / 7L;               /* -14285 */
    slres[1] = -100000L % 7L;               /* -5 */
    slres[2] = 100000L / -7L;               /* -14285 */
    slres[3] = 2147483647L / -1L;           /* -2147483647 */
    slres[4] = (-2147483647L - 1L) / 2L;    /* -1073741824 */
    slres[5] = (-2147483647L - 1L) % 7L;    /* -2 */
}

void edges(void) {
    edge16[0] = (s16)((-32767 - 1) / -1);   /* run time: wraps to -32768 */
    edge16[1] = (-32767 - 1) % -1;          /* run time: 0 */
    edge32[0] = (s32)((-2147483647L - 1L) / -1L); /* run time: wraps to INT32_MIN */
    edge32[1] = (-2147483647L - 1L) % -1L;  /* run time: 0 */
    edgez = 1u / 0u;                        /* run time, value undefined */
}

int main(void) {
    fold16();
    fold32();
    edges();
    done = 0xBEEF;
    while (1) {
    }
    return 0;
}
