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

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/* Wait for a button press (release then press) */
static void wait_for_key(void) {
    while (padHeld(0) != 0) WaitForVBlank();
    while (padHeld(0) == 0) WaitForVBlank();
}

/* Fade out (light to black) */
static void fade_out(u8 speed) {
    s8 brightness;
    u8 i;

    for (brightness = 15; brightness >= 0; brightness--) {
        setBrightness((u8)brightness);

        for (i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
}

/* Fade in (black to light) */
static void fade_in(u8 speed) {
    u8 brightness;
    u8 i;

    for (brightness = 0; brightness <= 15; brightness++) {
        setBrightness(brightness);

        for (i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
}

int main(void) {
    /* Force blank during setup */
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
    REG_TM = TM_BG1;

    /* Set scroll to (0,0) */
    bgSetScroll(0, 0, 0);

    /* Start with screen on */
    setScreenOn();

    /* Wait for first button press */
    wait_for_key();

    /* Main loop - cycle through fade effects */
    while (1) {
        /* Fast fade out (1 frame per step) */
        fade_out(1);
        WaitForVBlank();
        wait_for_key();

        /* Fast fade in */
        fade_in(1);
        WaitForVBlank();
        wait_for_key();

        /* Slow fade out (3 frames per step) */
        fade_out(3);
        WaitForVBlank();
        wait_for_key();

        /* Slow fade in */
        fade_in(3);
        WaitForVBlank();
        wait_for_key();

        /* Very slow fade out (6 frames per step) */
        fade_out(6);
        WaitForVBlank();
        wait_for_key();

        /* Very slow fade in */
        fade_in(6);
        WaitForVBlank();
        wait_for_key();
    }

    return 0;
}
