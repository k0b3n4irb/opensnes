/*
 * luna stress probe — PPU Mode 7 signed multiply accuracy.
 *
 * Distinct hardware unit from the CPU multiplier: the Mode 7 maths engine
 * multiplies M7A (signed 16-bit, $211B written low-then-high) by M7B
 * (signed 8-bit, the last byte written to $211C) and exposes a signed
 * 24-bit product at MPYL/MPYM/MPYH ($2134/$2135/$2136).
 *
 * Sign handling (both operands, and the 24-bit result) is the classic
 * divergence point. We store the RAW three product bytes per case into a
 * WRAM array so luna (`--peek`) and Mesen2 (wram read) are compared on the
 * PPU's actual output — not on any C-side arithmetic.
 *
 * Transitory stress prototype (see .claude/rules/luna_tooling.md).
 */
#include <snes.h>

#define M7A  (*(volatile u8 *)0x211B)
#define M7B  (*(volatile u8 *)0x211C)
#define MPYL (*(volatile u8 *)0x2134)
#define MPYM (*(volatile u8 *)0x2135)
#define MPYH (*(volatile u8 *)0x2136)

/** @brief 3 raw product bytes per case; 8 cases -> 24 bytes + marker. */
u8 res[28];

static void settle(void)
{
    volatile u8 k;
    for (k = 0; k < 8; k++) { }
}

/** @brief Program M7A (16-bit) x M7B (8-bit) and store raw MPYL/M/H at res+off. */
static void m7_mul(s16 a, s8 b, u8 off)
{
    M7A = (u8)((u16)a & 0xff);      /* low  byte of M7A */
    M7A = (u8)(((u16)a >> 8) & 0xff);/* high byte of M7A */
    M7B = (u8)b;                    /* 8-bit signed operand */
    settle();
    res[off]     = MPYL;
    res[off + 1] = MPYM;
    res[off + 2] = MPYH;
}

int main(void)
{
    consoleInit();

    m7_mul(1000, 5, 0);          /* +5000     = 0x001388 */
    m7_mul(-1000, 5, 3);         /* -5000     = 0xFFEC78 */
    m7_mul(100, -3, 6);          /* -300      = 0xFFFED4 */
    m7_mul(-100, -3, 9);         /* +300      = 0x00012C */
    m7_mul(32767, 127, 12);      /* +4161409  = 0x3F7F81 */
    m7_mul(-32768, -128, 15);    /* +4194304  = 0x400000 */
    m7_mul(0, -1, 18);           /* 0         = 0x000000 */
    m7_mul(-1, -1, 21);          /* +1        = 0x000001 */

    res[24] = 0xDE;              /* marker */
    res[25] = 0xC0;
    res[26] = 0x00;
    res[27] = 0x00;

    while (1) { }
    return 0;
}
