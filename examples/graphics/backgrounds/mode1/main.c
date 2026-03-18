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

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    /* Force blank during setup */
    setScreenOff();

    /*------------------------------------------------------------------------
     * Configure Background Tilemap
     *------------------------------------------------------------------------*/

    /* BG1 tilemap at VRAM $0000, 32x32 tiles */
    bgSetMapPtr(0, 0x0000, SC_32x32);

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palette
     *------------------------------------------------------------------------*/

    /* BG1: tiles at $4000, palette at slot 0 (offset 0) */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);

    /*------------------------------------------------------------------------
     * Load Tilemap Data
     *------------------------------------------------------------------------*/

    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/

    /* Set Mode 1 (2x 16-color BG, 1x 4-color BG) */
    setMode(BG_MODE1, 0);

    /* Enable only BG1 on main screen */
    REG_TM = TM_BG1;

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
