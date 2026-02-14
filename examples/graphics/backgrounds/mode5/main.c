/**
 * @file main.c
 * @brief Mode 5 Example - Hi-res 512x256
 *
 * Port of PVSnesLib Mode5 example.
 * Demonstrates Mode 5 hi-res interlaced graphics.
 *
 * Mode 5 characteristics:
 * - 512x256 resolution (interlaced)
 * - BG1: 16 colors (4bpp)
 * - BG2: 4 colors (2bpp)
 * - Both main and sub screens enabled
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /*------------------------------------------------------------------------
     * Configure Background Tilemap
     *------------------------------------------------------------------------*/

    /* BG1 tilemap at VRAM $6000, 32x32 tiles */
    bgSetMapPtr(0, 0x6000, SC_32x32);

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palette
     *------------------------------------------------------------------------*/

    /* BG1: tiles at $0000, palette at slot 0 */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x0000);

    /*------------------------------------------------------------------------
     * Load Tilemap Data
     *------------------------------------------------------------------------*/

    dmaCopyVram(tilemap, 0x6000, tilemap_end - tilemap);

    /*------------------------------------------------------------------------
     * Configure Mode 5 (Hi-res)
     *------------------------------------------------------------------------*/

    /* Mode 5: 512x256 interlaced, BG1=16 colors, BG2=4 colors */
    REG_BGMODE = 0x05;

    /* Enable BG1 on main screen */
    REG_TM = TM_BG1;

    /* Enable BG1 on sub screen (needed for hi-res interlace) */
    REG_TS = TM_BG1;

    /* Set scroll to (0,0) */
    bgSetScroll(0, 0, 0);

    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
