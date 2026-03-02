/*
 * Mode 1 Mixed Scroll
 *
 * Two Mode 1 backgrounds layered together:
 *   BG1 (shader pattern) auto-scrolls diagonally each frame
 *   BG2 (pvsneslib logo) stays fixed
 *
 * VRAM layout:
 *   $1400  BG2 tilemap (32x28, 1792 bytes)
 *   $1800  BG1 tilemap (32x32, 2048 bytes)
 *   $4000  BG1 tiles (shader, 4bpp)
 *   $5000  BG2 tiles (pvsneslib, 4bpp)
 *
 * CGRAM:
 *   Palette 0 (colors 0-15):  pvsneslib logo
 *   Palette 1 (colors 16-31): shader pattern
 *
 * Port of PVSnesLib Mode1MixedScroll example by alekmaul.
 */

#include <snes.h>

extern u8 bg1_tiles, bg1_tiles_end;
extern u8 bg1_pal, bg1_pal_end;
extern u8 bg1_map, bg1_map_end;

extern u8 bg2_tiles, bg2_tiles_end;
extern u8 bg2_pal, bg2_pal_end;
extern u8 bg2_map, bg2_map_end;

short scrX = 0, scrY = 0;

int main(void) {
    consoleInit();

    /* Load BG2 tiles + palette (pvsneslib logo, palette slot 0, tiles at $5000) */
    bgInitTileSet(1, &bg2_tiles, &bg2_pal, 0,
                  &bg2_tiles_end - &bg2_tiles,
                  &bg2_pal_end - &bg2_pal,
                  BG_16COLORS, 0x5000);

    /* Load BG1 tiles + palette (shader, palette slot 1, tiles at $4000) */
    bgInitTileSet(0, &bg1_tiles, &bg1_pal, 1,
                  &bg1_tiles_end - &bg1_tiles,
                  &bg1_pal_end - &bg1_pal,
                  BG_16COLORS, 0x4000);

    /* Set tilemap locations */
    bgSetMapPtr(1, 0x1400, SC_32x32);
    bgSetMapPtr(0, 0x1800, SC_32x32);

    /* Copy tilemaps to VRAM */
    WaitForVBlank();
    dmaCopyVram(&bg2_map, 0x1400, &bg2_map_end - &bg2_map);
    dmaCopyVram(&bg1_map, 0x1800, &bg1_map_end - &bg1_map);

    /* Mode 1, enable only BG1 + BG2 (BG3 disabled) */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_BG2;

    setScreenOn();

    while (1) {
        scrX++;
        scrY++;
        bgSetScroll(0, scrX, scrY);

        WaitForVBlank();
    }
    return 0;
}
