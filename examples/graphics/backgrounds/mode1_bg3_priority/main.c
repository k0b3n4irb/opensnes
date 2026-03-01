/*
 * Mode 1 BG3 High Priority
 *
 * Demonstrates using BG3 as a high-priority HUD layer in Mode 1.
 * BG3 overlays BG1 and BG2, making it ideal for status bars and HUDs.
 *
 * VRAM layout:
 *   $0000  BG1 tilemap (2048 bytes)
 *   $0400  BG2 tilemap (2048 bytes)
 *   $0800  BG3 tilemap (2048 bytes)
 *   $2000  BG1 tiles (4bpp)
 *   $3000  BG2 tiles (4bpp)
 *   $4000  BG3 tiles (2bpp)
 *
 * Based on PVSnesLib example by odelot.
 * Backgrounds inspired by Streets of Rage 2.
 */

#include <snes.h>

extern u8 bg1_tiles, bg1_tiles_end;
extern u8 bg1_pal, bg1_pal_end;
extern u8 bg1_map, bg1_map_end;

extern u8 bg2_tiles, bg2_tiles_end;
extern u8 bg2_pal, bg2_pal_end;
extern u8 bg2_map, bg2_map_end;

extern u8 bg3_tiles, bg3_tiles_end;
extern u8 bg3_pal, bg3_pal_end;
extern u8 bg3_map, bg3_map_end;

int main(void) {
    consoleInit();

    /* Set tilemap locations */
    bgSetMapPtr(0, 0x0000, SC_32x32);
    bgSetMapPtr(1, 0x0400, SC_32x32);
    bgSetMapPtr(2, 0x0800, SC_32x32);

    /* Load tile data and palettes */
    bgInitTileSet(0, &bg1_tiles, &bg1_pal, 2,
                  &bg1_tiles_end - &bg1_tiles,
                  &bg1_pal_end - &bg1_pal,
                  BG_16COLORS, 0x2000);

    bgInitTileSet(1, &bg2_tiles, &bg2_pal, 4,
                  &bg2_tiles_end - &bg2_tiles,
                  &bg2_pal_end - &bg2_pal,
                  BG_16COLORS, 0x3000);

    bgInitTileSet(2, &bg3_tiles, &bg3_pal, 0,
                  &bg3_tiles_end - &bg3_tiles,
                  &bg3_pal_end - &bg3_pal,
                  BG_16COLORS, 0x4000);

    /* Copy tilemaps to VRAM during VBlank */
    WaitForVBlank();
    dmaCopyVram(&bg1_map, 0x0000, &bg1_map_end - &bg1_map);
    dmaCopyVram(&bg2_map, 0x0400, &bg2_map_end - &bg2_map);
    dmaCopyVram(&bg3_map, 0x0800, &bg3_map_end - &bg3_map);

    /* Mode 1 with BG3 high priority — BG3 renders on top of BG1+BG2 */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    /* Enable all three background layers */
    REG_TM = TM_BG1 | TM_BG2 | TM_BG3;
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
