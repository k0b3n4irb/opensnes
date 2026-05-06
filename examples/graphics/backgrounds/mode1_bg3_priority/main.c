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
#include <snes/asset.h>

/* Asset symbols (bg1_*, bg2_*, bg3_*) live in data.asm. The BG_LOAD
 * macro derives the externs from the prefix; no manual declarations
 * needed here. */

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
 * palette bank. The palette slot argument to BG_LOAD is in units of 16
 * colors: slot 2 means CGRAM colors 32-47 (BG1), slot 4 means colors
 * 64-79 (BG2), and slot 0 means colors 0-15 (BG3).
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    consoleInit();
    WaitForVBlank();

    /* BG1: tiles at $2000, map at $0000, palette slot 2 */
    BG_LOAD(bg1, 0, 2, BG_16COLORS, SC_32x32, 0x2000, 0x0000);

    /* BG2: tiles at $3000, map at $0400, palette slot 4 */
    BG_LOAD(bg2, 1, 4, BG_16COLORS, SC_32x32, 0x3000, 0x0400);

    /* BG3: tiles at $4000, map at $0800, palette slot 0 (HUD overlay) */
    BG_LOAD(bg3, 2, 0, BG_16COLORS, SC_32x32, 0x4000, 0x0800);

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
