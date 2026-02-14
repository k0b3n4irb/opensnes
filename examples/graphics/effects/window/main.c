/**
 * @file main.c
 * @brief Window Masking Example
 *
 * Demonstrates the window module for masking portions of layers.
 * Creates an animated "spotlight" effect using Window 1.
 *
 * Controls:
 *   D-Pad: Move window position
 *   A: Toggle window on/off
 *   B: Toggle inverted mode (show inside vs outside)
 *   L/R: Decrease/increase window width
 */

#include <snes.h>
#include <snes/window.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/*============================================================================
 * State Variables
 *============================================================================*/

static u8 window_x;
static u8 window_half_width;
static u8 window_enabled;
static u8 window_inverted;

static void update_window(void) {
    u8 left;
    u8 right;

    if (!window_enabled) {
        windowDisableAll();
        return;
    }

    /* Calculate window boundaries */
    if (window_x < window_half_width) {
        left = 0;
    } else {
        left = window_x - window_half_width;
    }

    if (window_x + window_half_width > 255) {
        right = 255;
    } else {
        right = window_x + window_half_width;
    }

    /* Set window position */
    windowSetPos(WINDOW_1, left, right);

    /* Enable window for BG1 */
    windowEnable(WINDOW_1, WINDOW_BG1);

    /* Set inversion (0 = mask outside, show inside; 1 = mask inside, show outside) */
    windowSetInvert(WINDOW_1, WINDOW_BG1, window_inverted);

    /* Enable window masking on main screen */
    windowSetMainMask(WINDOW_BG1);
}

int main(void) {
    u16 pad;
    u16 pad_prev;

    /* Initialize state */
    window_x = 128;           /* Center of screen */
    window_half_width = 40;   /* 80 pixel wide window */
    window_enabled = 1;
    window_inverted = 0;
    pad_prev = 0;

    /* Force blank during setup */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /*------------------------------------------------------------------------
     * Configure Background
     *------------------------------------------------------------------------*/

    /* BG1 tilemap at VRAM $1000, 32x32 tiles */
    bgSetMapPtr(0, 0x1000, SC_32x32);

    /* BG1: tiles at $4000, palette at slot 0 */
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);

    /* Load tilemap */
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    bgSetScroll(0, 0, 0);

    /*------------------------------------------------------------------------
     * Initialize Window
     *------------------------------------------------------------------------*/

    windowInit();
    update_window();

    /*------------------------------------------------------------------------
     * Turn on screen
     *------------------------------------------------------------------------*/

    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /*------------------------------------------------------------------------
     * Main loop
     *------------------------------------------------------------------------*/

    while (1) {
        WaitForVBlank();

        /* Read joypad */
        while (REG_HVBJOY & 0x01) {}
        pad = REG_JOY1L | (REG_JOY1H << 8);

        /* Detect new presses */
        u16 pressed = pad & ~pad_prev;

        /* D-Pad Left: move window left */
        if (pad & 0x0200) {  /* Left held */
            if (window_x > 2) {
                window_x -= 2;
                update_window();
            }
        }

        /* D-Pad Right: move window right */
        if (pad & 0x0100) {  /* Right held */
            if (window_x < 253) {
                window_x += 2;
                update_window();
            }
        }

        /* D-Pad Up: move window up - no effect for horizontal window */
        /* D-Pad Down: move window down - no effect for horizontal window */

        /* A button: toggle window on/off */
        if (pressed & 0x0080) {  /* A */
            window_enabled = !window_enabled;
            update_window();
        }

        /* B button: toggle inverted mode */
        if (pressed & 0x8000) {  /* B */
            window_inverted = !window_inverted;
            update_window();
        }

        /* L button: decrease width */
        if (pressed & 0x0020) {  /* L */
            if (window_half_width > 10) {
                window_half_width -= 10;
                update_window();
            }
        }

        /* R button: increase width */
        if (pressed & 0x0010) {  /* R */
            if (window_half_width < 120) {
                window_half_width += 10;
                update_window();
            }
        }

        pad_prev = pad;
    }

    return 0;
}
