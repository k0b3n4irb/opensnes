/**
 * @file main.c
 * @brief Color Math Transparency Example
 *
 * Demonstrates the color math module for transparency and tinting effects.
 *
 * Controls:
 *   D-Pad Up/Down: Increase/decrease shadow intensity
 *   A: Toggle shadow effect on/off
 *   B: Cycle through color tints (red, green, blue, none)
 */

#include <snes.h>
#include <snes/colormath.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

/*============================================================================
 * State Variables
 *============================================================================*/

static u8 shadow_intensity;
static u8 shadow_enabled;
static u8 tint_mode;  /* 0=none, 1=red, 2=green, 3=blue */

static void apply_effect(void) {
    if (shadow_enabled) {
        /* Apply shadow (darken) effect */
        colorMathShadow(COLORMATH_ALL, shadow_intensity);
    } else if (tint_mode > 0) {
        /* Apply color tint */
        colorMathEnable(COLORMATH_ALL);
        colorMathSetOp(COLORMATH_ADD);
        colorMathSetHalf(0);
        colorMathSetSource(COLORMATH_SRC_FIXED);

        switch (tint_mode) {
            case 1: /* Red tint */
                colorMathSetFixedColor(12, 0, 0);
                break;
            case 2: /* Green tint */
                colorMathSetFixedColor(0, 12, 0);
                break;
            case 3: /* Blue tint */
                colorMathSetFixedColor(0, 0, 12);
                break;
        }
    } else {
        /* No effect */
        colorMathDisable();
    }
}

int main(void) {
    u16 pad;
    u16 pad_prev;

    /* Initialize state */
    shadow_intensity = 8;
    shadow_enabled = 0;
    tint_mode = 0;
    pad_prev = 0;

    /* Force blank during setup */
    setScreenOff();

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
     * Initialize Color Math
     *------------------------------------------------------------------------*/

    colorMathInit();

    /*------------------------------------------------------------------------
     * Turn on screen
     *------------------------------------------------------------------------*/

    setScreenOn();

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

        /* A button: toggle shadow */
        if (pressed & 0x0080) {  /* A button */
            shadow_enabled = !shadow_enabled;
            if (shadow_enabled) {
                tint_mode = 0;  /* Disable tint when enabling shadow */
            }
            apply_effect();
        }

        /* B button: cycle tint */
        if (pressed & 0x8000) {  /* B button */
            if (!shadow_enabled) {
                tint_mode = (tint_mode + 1) % 4;
                apply_effect();
            }
        }

        /* D-Pad Up: increase shadow */
        if (pressed & 0x0800) {  /* Up */
            if (shadow_intensity < 31) {
                shadow_intensity++;
                if (shadow_enabled) {
                    apply_effect();
                }
            }
        }

        /* D-Pad Down: decrease shadow */
        if (pressed & 0x0400) {  /* Down */
            if (shadow_intensity > 0) {
                shadow_intensity--;
                if (shadow_enabled) {
                    apply_effect();
                }
            }
        }

        pad_prev = pad;
    }

    return 0;
}
