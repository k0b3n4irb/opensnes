/*
 * Mode1BG3HighPriority - PVSnesLib example ported to OpenSNES
 *
 * Demonstrates 3 BG layers in Mode 1:
 *   BG1: Back street background (palette slot 2, tiles at VRAM $2000)
 *   BG2: Front buildings (palette slot 4, tiles at VRAM $3000)
 *   BG3: HUD overlay with high priority (palette slot 0, tiles at VRAM $4000)
 *
 * BG3 in Mode 1 is 2bpp (4 colors). With BG3_MODE1_PRIORITY_HIGH,
 * high-priority BG3 tiles render in front of all other BG layers.
 */

#include <snes.h>

/* BG1 assets (back street, palette slot 2) */
extern u8 bg1_tiles[], bg1_tiles_end[];
extern u8 bg1_map[], bg1_map_end[];
extern u8 bg1_pal[];

/* BG2 assets (front buildings, palette slot 4) */
extern u8 bg2_tiles[], bg2_tiles_end[];
extern u8 bg2_map[], bg2_map_end[];
extern u8 bg2_pal[];

/* BG3 assets (HUD overlay, 2bpp, palette slot 0) */
extern u8 bg3_tiles[], bg3_tiles_end[];
extern u8 bg3_map[], bg3_map_end[];
extern u8 bg3_pal[];

int main(void) {
    consoleInit();

    /* Configure tilemap addresses */
    bgSetMapPtr(0, 0x0000, BG_MAP_32x32);
    bgSetMapPtr(1, 0x0400, BG_MAP_32x32);
    bgSetMapPtr(2, 0x0800, BG_MAP_32x32);

    /* Load tile graphics + palettes */
    /* BG1: palette slot 2 (colors 32-47) */
    bgInitTileSet(0, bg1_tiles, bg1_pal, 2,
                  bg1_tiles_end - bg1_tiles, 16 * 2,
                  BG_16COLORS, 0x2000);

    /* BG2: palette slot 4 (colors 64-79) */
    bgInitTileSet(1, bg2_tiles, bg2_pal, 4,
                  bg2_tiles_end - bg2_tiles, 16 * 2,
                  BG_16COLORS, 0x3000);

    /* BG3: palette slot 0, 2bpp in Mode 1 = 4 colors */
    bgInitTileSet(2, bg3_tiles, bg3_pal, 0,
                  bg3_tiles_end - bg3_tiles, 4 * 2,
                  BG_16COLORS, 0x4000);

    /* Load tilemaps (during VBlank for VRAM safety) */
    WaitForVBlank();
    dmaCopyVram(bg1_map, 0x0000, bg1_map_end - bg1_map);
    dmaCopyVram(bg2_map, 0x0400, bg2_map_end - bg2_map);
    dmaCopyVram(bg3_map, 0x0800, bg3_map_end - bg3_map);

    /* Mode 1 with BG3 high priority for HUD overlay */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    REG_TM = TM_BG1 | TM_BG2 | TM_BG3;
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
