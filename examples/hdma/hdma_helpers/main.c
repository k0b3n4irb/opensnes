/**
 * @file main.c
 * @brief Interactive showcase of all four HDMA library helper effects
 * @ingroup examples
 *
 * Demonstrates the four high-level HDMA helper functions provided by the
 * OpenSNES library: brightness gradient, color gradient, iris wipe, and
 * water ripple. Each effect uses one or two HDMA channels to modify PPU
 * registers per-scanline without CPU intervention. The brightness gradient
 * writes INIDISP ($2100), the color gradient rewrites CGRAM color 0 via
 * CGADD/CGDATA, the iris wipe programs window registers (WH0/WH1) with
 * TMW masking, and the water ripple offsets BG1HOFS per scanline using
 * a sine-based displacement table. Effects can be toggled and their
 * parameters adjusted in real time using the controller.
 *
 * @par SNES Concepts
 * - HDMA brightness gradient (INIDISP per-scanline dimming)
 * - HDMA color gradient (CGRAM color 0 rewrite per scanline group)
 * - HDMA iris wipe (window registers WH0/WH1 + TMW masking)
 * - HDMA water ripple (BG1HOFS sine displacement per scanline)
 * - Safe HDMA channel management (stop previous before starting new)
 *
 * @par What to Observe
 * - Press A: brightness gradient (screen fades to black at bottom)
 * - Press B: color gradient (backdrop shifts from deep blue to orange)
 * - Press X: iris wipe (circular window reveals the background)
 * - Press Y: water ripple (sinusoidal horizontal distortion animates)
 * - D-pad UP/DOWN: adjust the active effect's parameter (radius, amplitude, etc.)
 * - START: stop the current effect and restore normal display
 *
 * @par Modules Used
 * console, sprite, dma, input, background, hdma
 *
 * @see hdma.h, input.h, background.h, video.h
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/input.h>
#include <snes/hdma.h>

/** @brief 4bpp tile data for the background image. */
extern u8 tiles[], tiles_end[];
/** @brief 15-bit BGR palette for the background (up to 16 colors). */
extern u8 palette[], palette_end[];
/** @brief Tilemap data (32x32 tile grid) for the background. */
extern u8 tilemap[], tilemap_end[];

/**
 * @brief Current wave amplitude for the water ripple effect (library variable).
 *
 * This variable is defined in the HDMA library module. Writing to it adjusts
 * the horizontal displacement amplitude of the sine-wave distortion applied
 * per-scanline to BG1HOFS. Valid range: 2-24 pixels. The library reads this
 * value each frame in hdmaWaveUpdate() to scale the sine table entries.
 */
extern u8 hdma_wave_amplitude;

/**
 * @brief Tracks which HDMA effect is currently active and its parameters.
 *
 * Only one effect runs at a time. Switching effects requires stopping the
 * previous one first (via stopCurrentEffect()) to avoid HDMA channel conflicts
 * and stale register values.
 */
typedef struct {
    u8 active_effect;    /**< Currently active effect: 0=none, 1=brightness, 2=color, 3=iris, 4=ripple */
    u8 iris_radius;      /**< Iris wipe circle radius in pixels (10-200) */
    u8 brightness_bot;   /**< Bottom brightness for gradient (0=black, 14=nearly full) */
    u8 ripple_amp;       /**< Water ripple amplitude in pixels (2-24) */
} EffectState;

/** @brief Global effect state instance with default values. */
EffectState fx = {0, 80, 0, 6};

/**
 * @brief Scratch HDMA table used for channel 6 null-terminator setup.
 *
 * Defined in the HDMA library module. When an effect is stopped, this table
 * is loaded with a single 0x00 terminator and assigned to channel 6 to keep
 * the channel "alive" for proper HDMA initialization at each VBlank.
 */
extern u8 hdma_table_a[];

/**
 * @brief Stop whichever HDMA effect is currently running and restore defaults.
 *
 * This function performs a complete cleanup:
 * 1. Stops ALL HDMA helper channels (ch6 and ch7) unconditionally, because
 *    different effects use different channels and a previous effect's channel
 *    may still be running when switching.
 * 2. Re-enables ch6 with a null (terminator-only) table. This is critical
 *    because the SNES HDMA controller initializes table pointers (A1T -> A2A)
 *    only for channels that are enabled at VBlank. If ch6 is disabled and
 *    then enabled mid-frame, the first frame reads stale A2A data, causing
 *    a visible one-frame glitch (horizontal lines).
 * 3. Reloads the original palette from ROM, because the color gradient effect
 *    modifies CGRAM per-scanline and those values persist after HDMA stops.
 *    CGRAM DMA must happen during VBlank to avoid visual artifacts.
 */
