/**
 * @file main.c
 * @brief Mode 3 full-color (256-color, 8bpp) background
 * @ingroup examples
 *
 * Displays a 256-color background using Mode 3, the highest color-depth
 * tiled mode on the SNES. Each tile is 64 bytes (8 bitplanes), so this
 * tileset (39 KB) is larger than a ROM bank: the converter writes it in two
 * parts, bg_tiles and bg_tiles_1, and the linker places each where it has
 * room. A C pointer carries its bank byte, so dmaCopyVram() takes each part
 * wherever it landed.
 *
 * This is a port of the PVSnesLib "Mode3" example.
 *
 * @par SNES Concepts
 * - Mode 3: BG1 at 8bpp (256 colors from the whole CGRAM), BG2 at 4bpp
 * - 8bpp tile format: 64 bytes per tile (8 interleaved bitplanes)
 * - Data larger than a ROM bank: two parts, each copied with its own bank
 * - VRAM layout: tiles at $0000 (large), tilemap at $6000
 *
 * @par What to Observe
 * - A single full-screen image with up to 256 unique colors
 * - Richer color gradients and detail compared to Mode 1 (16 colors)
 * - Static display with no scrolling
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>

/** @brief The picture, converted by the build from res/bg.bmp (res/bg.bmp.toml). */
#include "res/bg.inc"   /* generated: bg_tiles[] + bg_tiles_1[] (the 39 KB tileset in two bank-sized parts), bg_map[], bg_pal[] */

/**
 * @brief Entry point -- load 256-color tileset and display Mode 3 image
 *
 * Forces blank, DMAs the 8bpp tile data (two parts, each at most a ROM
 * bank), the tilemap and the 256-color palette into VRAM/CGRAM, then
 * configures Mode 3 and turns on the screen. BG1 has 256 colors per tile
 * (8 bitplanes), the richest color depth of any tiled SNES mode; the
 * tradeoff is one such layer, plus a 16-color BG2 that is unused here.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    setScreenOff();

    /* The tile data is in two parts so the linker can place each in a bank
     * with room; the pointer passed carries the bank byte. */
    dmaCopyVram(bg_tiles,   0x0000, bg_tiles_end - bg_tiles);
    dmaCopyVram(bg_tiles_1,  0x4000, bg_tiles_1_end - bg_tiles_1);
    dmaCopyVram(bg_map, 0x6000, bg_map_end - bg_map);
    dmaCopyCGram(bg_pal, 0,     bg_pal_end - bg_pal);

    bgSetMapPtr(0, 0x6000, SC_32x32);

    /* Mode 3: 256 colors, BG1 only */
    setMode(BG_MODE3, 0);
    setMainScreen(LAYER_BG1);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
