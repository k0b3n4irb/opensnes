/**
 * @file main.c
 * @brief HiROM Mode Demo with Library Support
 *
 * Demonstrates HiROM memory mapping on the SNES using the OpenSNES library.
 *
 * HiROM vs LoROM:
 * - LoROM: 32KB banks ($8000-$FFFF per bank)
 * - HiROM: 64KB banks ($0000-$FFFF per bank)
 *
 * HiROM advantages:
 * - Larger contiguous ROM access without bank switching
 * - Better for games with large data tables or streaming
 * - Simpler address calculation for large assets
 *
 * This example displays "HIROM MODE" and responds to joypad input
 * to confirm the ROM and library are correctly configured.
 *
 * KNOWN LIMITATION (HiROM Phase 2):
 * Library input functions (padHeld, padPressed, etc.) have a return value
 * issue in HiROM mode due to compiler ABI behavior. As a workaround, this
 * demo reads input directly from the pad_keys RAM address ($002C) which
 * is populated by the NMI handler. This is functionally equivalent to
 * padHeld(0) but bypasses the function call overhead.
 */

#include <snes.h>
#include <snes/console.h>
#include <snes/dma.h>

/* Direct access to pad_keys - workaround for HiROM function return issue */
#define PAD_KEYS ((volatile u16*)0x002C)


/*============================================================================
 * Embedded Font (2bpp, 16 bytes per tile)
 * Characters: space, H, I, R, O, M, D, E, L, B, +
 *============================================================================*/

static const u8 font_tiles[] = {
    /* 0: Space */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* 1: H */
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x7E,0x00,
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 2: I */
    0x3C,0x00, 0x18,0x00, 0x18,0x00, 0x18,0x00,
    0x18,0x00, 0x18,0x00, 0x3C,0x00, 0x00,0x00,
    /* 3: R */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x6C,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 4: O */
    0x3C,0x00, 0x66,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x3C,0x00, 0x00,0x00,
    /* 5: M */
    0x63,0x00, 0x77,0x00, 0x7F,0x00, 0x6B,0x00,
    0x63,0x00, 0x63,0x00, 0x63,0x00, 0x00,0x00,
    /* 6: D */
    0x78,0x00, 0x6C,0x00, 0x66,0x00, 0x66,0x00,
    0x66,0x00, 0x6C,0x00, 0x78,0x00, 0x00,0x00,
    /* 7: E */
    0x7E,0x00, 0x60,0x00, 0x60,0x00, 0x7C,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 8: L */
    0x60,0x00, 0x60,0x00, 0x60,0x00, 0x60,0x00,
    0x60,0x00, 0x60,0x00, 0x7E,0x00, 0x00,0x00,
    /* 9: B */
    0x7C,0x00, 0x66,0x00, 0x66,0x00, 0x7C,0x00,
    0x66,0x00, 0x66,0x00, 0x7C,0x00, 0x00,0x00,
    /* 10: + (for library indicator) */
    0x00,0x00, 0x18,0x00, 0x18,0x00, 0x7E,0x00,
    0x18,0x00, 0x18,0x00, 0x00,0x00, 0x00,0x00,
};

#define FONT_SIZE (11 * 16)

/* Tile indices */
#define TILE_SPACE 0
#define TILE_H     1
#define TILE_I     2
#define TILE_R     3
#define TILE_O     4
#define TILE_M     5
#define TILE_D     6
#define TILE_E     7
#define TILE_L     8
#define TILE_B     9
#define TILE_PLUS  10

/*============================================================================
 * VRAM Configuration
 *============================================================================*/

#define TILEMAP_ADDR  0x0400   /* Word address for tilemap */
#define TILES_ADDR    0x0000   /* Word address for tiles */

/*============================================================================
 * Helper Functions
 *============================================================================*/

static void write_tile(u8 x, u8 y, u8 tile) {
    u16 addr;
    addr = TILEMAP_ADDR + y * 32 + x;
    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    REG_VMDATAL = tile;
    REG_VMDATAH = 0;
}

static void clear_tilemap(void) {
    u16 i;
    REG_VMAIN = 0x80;
    REG_VMADDL = TILEMAP_ADDR & 0xFF;
    REG_VMADDH = TILEMAP_ADDR >> 8;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = TILE_SPACE;
        REG_VMDATAH = 0;
    }
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    u16 pressed;

    /* Use library initialization */
    consoleInit();

    /* Disable screen during setup */
    setScreenOff();

    /* Set Mode 0 (4 BG layers, 2bpp each) */
    setMode(BG_MODE0, 0);

    /* Load font tiles using DMA */
    dmaCopyVram((u8*)font_tiles, TILES_ADDR, FONT_SIZE);

    /* Clear tilemap */
    clear_tilemap();

    /* Configure BG1 */
    REG_BG1SC = 0x04;    /* Tilemap at $0800, 32x32 */
    REG_BG12NBA = 0x00;  /* BG1 tiles at $0000 */
    REG_TM = 0x01;       /* Enable BG1 on main screen */

    /* Set palette - dark blue background, white text */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x28;  /* Dark blue (color 0) */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* White (color 1) */

    /* Display "HIROM MODE" centered on screen */
    /* Screen is 32 tiles wide, "HIROM MODE" is 10 chars */
    /* Center: (32 - 10) / 2 = 11 */
    write_tile(11, 12, TILE_H);
    write_tile(12, 12, TILE_I);
    write_tile(13, 12, TILE_R);
    write_tile(14, 12, TILE_O);
    write_tile(15, 12, TILE_M);
    write_tile(16, 12, TILE_SPACE);
    write_tile(17, 12, TILE_M);
    write_tile(18, 12, TILE_O);
    write_tile(19, 12, TILE_D);
    write_tile(20, 12, TILE_E);

    /* Display "+ LIB" to show library is working */
    write_tile(12, 14, TILE_PLUS);
    write_tile(14, 14, TILE_L);
    write_tile(15, 14, TILE_I);
    write_tile(16, 14, TILE_B);

    /* Enable screen */
    setScreenOn();

    /* Main loop - use library VBlank */
    while (1) {
        WaitForVBlank();

        /* Read input directly (HiROM workaround - see file header) */
        pressed = *PAD_KEYS;

        /* Change background color on A button */
        if (pressed & KEY_A) {
            /* Light blue when A is held */
            REG_CGADD = 0;
            REG_CGDATA = 0x00; REG_CGDATA = 0x7C;
        } else {
            /* Dark blue normally */
            REG_CGADD = 0;
            REG_CGDATA = 0x00; REG_CGDATA = 0x28;
        }
    }

    return 0;
}
