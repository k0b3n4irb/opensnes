/**
 * @file main.c
 * @brief Two ways to mix two layers: colour addition, and pseudo-hires
 * @ingroup examples
 *
 * Demonstrates the SNES color math unit by blending two background layers.
 * BG1 (main screen) shows a 4bpp landscape, while BG3 (sub screen) holds
 * 2bpp semi-transparent clouds that scroll horizontally. The PPU adds the
 * sub screen pixel colors to the main screen via registers CGWSEL ($2130)
 * and CGADSUB ($2131), producing a translucent overlay effect.
 *
 * Mode 1 is used with BG3 priority high so the 2bpp cloud layer renders
 * above BG1. Color addition is applied to BG1 and the backdrop, meaning
 * the cloud colors are numerically added to whatever is underneath.
 *
 * Press A for the other way the PPU mixes two layers: pseudo-hires (SETINI
 * bit 3). The colour math is switched off, and the PPU interleaves the sub
 * screen (even columns) with the main screen (odd columns) into a 512-pixel
 * line: a 50 % blend that costs no colour math at all, at the price of the
 * horizontal resolution of each layer. A again goes back to addition.
 *
 * Based on PVSnesLib Transparency example by Alekmaul; the pseudo-hires mode
 * was the pseudo_hires example, merged here.
 *
 * @par SNES Concepts
 * - Color math: addition mode via CGWSEL/CGADSUB registers
 * - Main screen vs sub screen layer assignment
 * - Mode 1 with BG3 priority high for overlay effects
 * - 4bpp (BG1) and 2bpp (BG3) tile formats in the same mode
 * - Pseudo-hires: SETINI ($2133) bit 3, videoSetPseudoHires(); main and sub
 *   screens alternate pixel by pixel on a 512-pixel line
 *
 * @par What to Observe
 * - A landscape background with semi-transparent clouds scrolling over it
 * - The clouds blend additively, brightening the landscape beneath them
 * - The clouds scroll by themselves
 * - Press A: the clouds are no longer added but interleaved column by column
 *   with the landscape (pseudo-hires); A again: back to addition
 *
 * @par Modules Used
 * console, sprite, dma, background, colormath, input
 *
 * @see colormath.h, background.h, video.h
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/video.h>
#include <snes/background.h>
#include <snes/colormath.h>

/** @brief 1 while the layers are mixed by pseudo-hires instead of addition */
u8 ph_on;

/**
 * @brief The landscape and the clouds, converted by the build from res/.
 *
 * A C pointer carries its bank byte, so `dmaCopyVram` / `dmaCopyCGram`
 * take the data wherever the linker placed it.
 */
#include "res/backgrounds.inc"
#include "res/clouds.inc"
/* generated: backgrounds_tiles/_map/_pal (the landscape), clouds_tiles/_map/_pal (each with _end) */

/**
 * @brief Entry point: color math transparency with scrolling clouds over a landscape.
 *
 * Sets up Mode 1 with two background layers:
 * - BG1 (main screen): 4bpp landscape, static
 * - BG3 (sub screen): 2bpp semi-transparent clouds, scrolling horizontally
 *
 * The SNES color math unit adds the sub screen pixel colors to the main screen,
 * producing a translucent cloud overlay. BG3 is given priority high so its
 * tiles render above BG1 in Mode 1's layer ordering.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    /** @brief Horizontal scroll offset for the cloud layer (BG3), incremented each frame */
    s16 scrX = 0;

    consoleInit();

    /* Force blank (bit 7 of INIDISP) disables rendering, allowing unlimited
     * VRAM/CGRAM write time. Required because the graphics data is too large
     * for a single VBlank DMA transfer (~4KB budget). */
    setScreenOff();

    /* Load BG1 (4bpp landscape) tiles, tilemap, and palette */
    dmaCopyVram(backgrounds_tiles, 0x0000, backgrounds_tiles_end - backgrounds_tiles);
    dmaCopyVram(backgrounds_map,   0x2000, backgrounds_map_end   - backgrounds_map);
    dmaCopyCGram(backgrounds_pal,  16,     backgrounds_pal_end   - backgrounds_pal);

    /* Load BG3 (2bpp clouds) tiles, tilemap, and palette */
    dmaCopyVram(clouds_tiles, 0x1000, clouds_tiles_end - clouds_tiles);
    dmaCopyVram(clouds_map,   0x2400, clouds_map_end   - clouds_map);
    dmaCopyCGram(clouds_pal,  0,      clouds_pal_end   - clouds_pal);

    /* BG1: map at VRAM $2000 (32x32 tiles), tiles at $0000 */
    bgSetMapPtr(0, 0x2000, SC_32x32);
    bgSetGfxPtr(0, 0x0000);

    /* BG3: map at VRAM $2400, tiles at $1000. BG3 uses 2bpp tiles in
     * Mode 1, half the VRAM per tile of a 4bpp layer. */
    bgSetMapPtr(2, 0x2400, SC_32x32);
    bgSetGfxPtr(2, 0x1000);

    /* Mode 1 with BG3 priority high. In Mode 1, BG3 is normally 2bpp and
     * drawn behind BG1/BG2. Setting BG3_MODE1_PRIORITY_HIGH promotes BG3's
     * high-priority tiles above all other BG layers, which is essential for
     * the cloud overlay to be visible on top of the landscape. */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    /* Main screen: the layers that form the final displayed image.
     * Both BG1 (landscape) and BG3 (clouds) must be on the main screen
     * for the clouds to be visible. */
    setMainScreen(LAYER_BG1 | LAYER_BG3);

    /* Sub screen: the source for color math blending. BG3 on the sub screen
     * provides the color values that will be added to the main screen pixels.
     * Without this, the color math unit has no source data to blend. */
    setSubScreen(LAYER_BG3);

    /* Configure the color math unit for additive blending:
     * - Source = sub screen (BG3 cloud colors)
     * - Operation = ADD (cloud RGB values added to main screen pixels)
     * - Targets = BG1 + backdrop (both receive the cloud color addition)
     *
     * The backdrop is the "background color" (CGRAM entry 0) visible where
     * no BG tile is drawn. Including it ensures clouds blend even over empty
     * areas. */
    colorMathInit();
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetLayers(LAYER_BG1 | COLORMATH_BACKDROP);

    setScreenOn();

    while (1) {
        /* Scroll BG3 (clouds) one pixel right each frame.
         * bgSetScroll(bg_index, x, y) — bg_index 2 = BG3 (0-indexed).
         * The scroll value wraps naturally at the tilemap width, creating
         * seamless looping if the tilemap tiles horizontally. */
        scrX++;
        bgSetScroll(2, scrX, 0);

        WaitForVBlank();

        /* A: addition <-> pseudo-hires. In pseudo-hires the colour math is
         * off (no layer in CGADSUB): the sub screen reaches the picture only
         * through the interleaving. */
        if (padPressed(0) & KEY_A) {
            ph_on ^= 1;
            if (ph_on) {
                colorMathSetLayers(0);
                videoSetPseudoHires(1);
            } else {
                videoSetPseudoHires(0);
                colorMathSetLayers(LAYER_BG1 | COLORMATH_BACKDROP);
            }
        }
    }

    return 0;
}
