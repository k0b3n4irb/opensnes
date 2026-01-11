/**
 * @file main.c
 * @brief Minimal OpenSNES Example - Red Screen
 *
 * Displays a solid red screen. The simplest possible visual output.
 *
 * License: CC0 (Public Domain)
 */

typedef unsigned char u8;

/* SNES hardware registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)

int main(void) {
    /* Set palette color 0 to red */
    /* SNES color format: 0bbbbbgg gggrrrrr */
    REG_CGADD = 0;          /* Palette index 0 */
    REG_CGDATA = 0x1F;      /* Red low byte (r=31, g=0) */
    REG_CGDATA = 0x00;      /* Red high byte (b=0) */

    /* Turn on screen at full brightness */
    REG_INIDISP = 0x0F;

    /* Infinite loop */
    while (1) {
    }

    return 0;
}
