/**
 * @file main.c
 * @brief Tracker music with transport controls, and sound effects over it, via SNESMOD
 * @ingroup examples
 *
 * Plays an Impulse Tracker (.it) module through the SPC700 sound processor
 * using the SNESMOD library, with transport controls (play, stop,
 * pause/resume, volume, fade out), and fires sound effects over the music.
 *
 * The soundbank holds two modules: the music (pollen8) and a module that is
 * only a set of instruments (effectssfx), whose samples become the sound
 * effects. Effects are uploaded into the SPC700's RAM after the module, in
 * the room the module and its echo buffer leave: pollen8 keeps 13 506 bytes
 * free (`opensnes-music inspect music/pollen8.it`), enough for four of the
 * five effect samples. The fifth, the 8 740-byte "tada", would not fit —
 * and the driver does not check: an effect past the room is copied over the
 * echo buffer, which the echo then overwrites. So this example loads four.
 *
 * Ported from PVSnesLib's "music" and "effects" examples by alekmaul.
 *
 * @par SNES Concepts
 * - SPC700 audio: separate coprocessor with 64 KB audio RAM
 * - SNESMOD workflow: init → setSoundbank → loadModule → play
 * - Per-frame snesmodProcess(): sends the queued commands (play, stop,
 *   volume, sound effects) to the SPC700, which plays the patterns itself
 * - Module volume and fade control
 * - Sound effects: snesmodLoadEffect() uploads a sample, snesmodPlayEffect()
 *   plays it on a free voice, with volume, pan and pitch
 * - SPC RAM is shared: module, samples, echo buffer and effects
 *
 * @par What to Observe
 * - Music plays immediately on boot
 * - A=play, B=stop, X=pause/resume, L/R=volume, START=fade out
 * - LEFT/RIGHT choose an effect, UP/DOWN its pitch, Y plays it over the music
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

/** @brief Number of effects loaded: the four that fit beside the music */
#define NUM_EFFECTS 4

/** @brief The effects' sources in the soundbank, in the order they are loaded */
static const u16 effect_source[NUM_EFFECTS] = {
    SFX_HALL_STRINGS, SFX_HONKY_TONK_PIANO, SFX_MARIMBA_1, SFX_COWBELL
};

/** @brief Their names on screen, padded to the same width */
static const char *const effect_name[NUM_EFFECTS] = {
    "STRINGS", "PIANO  ", "MARIMBA", "COWBELL"
};

/** @brief The three pitches of snesmodPlayEffect(), and their names */
static const u8 pitch_value[3] = {
    SNESMOD_PITCH_LOW, SNESMOD_PITCH_NORMAL, SNESMOD_PITCH_HIGH
};
static const char *const pitch_name[3] = { "LOW   ", "NORMAL", "HIGH  " };

/**
 * @brief Entry point — initialize audio, display controls, run transport loop
 * @return Never returns
 */
int main(void) {
    u16 pad;
    u8 volume;
    u8 paused;
    u8 slot[NUM_EFFECTS];
    u8 effect;
    u8 pitch;
    u8 i;

    /* Init hardware + text */
    textModeInit();
    setColor(0, RGB(0, 0, 10));

    /* HUD */
    textPrintAt(5, 2, "SNESMOD MUSIC EXAMPLE");
    textPrintAt(5, 4, "CONTROLS:");
    textPrintAt(7, 6, "A - PLAY MUSIC");
    textPrintAt(7, 7, "B - STOP MUSIC");
    textPrintAt(7, 8, "X - PAUSE/RESUME");
    textPrintAt(7, 9, "L/R - VOLUME DOWN/UP");
    textPrintAt(7, 10, "START - FADE OUT");
    textPrintAt(7, 12, "Y - PLAY EFFECT");
    textPrintAt(7, 13, "LEFT/RIGHT - EFFECT");
    textPrintAt(7, 14, "UP/DOWN - PITCH");
    textPrintAt(5, 17, "NOW PLAYING: POLLEN8");
    textPrintAt(5, 19, "EFFECT: STRINGS");
    textPrintAt(5, 20, "PITCH:  NORMAL");
    WaitForVBlank();

    /* Initialize SNESMOD */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_POLLEN8);

    /* The effects go after the module: load them after it, and keep the
     * slot each one gets — snesmodPlayEffect() takes the slot, not the
     * source number. */
    for (i = 0; i < NUM_EFFECTS; i++) {
        slot[i] = snesmodLoadEffect(effect_source[i]);
    }

    snesmodPlay(0);

    setScreenOn();

    /* State */
    volume = 255;   /* the driver's own start value: 0-255 */
    paused = 0;
    effect = 0;
    pitch = 1;      /* normal */

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
            if (volume < 245) { volume += 10; snesmodSetModuleVolume(volume); }
        }
        if (pad & KEY_START) {
            snesmodFadeVolume(0, 4);
        }

        /* Effects: choose one, set its pitch, play it over the music */
        if ((pad & KEY_LEFT) && effect > 0) {
            effect--;
            textPrintAt(13, 19, effect_name[effect]);
        }
        if ((pad & KEY_RIGHT) && effect < NUM_EFFECTS - 1) {
            effect++;
            textPrintAt(13, 19, effect_name[effect]);
        }
        if ((pad & KEY_DOWN) && pitch > 0) {
            pitch--;
            textPrintAt(13, 20, pitch_name[pitch]);
        }
        if ((pad & KEY_UP) && pitch < 2) {
            pitch++;
            textPrintAt(13, 20, pitch_name[pitch]);
        }
        if (pad & KEY_Y) {
            snesmodPlayEffect(slot[effect], 127, 128, pitch_value[pitch]);
        }
    }

    return 0;
}
