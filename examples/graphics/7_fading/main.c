/**
 * @file main.c
 * @brief Fading Effect Example
 *
 * Port of pvsneslib Fading example.
 * Demonstrates screen brightness control for fade in/out effects.
 *
 * The SNES INIDISP register ($2100) controls screen brightness:
 * - Bits 0-3: Brightness level (0-15)
 * - Bit 7: Force blank (1 = screen off)
 *
 * Controls:
 *   Any button: Advance to next fade effect
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/* Wait for a button press */
static void wait_for_key(void) {
    u16 pad;

    /* Wait for all buttons to be released first */
    do {
        WaitForVBlank();
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
    } while (pad != 0 && pad != 0xFFFF);

    /* Now wait for a button press */
    do {
        WaitForVBlank();
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);
    } while (pad == 0 || pad == 0xFFFF);
}

/* Fade out (light to black) */
static void fade_out(u8 speed) {
    s8 brightness;
    u8 i;

    for (brightness = 15; brightness >= 0; brightness--) {
        REG_INIDISP = (u8)brightness;

        /* Wait multiple frames based on speed */
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
        REG_INIDISP = brightness;

        /* Wait multiple frames based on speed */
        for (i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
}

int main(void) {
    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

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
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

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
