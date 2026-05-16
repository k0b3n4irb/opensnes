/**
 * @file main.c
 * @brief Screen fade in/out using INIDISP brightness control
 * @ingroup examples
 *
 * Demonstrates smooth screen fading by stepping through the 16 brightness
 * levels of the SNES INIDISP register ($2100). Bits 0-3 of INIDISP control
 * master brightness from 0 (black) to 15 (full), while bit 7 is the force
 * blank flag. By writing successive brightness values with timed delays
 * between frames, the display fades smoothly to or from black. Three fade
 * speeds are showcased: fast (1 frame/step), medium (3 frames/step), and
 * slow (6 frames/step). This technique is used in virtually every SNES game
 * for scene transitions, title screens, and game-over sequences.
 *
 * @par SNES Concepts
 * - INIDISP register ($2100) brightness levels 0-15
 * - Frame-counted delay loops for animation timing
 * - setBrightness() library wrapper for safe INIDISP writes
 * - Force blank (bit 7) vs brightness dimming (bits 0-3)
 *
 * @par What to Observe
 * - Press any button to advance through fade effects
 * - Fast fade out (instant-feeling), then fast fade in
 * - Medium fade (noticeable transition), then slow fade (cinematic pace)
 * - The cycle repeats: fast, medium, slow, fast, ...
 *
 * @par Modules Used
 * console, sprite, dma, input, background
 *
 * @see video.h, input.h, background.h
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

/** @brief 4bpp tile data for the background image. */
extern u8 tiles[], tiles_end[];
/** @brief Tilemap data (32x32 tile grid) for the background image. */
extern u8 tilemap[], tilemap_end[];
/** @brief 15-bit BGR palette (up to 16 colors) for the background image. */
extern u8 palette[], palette_end[];

/**
 * @brief Block until the user presses a button (with debounce).
 *
 * First waits for all buttons to be released (if any are held), then waits
 * for a new button press. This two-phase approach prevents the same press
 * from being detected twice and ensures clean transition between fade steps.
 * Each iteration calls WaitForVBlank() to yield CPU time and synchronize
 * with the display refresh.
 */
static void wait_for_key(void) {
    while (padHeld(0) != 0) WaitForVBlank();
    while (padHeld(0) == 0) WaitForVBlank();
}

/* Note: the previous example-local `fade_out` / `fade_in` helpers have
 * been promoted to the lib (snes/console.h) as `fadeOut` / `fadeIn`.
 * Use those directly — same signature, same semantics. */

/**
 * @brief Entry point -- cycles through fade-out/fade-in at three speeds.
 *
 * Loads a background image, then enters an interactive loop where each
 * button press triggers the next fade effect: fast (1 frame/step),
 * medium (3 frames/step), and slow (6 frames/step). The cycle repeats
 * indefinitely. This demonstrates how INIDISP brightness control provides
 * smooth scene transitions without any per-pixel computation.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    /* Force blank during setup -- required for safe VRAM writes */
    setScreenOff();

    /*------------------------------------------------------------------------
     * Configure Background Tilemap
     *------------------------------------------------------------------------*/

    /* BG1 tilemap at VRAM $1000, 32x32 tiles */
    bgSetMapPtr(0, 0x1000, SC_32x32);

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palette
     *------------------------------------------------------------------------*/

    /* BG1: tiles at $4000, palette at slot 0 (offset 0) */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);

    /*------------------------------------------------------------------------
     * Load Tilemap Data
     *------------------------------------------------------------------------*/

    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

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

        /* Very slow fade out (6 frames per step) */
        fadeOut(6);
        WaitForVBlank();
        wait_for_key();

        /* Very slow fade in */
        fadeIn(6);
        WaitForVBlank();
        wait_for_key();
    }

    return 0;
}
