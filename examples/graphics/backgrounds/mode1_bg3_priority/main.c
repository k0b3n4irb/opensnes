/**
 * @file main.c
 * @brief Mode 1 BG3 high-priority overlay for HUD-style layering
 * @ingroup examples
 *
 * Demonstrates the Mode 1 BG3 priority bit, which promotes the normally
 * lowest-priority 2bpp layer to render above BG1 and BG2. This hardware
 * feature is how most SNES games implement status bars and HUDs that
 * overlay the gameplay scene without using sprites. Three separate
 * backgrounds are loaded: BG1 and BG2 provide the scene (4bpp, 16 colors
 * each with separate palette banks), while BG3 (2bpp, 4 colors) draws on
 * top of everything when BG3_MODE1_PRIORITY_HIGH is set in setMode().
 *
 * Based on the PVSnesLib example by odelot. Backgrounds inspired by
 * Streets of Rage 2.
 *
 * @par SNES Concepts
 * - BG3 priority bit (BG3_MODE1_PRIORITY_HIGH): promotes BG3 above BG1+BG2
 * - Mode 1 layer depths: normally BG1 > BG2 > BG3, but priority bit inverts BG3
 * - Multi-layer VRAM layout with separate tilemap and tile regions per BG
 * - Palette banking via bgInitTileSet() palette offset parameter (e.g., 2, 4 for 32/64-byte CGRAM offsets)
 * - VRAM layout: tilemaps at $0000/$0400/$0800, tiles at $2000/$3000/$4000
 *
 * @par What to Observe
 * - Three background layers visible simultaneously
 * - BG3 (the HUD/overlay layer) renders on top of BG1 and BG2
 * - Each layer uses a different palette bank (no color sharing between layers)
 * - The scene is static (no scrolling or animation)
 *
 * @par Modules Used
 * console, sprite, dma, background
 *
 * @see background.h, dma.h, video.h
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
