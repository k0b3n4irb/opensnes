/**
 * @file main.c
 * @brief Sound effects the way a game engine drives them: voices by number
 * @ingroup examples
 *
 * Three recipes a game-side sound player is built from, each measured by a
 * luna manifest, with the audio module as it is (no extra layer):
 *
 * 1. **A looped waveform whose pitch moves every frame** - a 64-sample cycle
 *    loops on voice 2 and audioSetVoiceVolumePitch() sends the note each frame.
 * 2. **Stop, then play, on the same voice inside one frame** - the "last
 *    arrival cuts the sound in progress" rule: audioStopVoice() followed by
 *    audioPlaySampleOn() on the same voice, with no frame in between.
 * 3. **A sample loaded in pieces** - audioLoadSampleBegin() then a small
 *    audioLoadSampleStep() per frame while the game keeps running.
 *
 * A fourth block times seven voices updated every frame. Each block counts
 * the scanlines it used with profileScanlineStart()/End() (1 line = 1364
 * master cycles) so the cost is read from RAM, not guessed.
 *
 * Nothing to press: the phases run on their own and the backdrop colour
 * names the phase.
 *
 * @par SNES Concepts
 * - Looped one-cycle waveform: pitch is the note
 * - Voice ownership: the game picks the voice, the module just obeys
 * - Piecewise upload over the APU ports: bounded cost per frame
 *
 * @par What to Observe
 * Backdrop: blue while loading, green during the waveform sweep, yellow for
 * the restarts, red for the seven-voice block, then grey and silence.
 *
 * @par Modules Used
 * console, dma, audio, profile
 *
 * @see lib/include/snes/audio.h
 * @see docs/tutorials/audio_samples.md
 */

#include <snes.h>
#include <snes/audio.h>
#include <snes/profile.h>

#include "res/wave.h"

/** @brief Filler samples (data.asm): fixed byte patterns, never played */
extern u8 fill_a[], fill_a_end[];
extern u8 fill_b[], fill_b_end[];
extern u8 fill_c[], fill_c_end[];

/** @brief Sample slots: the waveform, then the three loaded in pieces */
#define SMP_WAVE   0
#define SMP_FILL_A 1
#define SMP_FILL_B 2
#define SMP_FILL_C 3

/** @brief Voice of the pitch sweep and voice of the restarts */
#define VOICE_SWEEP   2
#define VOICE_RESTART 3

/** @brief Bytes sent per frame while loading in pieces */
#define LOAD_STEP 192

/** @brief Phases, written to `phase` so a manifest knows where the run is */
#define PH_LOAD    1
#define PH_SWEEP   2
#define PH_RESTART 3
#define PH_SEVEN   4
#define PH_DONE    5

/** @brief Oracles (WRAM): read by testing/manifests/audio_sfx_voices.toml */
u8  phase;
u16 frames_in_phase;
u16 load_frames;       /**< frames from the first Begin to the last Step */
u16 load_lines_max;    /**< worst scanlines of one audioLoadSampleStep() */
u16 free_before;       /**< audioGetFreeMemory() before loading in pieces */
u16 free_after;        /**< ...and after the three samples are whole */
u16 sweep_pitch;       /**< last pitch sent by the sweep */
u16 sweep_lines_max;   /**< worst scanlines of one pitch+volume update */
u8  restarts_ok;       /**< stop+play pairs that answered the right voice */
u16 restart_pitch;     /**< pitch of the last restart */
u16 restart_lines_max; /**< worst scanlines of one stop+play pair */
u16 seven_lines_max;   /**< worst scanlines of seven voice updates */
u16 seven_lines_sum;   /**< their sum over seven_frames */
u16 seven_frames;      /**< frames measured in the seven-voice block */
u16 idle_lines_max;    /**< the same loop with nothing sent: the floor */

/** @brief Backdrop colour per phase, so the run can be followed by eye */
static void tint(u16 c) {
    setColor(0, c);
}

/** @brief Wait one frame and count it into the current phase */
static void tick(void) {
    WaitForVBlank();
    frames_in_phase++;
}

/** @brief Start a phase: set the oracle, zero its frame counter */
static void enter(u8 p, u16 c) {
    phase = p;
    frames_in_phase = 0;
    tint(c);
}

