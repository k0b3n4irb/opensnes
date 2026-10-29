/**
 * @file main.c
 * @brief Mode 3 full-color (256-color, 8bpp) background
 * @ingroup examples
 *
 * Displays a 256-color background using Mode 3, the highest color-depth
 * tiled mode on the SNES: a city canal at night, 253 colours, its haze a
 * gradient no 16-colour palette could hold. Each tile is 64 bytes
 * (8 bitplanes), so this tileset (770 tiles, 48 KB) is larger than a ROM
 * bank: the converter writes it in two parts, canal_tiles and canal_tiles_1,
 * and the linker places each where it has room. The generated
 * canal_tiles_load() copies both with dmaCopyVram(), whose pointer carries
 * the bank byte, so each part is taken wherever it landed.
 *
 * This began as the PVSnesLib "Mode3" example.
 *
 * @par SNES Concepts
 * - Mode 3: BG1 at 8bpp (256 colors from the whole CGRAM), BG2 at 4bpp
 * - 8bpp tile format: 64 bytes per tile (8 interleaved bitplanes)
 * - Data larger than a ROM bank: two parts, each copied with its own bank
 * - VRAM layout: tiles at $0000 (24640 words), tilemap at $7000
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

/** @brief The picture, converted by the build from res/canal.png (res/canal.png.toml). */
#include "res/canal.inc"   /* generated: canal_tiles[] + canal_tiles_1[] (the 48 KB tileset in two bank-sized parts) and canal_tiles_load(), canal_map[], canal_pal[] */

/** @brief VRAM word address of the tilemap, past the 770 tiles (770 x 32 words = $6040) */
#define MAP_ADDR 0x7000

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
    consoleInit();

    setScreenOff();

    /* The tile data is in two parts so the linker can place each in a bank
     * with room; canal_tiles_load(), from the generated canal.inc, copies
     * both: part n at 0x0000 + n * 0x4000 words, each pointer carrying its
     * bank byte. */
    canal_tiles_load(0x0000);
    dmaCopyVram(canal_map, MAP_ADDR, canal_map_end - canal_map);
    dmaCopyCGram(canal_pal, 0,       canal_pal_end - canal_pal);

    bgSetMapPtr(0, MAP_ADDR, SC_32x32);

    /* Mode 3: 256 colors, BG1 only */
    setMode(BG_MODE3, 0);
    setMainScreen(LAYER_BG1);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
