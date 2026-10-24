/**
 * @file main.c
 * @brief Fundamentals — how a glyph is drawn: hand-coded 2bpp font tiles
 * @ingroup examples
 *
 * The under-the-hood companion to the Text family. Instead of calling the
 * `text` module, it writes a hand-coded 2bpp bitmap font straight into VRAM,
 * builds a tilemap row to spell "HELLO WORLD!", and shows it on BG1 in Mode
 * 0, in the city's colours: cyan letters with a pink shadow on a terminal's
 * glass. Every tile is raw bitplane data in a C array — no assets, no font
 * library — so you see exactly how the SNES PPU turns bytes into pixels.
 * Read `examples/text/print_string` first for the easy way; read this to
 * understand what that module hides.
 *
 * @par SNES Concepts
 * - 2bpp tile format: each 8x8 tile is 16 bytes (two bitplanes interleaved);
 *   plane 0 draws colour 1, plane 1 colour 2
 * - VRAM tilemap structure: 32x32 entries, each a 16-bit word (tile index + attributes)
 * - CGRAM palette: three BGR555 colors loaded at palette index 0
 * - Mode 0: four BG layers, each limited to 4 colors (2bpp)
 * - DMA transfers for tile data and palette via dmaCopyVram / dmaCopyCGram
 *
 * @par What to Observe
 * - "HELLO WORLD!" in cyan, each letter with a pink shadow one pixel right
 *   and down, on the dark glass of a terminal
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
 * bitplanes per pixel row, plane 0 first. A pixel's colour is its plane 1
 * bit times two plus its plane 0 bit. Plane 0 holds the letter, the glyph of
 * the city's font (the `text` module's, lib/source/text.asm): colour 1, cyan.
 * Plane 1 holds its shadow, the letter moved one pixel right and one down,
 * where the letter is not: colour 2, pink. No pixel has both bits, so colour
 * 3 is never drawn.
 *
 * Tile index map: 0=Space, 1=H, 2=E, 3=L, 4=O, 5=W, 6=R, 7=D, 8=!
 */
static const u8 font_tiles[] = {
    /* Tile 0: Space (blank) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* Tile 1: H */
    0x66, 0x00, 0x66, 0x11, 0x66, 0x11, 0x7E, 0x01,
    0x66, 0x19, 0x66, 0x11, 0x66, 0x11, 0x00, 0x33,

    /* Tile 2: E */
    0x7E, 0x00, 0x60, 0x1F, 0x60, 0x10, 0x7C, 0x00,
    0x60, 0x1E, 0x60, 0x10, 0x7E, 0x00, 0x00, 0x3F,

    /* Tile 3: L */
    0x60, 0x00, 0x60, 0x10, 0x60, 0x10, 0x60, 0x10,
    0x60, 0x10, 0x60, 0x10, 0x7E, 0x00, 0x00, 0x3F,

    /* Tile 4: O */
    0x3C, 0x00, 0x66, 0x18, 0x66, 0x11, 0x66, 0x11,
    0x66, 0x11, 0x66, 0x11, 0x3C, 0x03, 0x00, 0x1E,

    /* Tile 5: W */
    0xC6, 0x00, 0xC6, 0x21, 0xC6, 0x21, 0xD6, 0x21,
    0xFE, 0x01, 0xEE, 0x11, 0xC6, 0x31, 0x00, 0x63,

    /* Tile 6: R */
    0x7C, 0x00, 0x66, 0x18, 0x66, 0x11, 0x7C, 0x03,
    0x6C, 0x12, 0x66, 0x10, 0x66, 0x11, 0x00, 0x33,

    /* Tile 7: D */
    0x78, 0x00, 0x6C, 0x10, 0x66, 0x10, 0x66, 0x11,
    0x66, 0x11, 0x6C, 0x13, 0x78, 0x06, 0x00, 0x3C,

    /* Tile 8: ! */
    0x18, 0x00, 0x18, 0x04, 0x18, 0x04, 0x18, 0x04,
    0x18, 0x04, 0x00, 0x0C, 0x18, 0x00, 0x00, 0x0C,
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
 * @brief Background palette in BGR555 format (3 colors, 6 bytes total)
 *
 * SNES CGRAM stores colors as 15-bit BGR555: 0bbbbbgggggrrrrr in two bytes
 * (little-endian). Color 0 is the background fill (a terminal's glass),
 * color 1 the letters, color 2 their shadow: the neon of the Rue basse.
 * Color 3 of the 4 Mode 0 palette entries is not used.
 */
static const u8 bg_palette[] = {
    0x61, 0x10,  /* Color 0: the glass, #0a1a22 */
    0x49, 0x73,  /* Color 1: cyan, #48d0e0 */
    0xB4, 0x30,  /* Color 2: pink, #a02860 */
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
    dmaCopyCGram((u8 *)bg_palette, 0, 6);

    /* Fill tilemap with spaces (tile 0) */
    dmaFillVRAM(0, 0x0400, 1024 * 2);

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
