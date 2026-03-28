/**
 * @file main.c
 * @brief Sound effect playback with pitch control via SNESMOD
 * @ingroup examples
 *
 * Demonstrates standalone sound effect playback through the SPC700 using
 * SNESMOD, without any background music. Five instrument samples are loaded
 * from a soundbank at startup. Each button triggers a different effect, and
 * the D-pad left/right cycles between three pitch settings.
 *
 * Ported from PVSnesLib "effects" example by alekmaul.
 *
 * @par SNES Concepts
 * - SPC700 effect playback: snesmodLoadEffect() + snesmodPlayEffect()
 * - Pitch control: SNESMOD_PITCH_LOW / NORMAL / HIGH
 * - Effects-only soundbank: .it file with instrument samples, no patterns
 * - Per-frame snesmodProcess() required even without music
 *
 * @par What to Observe
 * - A/B/X/Y play different instrument samples
 * - L/R play cowbell
 * - LEFT/RIGHT change pitch (shown on screen)
 *
 * @par Modules Used
 * console, sprite, dma, input, background, text
 *
 * @see snesmod.h
 */

#include <snes.h>
#include <snes/snesmod.h>
#include <snes/text.h>
#include "soundbank.h"

/** @brief Number of sound effects in the soundbank */
#define NUM_EFFECTS 5

/**
 * @brief Entry point — load effects, display controls, play on button press
 * @return Never returns
 */
int main(void) {
    u16 pad;
    u8 pitch_mode;
    u16 pitch;
    u16 i;

    /* Init hardware + text */
    textModeInit();
    setColor(0, RGB(0, 0, 10));

    /* HUD */
    textPrintAt(6, 2, "SNESMOD SFX EXAMPLE");
    textPrintAt(5, 5, "CONTROLS:");
    textPrintAt(7, 7, "A - TADA");
    textPrintAt(7, 8, "B - STRINGS");
    textPrintAt(7, 9, "X - PIANO");
    textPrintAt(7, 10, "Y - MARIMBA");
    textPrintAt(7, 11, "L/R - COWBELL");
    textPrintAt(7, 12, "LEFT/RIGHT - PITCH");
    textPrintAt(7, 15, "PITCH: NORMAL   ");
    textFlush();
    WaitForVBlank();

    /* Initialize SNESMOD */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);

    /* Load all sound effects */
    for (i = 0; i < NUM_EFFECTS; i++) {
        snesmodLoadEffect(i);
    }

    setScreenOn();

    /* State */
    pitch_mode = 1;
    pitch = SNESMOD_PITCH_NORMAL;

    /* Main loop */
    while (1) {
        WaitForVBlank();
        snesmodProcess();

        pad = padPressed(0);

        /* Change pitch with Left/Right */
        if (pad & KEY_LEFT) {
            if (pitch_mode > 0) {
                pitch_mode--;
                if (pitch_mode == 0) {
                    pitch = SNESMOD_PITCH_LOW;
                    textPrintAt(7, 15, "PITCH: LOW      ");
                } else {
                    pitch = SNESMOD_PITCH_NORMAL;
                    textPrintAt(7, 15, "PITCH: NORMAL   ");
                }
                textFlush();
            }
        }
        if (pad & KEY_RIGHT) {
            if (pitch_mode < 2) {
                pitch_mode++;
                if (pitch_mode == 1) {
                    pitch = SNESMOD_PITCH_NORMAL;
                    textPrintAt(7, 15, "PITCH: NORMAL   ");
                } else {
                    pitch = SNESMOD_PITCH_HIGH;
                    textPrintAt(7, 15, "PITCH: HIGH     ");
                }
                textFlush();
            }
        }

        /* Play effects on button press */
        if (pad & KEY_A) {
            snesmodPlayEffect(SFX_TADA, 127, 128, pitch);
        }
        if (pad & KEY_B) {
            snesmodPlayEffect(SFX_HALL_STRINGS, 127, 128, pitch);
        }
        if (pad & KEY_X) {
            snesmodPlayEffect(SFX_HONKY_TONK_PIANO, 127, 128, pitch);
        }
        if (pad & KEY_Y) {
            snesmodPlayEffect(SFX_MARIMBA_1, 127, 128, pitch);
        }
        if ((pad & KEY_L) || (pad & KEY_R)) {
            snesmodPlayEffect(SFX_COWBELL, 127, 128, pitch);
        }
    }

    return 0;
}
