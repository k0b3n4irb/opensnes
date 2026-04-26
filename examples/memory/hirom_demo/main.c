/**
 * @file main.c
 * @brief HiROM memory mapping mode with 64KB bank access
 * @ingroup examples
 *
 * Demonstrates building an OpenSNES ROM in HiROM mode, where each ROM
 * bank provides 64KB of contiguous address space ($0000-$FFFF) instead
 * of LoROM's 32KB ($8000-$FFFF). HiROM is selected by setting
 * USE_HIROM := 1 in the Makefile, which switches the ROM header, memory
 * map, and linker configuration.
 *
 * HiROM is preferred for games with large contiguous data (streaming
 * tilesets, large lookup tables) because it avoids the 32KB bank
 * boundary that LoROM imposes. The tradeoff is that mirror-region
 * access patterns differ, and some addressing modes behave differently.
 *
 * This example uses inline 2bpp font tiles (no external assets) and
 * direct VRAM register writes to display text, confirming that the
 * library, ROM header, and memory map are all correctly configured
 * for HiROM operation. Pressing A changes the background color as
 * a simple input validation.
 *
 * @par SNES Concepts
 * - HiROM vs LoROM memory mapping (64KB vs 32KB banks)
 * - ROM header configuration for HiROM (USE_HIROM flag)
 * - Mode 0 with 2bpp inline font tiles
 * - Direct VRAM register writes for tilemap text rendering
 * - CGRAM palette writes for runtime color changes
 *
 * @par What to Observe
 * - "HIROM MODE" and "+ LIB" text displayed on a dark blue background
 * - Hold A to change the background color to light blue
 * - Release A to restore the dark blue background
 * - If the text appears garbled, the HiROM configuration has an issue
 *
 * @par Modules Used
 * console, dma, input, sprite, background
 *
 * @see console.h, dma.h, input.h
 */

#include <snes.h>
#include <snes/console.h>
#include <snes/dma.h>
#include <snes/input.h>


/*============================================================================
 * Embedded Font (2bpp, 16 bytes per tile)
 * Characters: space, H, I, R, O, M, D, E, L, B, +
 *============================================================================*/

/**
 * @brief Inline 2bpp font tile data (11 characters, 16 bytes per tile).
 *
 * Each 2bpp tile is 8x8 pixels stored as 16 bytes: 8 rows of 2 bytes
 * each (one byte per bitplane). Only 2 colors are used: color 0
 * (transparent/background) and color 1 (text). This avoids needing
 * an external font asset file -- the entire character set is embedded
 * in the C source.
 */
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

/** @brief Total font data size in bytes (11 characters x 16 bytes per 2bpp tile) */
#define FONT_SIZE (11 * 16)

/** @brief Tile index for space (blank tile) */
#define TILE_SPACE 0
#define TILE_H     1  /**< Tile index for letter H */
#define TILE_I     2  /**< Tile index for letter I */
#define TILE_R     3  /**< Tile index for letter R */
#define TILE_O     4  /**< Tile index for letter O */
#define TILE_M     5  /**< Tile index for letter M */
#define TILE_D     6  /**< Tile index for letter D */
#define TILE_E     7  /**< Tile index for letter E */
#define TILE_L     8  /**< Tile index for letter L */
#define TILE_B     9  /**< Tile index for letter B */
#define TILE_PLUS  10 /**< Tile index for plus sign (+) */

/*============================================================================
 * VRAM Configuration
 *============================================================================*/

/** @brief VRAM word address for the BG1 tilemap (32x32 = 2KB) */
#define TILEMAP_ADDR  0x0400
/** @brief VRAM word address for the tile character data */
#define TILES_ADDR    0x0000

/**
 * @brief Initial 2-color palette (dark blue background, white text).
 *
 * SNES CGRAM stores BGR555 colors as 2 bytes each (little-endian).
 * Color 0 = backdrop (dark blue), Color 1 = text foreground (white).
 */
static const u8 init_palette[] = {
    0x00, 0x28,  /* Color 0: Dark blue */
    0xFF, 0x7F,  /* Color 1: White */
};

/*============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief Write a single tile entry to the BG1 tilemap via direct VRAM registers.
 *
 * Sets the VRAM address to the tilemap position (row * 32 + column) and
 * writes the tile index as a 16-bit tilemap entry (low = tile number,
 * high = attributes). VRAM writes only succeed during VBlank or forced
 * blank -- calling this during active display will silently fail.
 *
 * @param x    Column position in the 32-wide tilemap (0-31)
 * @param y    Row position in the 32-tall tilemap (0-31)
 * @param tile Tile character index to write
 */
static void write_tile(u8 x, u8 y, u8 tile) {
    u16 addr;
    addr = TILEMAP_ADDR + y * 32 + x;
    REG_VMAIN = 0x80;
    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;
    REG_VMDATAL = tile;
    REG_VMDATAH = 0;
}

/**
 * @brief Clear the entire BG1 tilemap to blank (space) tiles.
 *
 * Writes 1024 tilemap entries (32x32) with tile index 0 (space) and
 * zero attributes. Must be called during force blank or VBlank.
 */
static void clear_tilemap(void) {
    dmaFillVRAM(TILE_SPACE, TILEMAP_ADDR, 1024 * 2);
}

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Main entry point -- HiROM mode validation demo.
 *
 * Verifies that the HiROM memory map, ROM header, and library all
 * work correctly by displaying text and responding to input. The
 * display is set up with inline 2bpp font tiles written directly
 * to VRAM registers, proving that code and data addresses resolve
 * properly in HiROM's 64KB-per-bank layout.
 *
 * Holding the A button changes the backdrop color as a simple
 * demonstration that the library's input system works in HiROM mode.
 *
 * @return 0 (never reached -- infinite game loop)
 */
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
    bgSetMapPtr(0, TILEMAP_ADDR, BG_MAP_32x32);
    bgSetGfxPtr(0, TILES_ADDR);
    setMainScreen(TM_BG1);

    /* Load palette via DMA */
    dmaCopyCGram((u8 *)init_palette, 0, 4);

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

        /* Read input using library function */
        pressed = padHeld(0);

        /* Change background color on A button */
        if (pressed & KEY_A) {
            setColor(0, RGB(0, 0, 31));   /* Light blue */
        } else {
            setColor(0, RGB(0, 0, 10));   /* Dark blue */
        }
    }

    return 0;
}
