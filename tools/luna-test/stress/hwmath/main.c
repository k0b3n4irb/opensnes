/*
 * luna stress probe — SNES hardware multiply/divide unit accuracy.
 *
 * Exercises the PPU-adjacent math registers directly via MMIO and writes
 * every result into a WRAM array `results[]`, so the outcome can be read
 * byte-for-byte by BOTH luna (`state --peek results`) and Mesen2
 * (mem_read wram) and compared against the documented reference values.
 *
 *   Multiply  : WRMPYA($4202)=a, WRMPYB($4203)=b (write starts it)
 *               product16 at RDMPY ($4216/$4217), ready ~8 CPU cycles later.
 *   Divide    : WRDIV($4204/$4205)=dividend16, WRDIVB($4206)=divisor8 (starts)
 *               quotient16 at RDDIV ($4214/$4215), remainder16 at RDMPY
 *               ($4216/$4217), ready ~16 CPU cycles later.
 *   Divide-by-0: hardware yields quotient=$FFFF, remainder=dividend (fullsnes).
 *
 * This is a transitory stress prototype (see .claude/rules/luna_tooling.md).
 */
#include <snes.h>

#define WRMPYA (*(volatile u8 *)0x4202)
#define WRMPYB (*(volatile u8 *)0x4203)
#define RDMPY  (*(volatile u16 *)0x4216)   /* product (mul) / remainder (div) */
#define WRDIVL (*(volatile u8 *)0x4204)
#define WRDIVH (*(volatile u8 *)0x4205)
#define WRDIVB (*(volatile u8 *)0x4206)
#define RDDIV  (*(volatile u16 *)0x4214)   /* quotient */

/** @brief Results block — read by luna --peek and Mesen2 wram read. */
u16 results[16];

/** @brief Burn enough CPU cycles for the math unit to finish (>16). */
static void settle(void)
{
    volatile u8 k;
    for (k = 0; k < 12; k++) { }
}

/** @brief 8x8 unsigned hardware multiply. */
static u16 hw_mul(u8 a, u8 b)
{
    WRMPYA = a;
    WRMPYB = b;          /* writing B starts the multiply */
    settle();
    return RDMPY;
}

/** @brief 16/8 unsigned hardware divide -> quotient (q) and remainder (r). */
static void hw_div(u16 dividend, u8 divisor, u16 *q, u16 *r)
{
    WRDIVL = (u8)(dividend & 0xff);
    WRDIVH = (u8)(dividend >> 8);
    WRDIVB = divisor;    /* writing the divisor starts the divide */
    settle();
    *q = RDDIV;
    *r = RDMPY;
}

int main(void)
{
    u16 q, r;

    consoleInit();

    /* --- multiply cases --- */
    results[0] = hw_mul(0, 0);        /* 0            */
    results[1] = hw_mul(255, 255);    /* 65025        */
    results[2] = hw_mul(200, 200);    /* 40000        */
    results[3] = hw_mul(45, 45);      /* 2025         */

    /* --- divide cases (quotient, remainder) --- */
    hw_div(10000, 7, &q, &r);   results[4] = q;  results[5] = r;   /* 1428, 4      */
    hw_div(65535, 255, &q, &r); results[6] = q;  results[7] = r;   /* 257, 0       */
    hw_div(1000, 0, &q, &r);    results[8] = q;  results[9] = r;   /* 65535, 1000  (div by 0) */
    hw_div(0, 5, &q, &r);       results[10] = q; results[11] = r;  /* 0, 0         */
    hw_div(65535, 1, &q, &r);   results[12] = q; results[13] = r;  /* 65535, 0     */

    results[14] = 0;
    results[15] = 0xC0DE;   /* done marker */

    while (1) { }
    return 0;
}
