/**
 * @file main.c
 * @brief Tracker music playback with transport controls via SNESMOD
 * @ingroup examples
 *
 * Plays an Impulse Tracker (.it) module through the SPC700 sound processor
 * using the SNESMOD library. Provides transport controls: play, stop,
 * pause/resume, volume up/down, and fade out.
 *
 * Ported from PVSnesLib "music" example by alekmaul.
 *
 * @par SNES Concepts
 * - SPC700 audio: separate coprocessor with 64 KB audio RAM
 * - SNESMOD workflow: init → setSoundbank → loadModule → play
 * - Per-frame snesmodProcess() to stream pattern data to SPC700
 * - Module volume and fade control
 *
 * @par What to Observe
 * - Music plays immediately on boot
 * - A=play, B=stop, X=pause/resume, L/R=volume, START=fade out
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

/**
 * @brief Entry point — initialize audio, display controls, run transport loop
 * @return Never returns
 */
int main(void) {
    u16 pad;
    u8 volume;
    u8 paused;

    /* Init hardware */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Palette */
    setColor(0, 0x2800);
    setColor(1, RGB(31, 31, 31));

    /* Text setup */
    textInit();
    textLoadFont(0x0000);
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* HUD */
    textPrintAt(5, 2, "SNESMOD MUSIC EXAMPLE");
    textPrintAt(5, 5, "CONTROLS:");
    textPrintAt(7, 7, "A - PLAY MUSIC");
    textPrintAt(7, 8, "B - STOP MUSIC");
    textPrintAt(7, 9, "X - PAUSE/RESUME");
    textPrintAt(7, 10, "L/R - VOLUME DOWN/UP");
    textPrintAt(7, 11, "START - FADE OUT");
    textPrintAt(5, 14, "NOW PLAYING: POLLEN8");

    /* Screen enable */
    setMainScreen(LAYER_BG1);
    textFlush();
    WaitForVBlank();

    /* Initialize SNESMOD */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_POLLEN8);
    snesmodPlay(0);

    setScreenOn();

    /* State */
    volume = 127;
    paused = 0;

    /* Main loop */
    while (1) {
        WaitForVBlank();
        snesmodProcess();

        pad = padPressed(0);

        if (pad & KEY_A) {
            snesmodPlay(0);
            paused = 0;
        }
        if (pad & KEY_B) {
            snesmodStop();
        }
        if (pad & KEY_X) {
            if (paused) { snesmodResume(); paused = 0; }
            else { snesmodPause(); paused = 1; }
        }
        if (pad & KEY_L) {
            if (volume > 10) { volume -= 10; snesmodSetModuleVolume(volume); }
        }
        if (pad & KEY_R) {
            if (volume < 117) { volume += 10; snesmodSetModuleVolume(volume); }
        }
        if (pad & KEY_START) {
            snesmodFadeVolume(0, 4);
        }
    }

    return 0;
}
