/**
 * @file main.c
 * @brief Hello World - OpenSNES Library Example
 *
 * Demonstrates using the OpenSNES library for basic console functions.
 * Uses:
 * - consoleInit() for hardware initialization
 * - setMode() for video mode
 * - setScreenOn() to enable display
 * - WaitForVBlank() for frame synchronization
 *
 * License: CC0 (Public Domain)
 */

#include <snes.h>

/*
 * Font tiles (2bpp format, 16 bytes per tile)
 * Each row is 2 bytes: bitplane0, bitplane1
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

/* Message: HELLO WORLD! as tile indices */
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

    /* Initialize console hardware using library */
    consoleInit();

    /* Set Mode 0 using library (4 BG layers, all 2bpp) */
    setMode(BGMODE_MODE0);

    /* Configure BG1 for text display */
    REG_BG1SC = 0x04;   /* Tilemap at $0400, 32x32 */
    REG_BG12NBA = 0x00; /* BG1 tiles at $0000 */

    /* Load font tiles to VRAM $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    for (i = 0; i < 144; i += 2) {
        REG_VMDATAL = font_tiles[i];
        REG_VMDATAH = font_tiles[i + 1];
    }

    /* Set up palette */
    REG_CGADD = 0;
    REG_CGDATA = 0x00;  /* Color 0: Dark blue */
    REG_CGDATA = 0x28;
    REG_CGDATA = 0xFF;  /* Color 1: White */
    REG_CGDATA = 0x7F;

    /* Fill tilemap with spaces */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = 0;
        REG_VMDATAH = 0;
    }

    /* Write message at row 14, column 10 */
    addr = 0x05CA;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;

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
    REG_TM = TM_BG1;

    /* Turn on screen using library */
    setScreenOn();

    /* Main loop with VBlank sync */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
