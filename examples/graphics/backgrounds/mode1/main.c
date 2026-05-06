/**
 * @file main.c
 * @brief Static 16-color background display in Mode 1 (4bpp)
 * @ingroup examples
 *
 * Displays a single full-screen tiled background using Mode 1, the most
 * commonly used SNES video mode. Mode 1 provides two 4bpp layers (BG1 and
 * BG2, each up to 16 colors) plus one 2bpp layer (BG3, 4 colors). This
 * example uses only BG1 with a 16-color tileset and tilemap converted from
 * a PNG by gfx4snes.
 *
 * This is a direct port of the PVSnesLib "Mode1" example.
 *
 * @par SNES Concepts
 * - Mode 1: BG1/BG2 are 4bpp (16 colors each), BG3 is 2bpp (4 colors)
 * - bgInitTileSet() for combined tile + palette loading with VRAM address configuration
 * - dmaCopyVram() for tilemap data transfer to a separate VRAM region
 * - VRAM layout: tilemap at $0000, tiles at $4000 (non-overlapping regions)
 * - Force blank (setScreenOff) during VRAM setup to avoid mid-frame writes
 *
 * @par What to Observe
 * - A static full-screen image rendered as 8x8 tiles on BG1
 * - Only BG1 is enabled; BG2 and BG3 are unused
 * - The image remains stationary (no scrolling)
 *
 * @par Modules Used
 * console, sprite, dma, background
 *
 * @see background.h, dma.h, video.h
 */

#include <snes.h>
#include <snes/asset.h>

/*============================================================================
 * External Graphics Data
 *
 * Symbols are referenced through the BG_LOAD() macro, which expands to
 * `extern u8 bg_tiles[], bg_tiles_end[]` (and the `bg_pal*`, `bg_map*`
 * pairs) plus the three hardware calls. The data labels themselves
 * live in data.asm.
 *============================================================================*/

/**
 * @brief Entry point -- load a 4bpp tileset and display a static Mode 1 image
 *
 * Demonstrates the asset bundle convention: a single BG_LOAD() macro
 * does the same work as a manual `bgSetMapPtr` + `bgInitTileSet` +
 * `dmaCopyVram` triple, with the symbol naming convention as the
 * single source of truth (data.asm exports `bg_tiles`, `bg_pal`,
 * `bg_map` and the macro derives the externs and sizes).
 *
 * The VRAM layout places the tilemap at $0000 and tile graphics at $4000
 * to avoid overlap (each tilemap is up to 2KB, tile data can be many KB).
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    /* Force blank during setup */
    setScreenOff();

    /*------------------------------------------------------------------------
     * Load Background Bundle (tiles + palette + tilemap)
     *------------------------------------------------------------------------*/

    /* BG1: 16-color 4bpp, 32x32 map, tiles at $4000, tilemap at $0000 */
    BG_LOAD(bg, 0, 0, BG_16COLORS, SC_32x32, 0x4000, 0x0000);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/

    /* Set Mode 1 (2x 16-color BG, 1x 4-color BG) */
    setMode(BG_MODE1, 0);

    /* Enable only BG1 on main screen */
    setMainScreen(TM_BG1);

    /* Set scroll to (0,0) */
    bgSetScroll(0, 0, 0);

    /* Enable display at full brightness */
    setScreenOn();

    /* Main loop - just wait for VBlank */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
