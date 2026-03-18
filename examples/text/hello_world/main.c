/**
 * @file main.c
 * @brief Minimal "Hello World" text display using hand-coded 2bpp font tiles
 * @ingroup examples
 *
 * The simplest possible SNES program: writes a hand-coded 2bpp bitmap font
 * into VRAM, builds a tilemap row to spell "HELLO WORLD!", and displays it
 * on BG1 in Mode 0. No external assets or font library are used -- every
 * tile is defined as raw bitplane data in a C array, making this example a
 * good starting point for understanding how the SNES PPU interprets tile
 * graphics and tilemaps.
 *
 * @par SNES Concepts
 * - 2bpp tile format: each 8x8 tile is 16 bytes (two bitplanes interleaved)
 * - VRAM tilemap structure: 32x32 entries, each a 16-bit word (tile index + attributes)
 * - CGRAM palette: two BGR555 colors loaded at palette index 0
 * - Mode 0: four BG layers, each limited to 4 colors (2bpp)
 * - DMA transfers for tile data and palette via dmaCopyVram / dmaCopyCGram
 *
 * @par What to Observe
 * - "HELLO WORLD!" rendered in white text on a dark blue background
 * - The text appears centered at row 14, column 10 of the 32x32 tilemap
 * - The screen is otherwise blank (tilemap filled with tile 0, the space glyph)
 *
 * @par Modules Used
 * console, sprite, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>

/**
 * @brief Hand-coded 2bpp bitmap font tiles (9 glyphs, 16 bytes each)
 *
 * Each 8x8 tile occupies 16 bytes in SNES 2bpp format: two interleaved
 * bitplanes per pixel row. Bitplane 0 selects palette color 1, bitplane 1
 * selects color 2, and both together select color 3. All bitplane-1 bytes
 * are zero here, so every lit pixel maps to palette color 1 (white).
 *
 * Tile index map: 0=Space, 1=H, 2=E, 3=L, 4=O, 5=W, 6=R, 7=D, 8=!
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

/**
 * @brief "HELLO WORLD!" encoded as tile indices into font_tiles[]
 *
 * Each byte is an index into the font tile array. The string is terminated
 * by 0xFF, which acts as an end-of-message sentinel (not a valid tile index).
 */
static const u8 message[] = {
    1, 2, 3, 3, 4,  /* HELLO */
    0,              /* space */
    5, 4, 6, 3, 7,  /* WORLD */
    8,              /* ! */
    0xFF            /* end marker */
};

/**
 * @brief Background palette in BGR555 format (2 colors, 4 bytes total)
 *
 * SNES CGRAM stores colors as 15-bit BGR555: bbbbbgggggrrrrr in two bytes
 * (little-endian). Color 0 is the background fill, color 1 is the text.
 * Only 2 of the 4 available Mode 0 palette entries are used.
 */
static const u8 bg_palette[] = {
    0x00, 0x28,  /* Color 0: Dark blue */
    0xFF, 0x7F,  /* Color 1: White */
};

/**
 * @brief Entry point -- set up PPU, load font, build tilemap, display text
 *
 * Initializes the SNES hardware via consoleInit(), loads a hand-coded 2bpp
 * font into VRAM, fills the 32x32 tilemap with blank tiles, writes the
 * "HELLO WORLD!" message at a specific tilemap row/column, and enters an
 * infinite VBlank loop. All VRAM writes happen during forced blank (screen
 * off) to satisfy the PPU timing constraint.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 addr;   /**< VRAM word address for tilemap cursor position */
    u8 tile;    /**< Current tile index read from message[] */
    u16 i;      /**< Loop counter for tilemap fill and message write */

    /* Initialize console hardware using library */
    consoleInit();

    /* Set Mode 0 using library (4 BG layers, all 2bpp) */
    setMode(BG_MODE0, 0);

    /* Configure BG1 for text display */
    bgSetMapPtr(0, 0x0400, BG_MAP_32x32);
    bgSetGfxPtr(0, 0x0000);

    /* Load font tiles to VRAM $0000 via DMA */
    dmaCopyVram((u8 *)font_tiles, 0x0000, 144);

    /* Load palette via DMA */
    dmaCopyCGram((u8 *)bg_palette, 0, 4);

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
    setMainScreen(LAYER_BG1);

    /* Turn on screen using library */
    setScreenOn();

    /* Main loop with VBlank sync */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
