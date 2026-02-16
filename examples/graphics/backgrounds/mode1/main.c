/**
 * @file main.c
 * @brief Mode 1 Background Example
 *
 * Port of pvsneslib Mode1 example.
 * Demonstrates how to display a simple 16-color tiled background in Mode 1.
 *
 * Mode 1 is the most commonly used background mode:
 * - BG1 and BG2: 16 colors (4bpp)
 * - BG3: 4 colors (2bpp) - often used for text overlays
 *
 * This example shows:
 * - Loading tiles to VRAM using bgInitTileSet()
 * - Loading tilemap to VRAM using dmaCopyVram()
 * - Setting up Mode 1 with library functions
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
