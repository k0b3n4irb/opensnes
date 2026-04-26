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

/** @name BG1 asset labels (4bpp layer, defined in data.asm via .incbin)
 * @{ */
extern u8 bg1_tiles, bg1_tiles_end;  /**< BG1 tile graphics */
extern u8 bg1_pal, bg1_pal_end;      /**< BG1 16-color palette (CGRAM bank 2) */
extern u8 bg1_map, bg1_map_end;      /**< BG1 tilemap */
/** @} */

/** @name BG2 asset labels (4bpp layer, defined in data.asm via .incbin)
 * @{ */
extern u8 bg2_tiles, bg2_tiles_end;  /**< BG2 tile graphics */
extern u8 bg2_pal, bg2_pal_end;      /**< BG2 16-color palette (CGRAM bank 4) */
extern u8 bg2_map, bg2_map_end;      /**< BG2 tilemap */
/** @} */

/** @name BG3 asset labels (2bpp overlay layer, defined in data.asm via .incbin)
 * @{ */
extern u8 bg3_tiles, bg3_tiles_end;  /**< BG3 tile graphics */
extern u8 bg3_pal, bg3_pal_end;      /**< BG3 palette (CGRAM bank 0) */
extern u8 bg3_map, bg3_map_end;      /**< BG3 tilemap (HUD/overlay) */
/** @} */

/**
 * @brief Entry point -- load 3 BG layers and display with BG3 high priority
 *
 * Sets up a Mode 1 scene with three background layers. BG1 and BG2 are
 * 4bpp (16 colors each) providing the main scene, while BG3 is 2bpp
 * (4 colors) used as an overlay/HUD layer. The key feature is
 * BG3_MODE1_PRIORITY_HIGH passed to setMode(), which promotes BG3 above
 * BG1 and BG2 in the rendering order. Without this flag, BG3 would render
 * behind the other layers (lowest priority by default in Mode 1).
 *
 * Each layer loads into its own VRAM region and uses a separate CGRAM
 * palette bank (controlled by the palette offset parameter in
 * bgInitTileSet()). The palette offset is in units of 16 colors: offset 2
 * means CGRAM colors 32-47 (BG1), offset 4 means colors 64-79 (BG2),
 * and offset 0 means colors 0-15 (BG3).
 *
 * @return Never returns (infinite loop).
 */
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
    setMainScreen(TM_BG1 | TM_BG2 | TM_BG3);
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