static void stopCurrentEffect(void) {
    hdmaBrightnessGradientStop(7);  /* Disables ch7, restores INIDISP=0x0F */
    hdmaColorGradientStop(6);       /* Disables ch6 */
    hdmaIrisWipeStop(6);            /* Disables ch6, restores window regs */
    hdmaWaveStop();                 /* Disables wave ch, resets BG scroll */

    /* Re-enable ch6 with a null (terminator) table to keep HDMA init happy */
    hdma_table_a[0] = 0x00;
    hdmaSetupBank(6, HDMA_MODE_1REG, HDMA_DEST_BG1HOFS, hdma_table_a, 0x00);
    hdmaEnable(1 << 6);

    /* Restore original palette from ROM source during VBlank */
    WaitForVBlank();
    dmaCopyCGram(palette, 0, palette_end - palette);

    fx.active_effect = 0;
}

/**
 * @brief Entry point -- interactive showcase of four HDMA helper effects.
 *
 * Loads a background image and enters an interactive loop where each button
 * activates a different HDMA effect:
 * - A: brightness gradient (INIDISP per-scanline dimming)
 * - B: color gradient (CGRAM color 0 rewrite per scanline group)
 * - X: iris wipe (window registers WH0/WH1 with TMW masking)
 * - Y: water ripple (BG1HOFS sine displacement per scanline)
 * - D-pad UP/DOWN: adjust the active effect's parameter
 * - START: stop the current effect and restore normal display
 *
 * Before activating a new effect, the previous one is always stopped first
 * to avoid HDMA channel conflicts and stale PPU register values.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    u16 pressed;     /**< Newly pressed buttons this frame (edge-detected) */

    consoleInit();
    setScreenOff();

    /* Load background: tiles at $0000, tilemap at $1000 (no overlap) */
    bgSetMapPtr(0, 0x1000, SC_32x32);
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x0000);
    dmaCopyVram(tilemap, 0x1000, tilemap_end - tilemap);

    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1);
    setScreenOn();

    /* Pre-enable HDMA ch6 with a null table so it's initialized at
     * every VBlank HDMA init. Prevents stale A2A on first effect use. */
    hdma_table_a[0] = 0x00;
    hdmaSetupBank(6, HDMA_MODE_1REG, HDMA_DEST_BG1HOFS, hdma_table_a, 0x00);
    hdmaEnable(1 << 6);

    while (1) {
        WaitForVBlank();

        /* Animate ripple if active */
        if (fx.active_effect == 4) {
            hdmaWaveUpdate();
        }

        pressed = padPressed(0);

        /* START: stop current effect */
        if (pressed & KEY_START) {
            stopCurrentEffect();
        }

        /* A: Brightness gradient */
        if (pressed & KEY_A) {
            if (fx.active_effect != 1) {
                stopCurrentEffect();
            }
            fx.brightness_bot = 0;
            WaitForVBlank();
            hdmaBrightnessGradient(7, 15, fx.brightness_bot);
            fx.active_effect = 1;
        }

        /* B: Color gradient on color 0 */
        if (pressed & KEY_B) {
            if (fx.active_effect != 2) {
                stopCurrentEffect();
            }
            WaitForVBlank();
            hdmaColorGradient(6, 0,
                              RGB(4, 8, 28),   /* Deep blue top */
                              RGB(28, 16, 4));  /* Orange bottom */
            fx.active_effect = 2;
        }

        /* X: Iris wipe */
        if (pressed & KEY_X) {
            if (fx.active_effect != 3) {
                stopCurrentEffect();
            }
            fx.iris_radius = 80;
            WaitForVBlank();
            hdmaIrisWipe(6, TM_BG1, 128, 112, fx.iris_radius);
            fx.active_effect = 3;
        }

        /* Y: Water ripple */
        if (pressed & KEY_Y) {
            if (fx.active_effect != 4) {
                stopCurrentEffect();
            }
            fx.ripple_amp = 6;
            WaitForVBlank();
            hdmaWaterRipple(6, 0, fx.ripple_amp, 2);
            fx.active_effect = 4;
        }

        /* D-pad adjustments */
        if (pressed & KEY_UP) {
            if (fx.active_effect == 1 && fx.brightness_bot < 14) {
                fx.brightness_bot += 2;
                WaitForVBlank();
                hdmaBrightnessGradient(7, 15, fx.brightness_bot);
            }
            if (fx.active_effect == 3 && fx.iris_radius < 200) {
                fx.iris_radius += 10;
                WaitForVBlank();
                hdmaIrisWipe(6, TM_BG1, 128, 112, fx.iris_radius);
            }
            if (fx.active_effect == 4 && fx.ripple_amp < 24) {
                fx.ripple_amp += 2;
                hdma_wave_amplitude = fx.ripple_amp;
            }
        }
        if (pressed & KEY_DOWN) {
            if (fx.active_effect == 1 && fx.brightness_bot > 0) {
                fx.brightness_bot -= 2;
                WaitForVBlank();
                hdmaBrightnessGradient(7, 15, fx.brightness_bot);
            }
            if (fx.active_effect == 3 && fx.iris_radius > 10) {
                fx.iris_radius -= 10;
                WaitForVBlank();
                hdmaIrisWipe(6, TM_BG1, 128, 112, fx.iris_radius);
            }
            if (fx.active_effect == 4 && fx.ripple_amp > 2) {
                fx.ripple_amp -= 2;
                hdma_wave_amplitude = fx.ripple_amp;
            }
        }
    }

    return 0;
}
