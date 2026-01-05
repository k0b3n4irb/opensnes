/**
 * @file main_simple.c
 * @brief Simple Hello World - Works with new cc65816 compiler
 *
 * Minimal example showing hardware register writes.
 * Avoids for loops and string pointers (need more compiler work).
 *
 * License: CC0 (Public Domain)
 */

typedef unsigned char u8;
typedef unsigned short u16;

/* Hardware registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_BGMODE   (*(volatile u8*)0x2105)
#define REG_BG1SC    (*(volatile u8*)0x2107)
#define REG_BG12NBA  (*(volatile u8*)0x210B)
#define REG_VMAIN    (*(volatile u8*)0x2115)
#define REG_VMADDL   (*(volatile u8*)0x2116)
#define REG_VMADDH   (*(volatile u8*)0x2117)
#define REG_VMDATAL  (*(volatile u8*)0x2118)
#define REG_VMDATAH  (*(volatile u8*)0x2119)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_TM       (*(volatile u8*)0x212C)

/* Set palette colors */
void set_palette(void) {
    /* Color 0: Dark blue (background) */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x28;

    /* Color 1: White (text) */
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;
}

/* Write a simple "HI" message */
void write_message(void) {
    u16 addr;

    /* Calculate tilemap address for center of screen */
    addr = 0x0800 + (14 * 32) + 14;

    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;

    /* Write 'H' (tile 0) */
    REG_VMDATAL = 0;
    REG_VMDATAH = 0;

    /* Write 'I' (tile 1) */
    REG_VMDATAL = 1;
    REG_VMDATAH = 0;
}

/* Load simple 2 character font (H and I) */
void load_font(void) {
    /* Set VRAM address to $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Tile 0: 'H' character (8x8 2bpp) */
    REG_VMDATAL = 0x66; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x66; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x66; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x7E; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x66; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x66; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x66; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;

    /* Tile 1: 'I' character (8x8 2bpp) */
    REG_VMDATAL = 0x7E; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x18; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x18; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x18; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x18; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x18; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x7E; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;

    /* Tile 2: Space (blank) */
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0x00; REG_VMDATAH = 0x00;
}

int main(void) {
    /* Set up Mode 0 */
    REG_BGMODE = 0x00;

    /* BG1 tilemap at $0800 */
    REG_BG1SC = 0x08;

    /* BG1 tiles at $0000 */
    REG_BG12NBA = 0x00;

    /* Load font */
    load_font();

    /* Set palette */
    set_palette();

    /* Write message */
    write_message();

    /* Enable BG1 */
    REG_TM = 0x01;

    /* Turn on screen */
    REG_INIDISP = 0x0F;

    /* Infinite loop */
    while (1) {
    }

    return 0;
}
