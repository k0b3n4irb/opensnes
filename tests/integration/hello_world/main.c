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

/* Minimal font - actual letter shapes */
/* Each character is 8 bytes (1bpp), we'll expand to 2bpp inline */
static const u8 font_1bpp[] = {
    /* D (index 0) */
    0x7C, /* .XXXXX.. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x7C, /* .XXXXX.. */
    0x00, /* ........ */

    /* E (index 1) */
    0x7E, /* .XXXXXX. */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x7C, /* .XXXXX.. */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x7E, /* .XXXXXX. */
    0x00, /* ........ */

    /* H (index 2) */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x7E, /* .XXXXXX. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x00, /* ........ */

    /* L (index 3) */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x40, /* .X...... */
    0x7E, /* .XXXXXX. */
    0x00, /* ........ */

    /* O (index 4) */
    0x3C, /* ..XXXX.. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x3C, /* ..XXXX.. */
    0x00, /* ........ */

    /* R (index 5) */
    0x7C, /* .XXXXX.. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x7C, /* .XXXXX.. */
    0x48, /* .X..X... */
    0x44, /* .X...X.. */
    0x42, /* .X....X. */
    0x00, /* ........ */

    /* W (index 6) */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x42, /* .X....X. */
    0x5A, /* .X.XX.X. */
    0x66, /* .XX..XX. */
    0x42, /* .X....X. */
    0x00, /* ........ */

    /* ! (index 7) */
    0x18, /* ...XX... */
    0x18, /* ...XX... */
    0x18, /* ...XX... */
    0x18, /* ...XX... */
    0x00, /* ........ */
    0x00, /* ........ */
    0x18, /* ...XX... */
    0x00, /* ........ */

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

/* Load font tiles to VRAM - solid white test */
static void load_font(void) {
    /* Set VRAM address to $0000 (word address) */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* All tiles: Solid white - 9 tiles, 8 rows each */
    /* If this shows checkerboard, the problem is with C code VRAM writes */
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;

    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
    REG_VMDATAL = 0xFF; REG_VMDATAH = 0x00;
}

/* Write a string to the tilemap */
static void print_at(u8 x, u8 y, const char *str) {
    u16 addr;

    /* Calculate tilemap address (32 tiles per row) */
    /* BG1SC=0x08 means tilemap at VRAM word address $0800 */
    addr = 0x0800 + (y * 32) + x;

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
    /* We start in forced blank from crt0, VRAM/CGRAM/OAM cleared */

    /* Set up Mode 0 with BG1 (all BGs are 2bpp in Mode 0) */
    REG_BGMODE = 0x00;    /* Mode 0, 8x8 tiles, all 2bpp */

    /* BG1 tilemap at $0800 (byte addr), 32x32 */
    /* BG1SC: bits 7-2 = addr/$400, bits 1-0 = size (00=32x32) */
    /* $0800 / $400 = 2, so bits 7-2 = 2, register = 2 << 2 = 0x08 */
    REG_BG1SC = 0x08;

    /* BG1 tiles at $0000 */
    REG_BG12NBA = 0x00;   /* BG1 tiles at $0000 */

    /* Load font to VRAM - DISABLED, using assembly in crt0 instead */
    /* load_font(); */

    /* Set up palette */
    set_palette();

    /* Clear tilemap (fill with space tile) */
    {
        u16 i;
        REG_VMAIN = 0x80;
        REG_VMADDL = 0x00;
        REG_VMADDH = 0x08;  /* Word addr $0800 (BG1SC=0x08 maps here) */
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
