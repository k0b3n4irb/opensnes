/**
 * @file main.c
 * @brief Hello World - OpenSNES Text Display
 *
 * Displays "HELLO WORLD!" using BG tiles.
 * Uses a simple lookup table instead of switch statements.
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

/*
 * Font tiles (2bpp format, 16 bytes per tile)
 * Each row is 2 bytes: bitplane0, bitplane1
 * We only use bitplane0 for single-color text (color 1)
 */
static const u8 font_tiles[] = {
    /* Tile 0: Space (blank) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* Tile 1: H */
    0x66, 0x00, 0x66, 0x00, 0x66, 0x00, 0x7E, 0x00,
    0x66, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00,

    /* Tile 2: E */
    0x7E, 0x00, 0x60, 0x00, 0x60, 0x00, 0x7C, 0x00,
    0x60, 0x00, 0x60, 0x00, 0x7E, 0x00, 0x00, 0x00,

    /* Tile 3: L */
    0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00,
    0x60, 0x00, 0x60, 0x00, 0x7E, 0x00, 0x00, 0x00,

    /* Tile 4: O */
    0x3C, 0x00, 0x66, 0x00, 0x66, 0x00, 0x66, 0x00,
    0x66, 0x00, 0x66, 0x00, 0x3C, 0x00, 0x00, 0x00,

    /* Tile 5: W */
    0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xD6, 0x00,
    0xFE, 0x00, 0xEE, 0x00, 0xC6, 0x00, 0x00, 0x00,

    /* Tile 6: R */
    0x7C, 0x00, 0x66, 0x00, 0x66, 0x00, 0x7C, 0x00,
    0x6C, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00,

    /* Tile 7: D */
    0x78, 0x00, 0x6C, 0x00, 0x66, 0x00, 0x66, 0x00,
    0x66, 0x00, 0x6C, 0x00, 0x78, 0x00, 0x00, 0x00,

    /* Tile 8: ! */
    0x18, 0x00, 0x18, 0x00, 0x18, 0x00, 0x18, 0x00,
    0x18, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
};

/* Message to display: HELLO WORLD! */
/* Encoded as tile indices: H=1, E=2, L=3, O=4, space=0, W=5, R=6, D=7, !=8 */
static const u8 message[] = {
    1, 2, 3, 3, 4,  /* HELLO */
    0,              /* space */
    5, 4, 6, 3, 7,  /* WORLD */
    8,              /* ! */
    0xFF            /* end marker */
};

int main(void) {
    u16 i;
    u16 addr;
    u8 tile;

    /* Set up Mode 0 (4 BG layers, all 2bpp) */
    REG_BGMODE = 0x00;

    /* BG1 tilemap at word $0400 */
    REG_BG1SC = 0x04;

    /* BG1 tiles at word $0000 */
    REG_BG12NBA = 0x00;

    /* Load font tiles to VRAM $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Write 9 tiles * 16 bytes = 144 bytes = 72 words */
    for (i = 0; i < 144; i += 2) {
        REG_VMDATAL = font_tiles[i];
        REG_VMDATAH = font_tiles[i + 1];
    }

    /* Set up palette */
    REG_CGADD = 0;
    /* Color 0: Dark blue (background) */
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x28;
    /* Color 1: White (text) */
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Fill tilemap with spaces (tile 0) */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    /* Write message at row 14, column 10 */
    /* word addr = $0400 + 14*32 + 10 = $05CA */
    addr = 0x05CA;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;

    /* Write each character */
    i = 0;
    while (1) {
        tile = message[i];
        if (tile == 0xFF) {
            break;
        }
        REG_VMDATAL = tile;
        REG_VMDATAH = 0;
        i++;
    }

    /* Enable BG1 on main screen */
    REG_TM = 0x01;

    /* Turn on screen at full brightness */
    REG_INIDISP = 0x0F;

    /* Infinite loop */
    while (1) {
    }

    return 0;
}
