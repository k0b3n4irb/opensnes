/**
 * @file main.c
 * @brief Hello World - OpenSNES Font Test
 *
 * Displays "HELLO WORLD" using the OpenSNES font.
 * Tests the text rendering system.
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

/* Font data - first few characters for testing */
/* 2bpp format: 16 bytes per tile */
/* Characters: space (32), ! (33), ... A-Z (65-90) */
static const u8 font_data[] = {
    /* Space (32) - blank */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* Skip to 'A' at index 33 (33 * 16 = 528 bytes from start) */
    /* For testing, let's include A-Z directly */

    /* We'll use a simplified approach - just store the letters we need */
};

/* Minimal font - just uppercase letters A-Z for HELLO WORLD */
/* Each character is 8 bytes (1bpp), we'll expand to 2bpp inline */
static const u8 font_1bpp[] = {
    /* D (index 0) */
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00,
    /* E (index 1) */
    0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00,
    /* H (index 2) */
    0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
    /* L (index 3) */
    0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00,
    /* O (index 4) */
    0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00,
    /* R (index 5) */
    0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00,
    /* W (index 6) */
    0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00,
    /* ! (index 7) */
    0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00,
    /* Space (index 8) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Map characters to font indices */
static u8 char_to_tile(char c) {
    switch (c) {
        case 'D': return 0;
        case 'E': return 1;
        case 'H': return 2;
        case 'L': return 3;
        case 'O': return 4;
        case 'R': return 5;
        case 'W': return 6;
        case '!': return 7;
        default:  return 8;  /* Space */
    }
}

/* Load font tiles to VRAM (convert 1bpp to 2bpp on the fly) */
static void load_font(void) {
    u16 i, j;

    /* Set VRAM address to $0000 (word address) */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Write 9 characters (9 * 8 bytes 1bpp = 9 * 16 bytes 2bpp) */
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 8; j++) {
            /* 2bpp: bitplane 0 (the 1bpp data), bitplane 1 (zeros) */
            REG_VMDATAL = font_1bpp[i * 8 + j];  /* Bitplane 0 */
            REG_VMDATAH = 0x00;                   /* Bitplane 1 */
        }
    }
}

/* Write a string to the tilemap */
static void print_at(u8 x, u8 y, const char *str) {
    u16 addr;

    /* Calculate tilemap address (32 tiles per row) */
    /* Tilemap at $3000 = word address $1800 */
    addr = 0x1800 + (y * 32) + x;

    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;

    while (*str) {
        u8 tile = char_to_tile(*str);
        REG_VMDATAL = tile;  /* Tile number */
        REG_VMDATAH = 0x00;  /* Attributes: palette 0, no flip, priority 0 */
        str++;
    }
}

/* Set palette colors */
static void set_palette(void) {
    /* Color 0: Dark blue (background) */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;   /* R=0, G=0 */
    REG_CGDATA = 0x28;   /* B=10 */

    /* Color 1: White (text) */
    REG_CGDATA = 0xFF;   /* R=31, G=7 (low bits) */
    REG_CGDATA = 0x7F;   /* G=24 (high bits), B=31 */
}

int main(void) {
    /* We start in forced blank from crt0 */

    /* Set up Mode 1 with BG1 */
    REG_BGMODE = 0x01;    /* Mode 1, 8x8 tiles */

    /* BG1 tilemap at $3000 (word addr), 32x32 */
    REG_BG1SC = 0x30;     /* $3000 >> 8, 32x32 tiles */

    /* BG1 tiles at $0000 */
    REG_BG12NBA = 0x00;   /* BG1 tiles at $0000, BG2 at $0000 */

    /* Load font to VRAM */
    load_font();

    /* Set up palette */
    set_palette();

    /* Clear tilemap (fill with space tile) */
    {
        u16 i;
        REG_VMAIN = 0x80;
        REG_VMADDL = 0x00;
        REG_VMADDH = 0x18;  /* $1800 = tilemap */
        for (i = 0; i < 1024; i++) {
            REG_VMDATAL = 8;     /* Space tile */
            REG_VMDATAH = 0x00;
        }
    }

    /* Print "HELLO WORLD!" */
    print_at(10, 14, "HELLO WORLD!");

    /* Enable BG1 on main screen */
    REG_TM = 0x01;

    /* Turn on screen at full brightness */
    REG_INIDISP = 0x0F;

    /* Infinite loop */
    while (1) {
        /* Nothing to do */
    }

    return 0;
}