/** @brief Load one filler sample in pieces, one step per frame */
static void load_in_pieces(u8 slot, u8 *data, u8 *end) {
    u16 left;
    u16 lines;

    audioLoadSampleBegin(slot, data, (u16)(end - data), 0);
    do {
        tick();
        load_frames++;
        profileScanlineStart();
        left = audioLoadSampleStep(LOAD_STEP);
        lines = profileScanlineEnd();
        if (lines > load_lines_max) {
            load_lines_max = lines;
        }
    } while (left != 0 && left != AUDIO_LOAD_FAILED);
}

/** @brief The seven-voice update, one command per voice */
static void update_voices(u8 count, u16 pitch) {
    u8 v;
    for (v = 0; v < count; v++) {
        audioSetVoiceVolumePitch(v, 90, pitch + v * 16);
    }
}

int main(void) {
    u16 i;
    u16 lines;
    u8 v;

    consoleInit();
    dmaClearVRAM();
    audioInit();
    audioLoadSample(SMP_WAVE, wave_brr, (u16)(wave_brr_end - wave_brr), wave_brr_loop);
    for (v = 0; v < AUDIO_MAX_VOICES; v++) {
        /* instant attack, no decay, full sustain: the note holds until key-off */
        audioSetADSR(v, AUDIO_ATTACK_INSTANT, 0, 7, 0);
    }
    tint(RGB(2, 2, 6));
    setScreenOn();

    /* 3. Piecewise load: three samples, 192 bytes a frame. */
    enter(PH_LOAD, RGB(2, 4, 16));
    free_before = audioGetFreeMemory();
    load_in_pieces(SMP_FILL_A, fill_a, fill_a_end);
    load_in_pieces(SMP_FILL_B, fill_b, fill_b_end);
    load_in_pieces(SMP_FILL_C, fill_c, fill_c_end);
    free_after = audioGetFreeMemory();

    /* 1. Looped waveform, pitch changes every frame for 120 frames. */
    enter(PH_SWEEP, RGB(2, 14, 4));
    audioPlaySampleOn(VOICE_SWEEP, SMP_WAVE, 90, AUDIO_PAN_CENTER, 0x0800);
    for (i = 0; i < 120; i++) {
        tick();
        sweep_pitch = 0x0800 + i * 16;
        profileScanlineStart();
        audioSetVoiceVolumePitch(VOICE_SWEEP, 90, sweep_pitch);
        lines = profileScanlineEnd();
        if (lines > sweep_lines_max) {
            sweep_lines_max = lines;
        }
    }
    audioStopVoice(VOICE_SWEEP);

    /* 2. Stop + play on one voice inside one frame, sixteen frames running. */
    enter(PH_RESTART, RGB(16, 14, 2));
    audioPlaySampleOn(VOICE_RESTART, SMP_WAVE, 90, AUDIO_PAN_CENTER, 0x0800);
    for (i = 0; i < 16; i++) {
        tick();
        restart_pitch = (i & 1) ? 0x0800 : 0x1400;
        profileScanlineStart();
        audioStopVoice(VOICE_RESTART);
        v = audioPlaySampleOn(VOICE_RESTART, SMP_WAVE, 90, AUDIO_PAN_CENTER, restart_pitch);
        lines = profileScanlineEnd();
        if (v == VOICE_RESTART) {
            restarts_ok++;
        }
        if (lines > restart_lines_max) {
            restart_lines_max = lines;
        }
    }
    audioStopVoice(VOICE_RESTART);

    /* Floor: the loop with nothing sent, for the cost of the measure itself. */
    for (i = 0; i < 16; i++) {
        tick();
        profileScanlineStart();
        lines = profileScanlineEnd();
        if (lines > idle_lines_max) {
            idle_lines_max = lines;
        }
    }

    /* 4. Seven voices, each sent a new volume and pitch every frame. */
    enter(PH_SEVEN, RGB(16, 2, 2));
    for (v = 0; v < 7; v++) {
        audioPlaySampleOn(v, SMP_WAVE, 60, AUDIO_PAN_CENTER, 0x0800 + v * 0x80);
        tick();
    }
    for (i = 0; i < 60; i++) {
        tick();
        profileScanlineStart();
        update_voices(7, 0x0800 + i * 8);
        lines = profileScanlineEnd();
        seven_frames++;
        seven_lines_sum += lines;
        if (lines > seven_lines_max) {
            seven_lines_max = lines;
        }
    }
    audioStopAll();

    enter(PH_DONE, RGB(8, 8, 8));
    while (1) {
        WaitForVBlank();
    }
}
