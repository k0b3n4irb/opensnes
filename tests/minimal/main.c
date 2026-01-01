/**
 * @file main.c
 * @brief Minimal OpenSNES Test ROM
 *
 * The simplest possible ROM to verify the SDK works.
 * Just sets background color and loops.
 */

/* Minimal type definitions */
typedef unsigned char u8;
typedef unsigned short u16;

/* Hardware registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_NMITIMEN (*(volatile u8*)0x4200)

/**
 * @brief Main entry point
 */
int main(void) {
    /* Set palette color 0 to blue */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;   /* Low byte: 00000000 (R=0, G=0) */
    REG_CGDATA = 0x7C;   /* High byte: 0BBBBBGG (B=31, G=0) */

    /* Enable screen at full brightness */
    REG_INIDISP = 0x0F;

    /* Infinite loop */
    while (1) {
        /* Do nothing - just show blue screen */
    }

    return 0;
}
