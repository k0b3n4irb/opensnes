/**
 * @file main.c
 * @brief HDMA Helpers Demo
 *
 * Demonstrates the 4 HDMA library helper effects:
 *   A: Brightness gradient (fade to black at bottom)
 *   B: Color gradient (sky color shift on BG color 0)
 *   X: Iris wipe (circular window)
 *   Y: Water ripple (increasing distortion top-to-bottom)
 *
 * D-pad: adjust parameters per effect
 * START: stop current effect
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/input.h>
#include <snes/hdma.h>

extern u8 tiles[], tiles_end[];
extern u8 palette[], palette_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 hdma_wave_amplitude;

/* Effect state */
typedef struct {
    u8 active_effect;    /* 0=none, 1=brightness, 2=color, 3=iris, 4=ripple */
    u8 iris_radius;
    u8 brightness_bot;
    u8 ripple_amp;
} EffectState;

EffectState fx = {0, 80, 0, 6};

extern u8 hdma_table_a[];

static void stopCurrentEffect(void) {
    /* Stop ALL HDMA channels unconditionally to avoid leftover HDMA from
     * a previous effect on a different channel (e.g. brightness=ch7
     * still running when switching from A to B which uses ch6). */
    hdmaBrightnessGradientStop(7);  /* Disables ch7, restores INIDISP=0x0F */
    hdmaColorGradientStop(6);       /* Disables ch6 */
    hdmaIrisWipeStop(6);            /* Disables ch6, restores window regs */
    hdmaWaveStop();                 /* Disables wave ch, resets BG scroll */

    /* Re-enable ch6 with a null (terminator) table. This keeps ch6 always
     * enabled so HDMA init at each VBlank properly loads A1T → A2A.
     * Without this, enabling ch6 AFTER init causes one frame of stale
     * A2A reads (horizontal lines artifact). */
    hdma_table_a[0] = 0x00;
    hdmaSetupBank(6, HDMA_MODE_1REG, HDMA_DEST_BG1HOFS, hdma_table_a, 0x00);
    hdmaEnable(1 << 6);

    /* Restore original palette (color gradient modifies CGRAM per-scanline,
     * values persist after HDMA stops). Reload from ROM source.
     * CGRAM DMA must happen during VBlank — doing it during active display
     * causes brief visual artifacts (vertical bars). */
    WaitForVBlank();
    dmaCopyCGram(palette, 0, palette_end - palette);

    fx.active_effect = 0;
}

int main(void) {
    u16 pressed;

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
