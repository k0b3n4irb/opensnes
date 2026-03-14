/**
 * @file main.c
 * @brief Mode 5 Hi-Res Background Demo
 *
 * Demonstrates BG Mode 5 (512×256, 16-color, 4bpp).
 * Mode 5 is the SNES high-resolution mode, doubling horizontal
 * resolution to 512 pixels using interlaced tile data.
 *
 * Ported from PVSnesLib "Mode5" example.
 */

#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    /* Load tiles + palette to VRAM/CGRAM */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x0000);

    /* Load tilemap to VRAM */
    WaitForVBlank();
    dmaCopyVram(tilemap, 0x6000, tilemap_end - tilemap);
    bgSetMapPtr(0, 0x6000, SC_32x32);

    /* Mode 5: hi-res, BG1 only */
    setMode(BG_MODE5, 0);
    setMainScreen(LAYER_BG1);
    setSubScreen(0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
