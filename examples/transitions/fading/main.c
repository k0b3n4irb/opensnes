/**
 * @file main.c
 * @brief Screen transitions: brightness fades and the mosaic filter
 * @ingroup examples
 *
 * The two transitions the PPU gives for free. A fade steps the master
 * brightness of INIDISP ($2100) from 15 (full) to 0 (black) and back; the
 * mosaic filter (MOSAIC, $2106) fills N x N blocks of a background with
 * the colour of their top-left pixel, from 1 x 1 up to 16 x 16. Three fade
 * speeds are shown, then the mosaic growing and shrinking back. Neither
 * costs CPU time beyond one register write per step.
 *
 * @par SNES Concepts
 * - INIDISP ($2100) brightness levels 0-15; brightness 0 is not force blank
 * - MOSAIC ($2106): block size in bits 7-4, the BG layers it applies to in bits 3-0
 * - fadeOut() / fadeIn(): one brightness step every `speed` frames, the program held meanwhile
 * - fadeStart() / fadeStep(): the same fade, a step per frame, the loop free
 * - mosaicSetLayers() / mosaicFadeOut() / mosaicFadeIn() / mosaicDisable()
 * - Sprites are never mosaicked: only background layers are
 *
 * @par What to Observe
 * - Press any button to advance to the next transition
 * - Fast fade out and in (1 frame per step), then 3 frames per step, then 6
 *   (the 6-frame fades do not hold the program: the loop counts its frames
 *   in `fade_ticks` while they run)
 * - Then the mosaic: the picture breaks into growing blocks, then sharpens again
 * - The cycle repeats
 *
 * @par Modules Used
 * console, sprite, dma, input, background, mosaic
 *
 * @see video.h, mosaic.h, input.h, background.h
 */

#include <snes.h>
#include "vram_map.h"  /* generated VRAM bases (devtools/vram_layout) */

/*============================================================================
 * Graphics Data (converted by the build from res/, one settings file beside each picture)
 *============================================================================*/

/** @brief 4bpp tile data for the background image. */
#include "res/moonrise.inc"   /* generated: moonrise_tiles[], moonrise_map[], moonrise_pal[], each with _end */
/** @brief Tilemap data (32x32 tile grid) for the background image. */
/** @brief 15-bit BGR palette (up to 16 colors) for the background image. */

/**
 * @brief Block until the user presses a button (with debounce).
 *
 * First waits for all buttons to be released (if any are held), then waits
 * for a new button press. This two-phase approach prevents the same press
 * from being detected twice and ensures clean transition between fade steps.
 * Each iteration calls WaitForVBlank() to yield CPU time and synchronize
 * with the display refresh.
 */
/** @brief Frames the main loop ran during the non-blocking fades (96 a fade at 6 frames per step). */
u16 fade_ticks;

static void wait_for_key(void) {
    while (padHeld(0) != 0) WaitForVBlank();
    while (padHeld(0) == 0) WaitForVBlank();
}

/* Note: the previous example-local `fade_out` / `fade_in` helpers have
 * been promoted to the lib (snes/console.h) as `fadeOut` / `fadeIn`.
 * Use those directly — same signature, same semantics. */

/**
 * @brief Entry point -- cycles through the fades at three speeds, then the mosaic.
 *
 * Loads a background image, then enters an interactive loop where each
 * button press triggers the next transition: fade out and in at 1, 3 and
 * 6 frames per step, then the mosaic growing to 16 x 16 blocks and
 * shrinking back. The cycle repeats indefinitely.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    consoleInit();

    /* Force blank during setup -- required for safe VRAM writes */
    setScreenOff();

    /*------------------------------------------------------------------------
     * Configure Background Tilemap
     *------------------------------------------------------------------------*/

    /* BG1 tilemap at VRAM $1000, 32x32 tiles */
    bgSetMapPtr(0, VRAM_BG_MAP, SC_32x32);

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palette
     *------------------------------------------------------------------------*/

    /* BG1: tiles at $4000, palette at slot 0 (offset 0) */
    bgInitTileSet(0, moonrise_tiles, moonrise_pal, 0,
                  moonrise_tiles_end - moonrise_tiles,
                  moonrise_pal_end - moonrise_pal,
                  BG_16COLORS, VRAM_BG_TILES);

    /*------------------------------------------------------------------------
     * Load Tilemap Data
     *------------------------------------------------------------------------*/

    dmaCopyVram(moonrise_map, VRAM_BG_MAP, moonrise_map_end - moonrise_map);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/

    /* Set Mode 1 */
    setMode(BG_MODE1, 0);

    /* Enable only BG1 on main screen */
    setMainScreen(TM_BG1);

    /* Set scroll to (0,0) */
    bgSetScroll(0, 0, 0);

    /* Start with screen on */
    setScreenOn();

    /* Wait for first button press */
    wait_for_key();

    /* Main loop - cycle through fade effects */
    while (1) {
        /* Fast fade out (1 frame per step) */
        fadeOut(1);
        WaitForVBlank();
        wait_for_key();

        /* Fast fade in */
        fadeIn(1);
        WaitForVBlank();
        wait_for_key();

        /* Slow fade out (3 frames per step) */
        fadeOut(3);
        WaitForVBlank();
        wait_for_key();

        /* Slow fade in */
        fadeIn(3);
        WaitForVBlank();
        wait_for_key();

        /* Very slow fade out (6 frames per step), without holding the program:
         * fadeStart() writes the first level, then the loop goes on a frame at a
         * time and fadeStep() makes the others. What a game does each frame (its
         * sound driver, its animation) takes the place of `fade_ticks++`. */
        fade_ticks = 0;
        fadeStart(FADE_OUT, 6);
        do {
            WaitForVBlank();
            fade_ticks++;
        } while (fadeStep());
        WaitForVBlank();
        wait_for_key();

        /* Very slow fade in, the same way */
        fadeStart(FADE_IN, 6);
        do {
            WaitForVBlank();
            fade_ticks++;
        } while (fadeStep());
        WaitForVBlank();
        wait_for_key();

        /* Mosaic out: BG1's blocks grow from 1x1 to 16x16, 3 frames per size */
        mosaicSetLayers(LAYER_BG1);
        mosaicFadeOut(3);
        WaitForVBlank();
        wait_for_key();

        /* Mosaic in: the blocks shrink back, then the filter is switched off */
        mosaicFadeIn(3);
        mosaicDisable();
        WaitForVBlank();
        wait_for_key();
    }

    return 0;
}
