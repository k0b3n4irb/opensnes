/**
 * @file main.c
 * @brief Two ways to mix two layers: colour addition, and pseudo-hires
 * @ingroup examples
 *
 * Demonstrates the SNES color math unit by blending two background layers.
 * BG1 (main screen) shows a 4bpp rainy street at night, while BG3 (sub
 * screen) holds 2bpp mist that drifts horizontally. The PPU adds the
 * sub screen pixel colors to the main screen via registers CGWSEL ($2130)
 * and CGADSUB ($2131), producing a translucent overlay effect.
 *
 * Mode 1 is used with BG3 priority high so the 2bpp mist layer renders
 * above BG1. Color addition is applied to BG1 and the backdrop, meaning
 * the mist colour is numerically added to whatever is underneath.
 *
 * Press A for the other way the PPU mixes two layers: pseudo-hires (SETINI
 * bit 3). The colour math is switched off, and the PPU interleaves the sub
 * screen (even columns) with the main screen (odd columns) into a 512-pixel
 * line: a 50 % blend that costs no colour math at all, at the price of the
 * horizontal resolution of each layer. A again goes back to addition.
 *
 * This began as the PVSnesLib Transparency example by Alekmaul; the
 * pseudo-hires mode was the pseudo_hires example, merged here. The street
 * and the mist are ours: the street generated, the mist procedural (wisps
 * of a periodic noise, so they wrap at 256 pixels).
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
 * - A rainy street with semi-transparent mist drifting over it
 * - The mist blends additively, lightening the street beneath it
 * - The mist drifts by itself
 * - Press A: the mist is no longer added but interleaved column by column
 *   with the street (pseudo-hires); A again: back to addition
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
 * @brief The street and the mist, converted by the build from res/.
 *
 * A C pointer carries its bank byte, so `dmaCopyVram` / `dmaCopyCGram`
 * take the data wherever the linker placed it.
 */
#include "res/street.inc"
#include "res/mist.inc"
/* generated: street_tiles/_map/_pal (the street), mist_tiles/_map/_pal (each with _end) */

/** @brief VRAM word addresses: the street's 883 tiles fill $0000-$372F */
#define BG1_TILES 0x0000
#define BG3_TILES 0x4000
#define BG1_MAP   0x5000
#define BG3_MAP   0x5400

/**
 * @brief Entry point: color math transparency with drifting mist over a street.
 *
 * Sets up Mode 1 with two background layers:
 * - BG1 (main screen): 4bpp street, static
 * - BG3 (sub screen): 2bpp semi-transparent mist, scrolling horizontally
 *
 * The SNES color math unit adds the sub screen pixel colors to the main screen,
 * producing a translucent mist overlay. BG3 is given priority high so its
 * tiles render above BG1 in Mode 1's layer ordering.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    /** @brief Horizontal scroll offset for the mist layer (BG3), incremented each frame */
    s16 scrX = 0;

    consoleInit();

    /* Force blank (bit 7 of INIDISP) disables rendering, allowing unlimited
     * VRAM/CGRAM write time. Required because the graphics data is too large
     * for a single VBlank DMA transfer (~4KB budget). */
    setScreenOff();

    /* Load BG1 (4bpp street) tiles, tilemap, and palette (palette 1) */
    dmaCopyVram(street_tiles, BG1_TILES, street_tiles_end - street_tiles);
    dmaCopyVram(street_map,   BG1_MAP,   street_map_end   - street_map);
    dmaCopyCGram(street_pal,  16,        street_pal_end   - street_pal);

    /* Load BG3 (2bpp mist) tiles, tilemap, and palette (palette 0) */
    dmaCopyVram(mist_tiles, BG3_TILES, mist_tiles_end - mist_tiles);
    dmaCopyVram(mist_map,   BG3_MAP,   mist_map_end   - mist_map);
    dmaCopyCGram(mist_pal,  0,         mist_pal_end   - mist_pal);

    /* BG1: map at VRAM $5000 (32x32 tiles), tiles at $0000 */
    bgSetMapPtr(0, BG1_MAP, SC_32x32);
    bgSetGfxPtr(0, BG1_TILES);

    /* BG3: map at VRAM $5400, tiles at $4000. BG3 uses 2bpp tiles in
     * Mode 1, half the VRAM per tile of a 4bpp layer. */
    bgSetMapPtr(2, BG3_MAP, SC_32x32);
    bgSetGfxPtr(2, BG3_TILES);

    /* Mode 1 with BG3 priority high. In Mode 1, BG3 is normally 2bpp and
     * drawn behind BG1/BG2. Setting BG3_MODE1_PRIORITY_HIGH promotes BG3's
     * high-priority tiles above all other BG layers, which is essential for
     * the mist overlay to be visible on top of the street. */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    /* Main screen: the layers that form the final displayed image.
     * Both BG1 (street) and BG3 (mist) must be on the main screen
     * for the mist to be visible. */
    setMainScreen(LAYER_BG1 | LAYER_BG3);

    /* Sub screen: the source for color math blending. BG3 on the sub screen
     * provides the color values that will be added to the main screen pixels.
     * Without this, the color math unit has no source data to blend. */
    setSubScreen(LAYER_BG3);

    /* Configure the color math unit for additive blending:
     * - Source = sub screen (BG3 mist colour)
     * - Operation = ADD (mist RGB values added to main screen pixels)
     * - Targets = BG1 + backdrop (both receive the mist colour addition)
     *
     * The backdrop is the "background color" (CGRAM entry 0) visible where
     * no BG tile is drawn. Including it ensures the mist blends even over empty
     * areas. */
    colorMathInit();
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetLayers(LAYER_BG1 | COLORMATH_BACKDROP);

    setScreenOn();

    while (1) {
        /* Scroll BG3 (mist) one pixel right each frame.
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
