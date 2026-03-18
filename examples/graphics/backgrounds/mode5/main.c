/**
 * @file main.c
 * @brief Mode 5 hi-resolution background display
 * @ingroup examples
 *
 * Demonstrates SNES BG Mode 5 (512x256, 4bpp, 16 colors). Mode 5 doubles
 * the horizontal resolution from 256 to 512 pixels by using interlaced
 * tile data and requiring BG1 on both the main and sub screens. The PPU
 * alternates even/odd pixel columns between the two screens each frame,
 * producing a true 512-pixel-wide output on compatible displays. Tile data
 * must be converted with the special `-M 5` interleaving flag.
 *
 * @par SNES Concepts
 * - BG Mode 5 hi-res rendering (512px horizontal)
 * - Main screen + sub screen cooperation for interlaced output
 * - Mode 5 tile data interleaving (`gfx4snes -M 5`)
 * - DMA tilemap transfer to VRAM
 *
 * @par What to Observe
 * - A single full-screen image rendered at 512-pixel horizontal resolution
 * - Text and fine detail should appear sharper than standard 256px modes
 *
 * @par Modules Used
 * console, dma, background
 *
 * @see background.h, dma.h, video.h
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

    /* Mode 5: hi-res, BG1 on both main+sub (required for 512px) */
    setMode(BG_MODE5, 0);
    setMainScreen(LAYER_BG1);
    setSubScreen(LAYER_BG1);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
