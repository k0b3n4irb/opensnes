/**
 * @file main.c
 * @brief Soundboard — the audio v2 engine driven entirely from C
 * @ingroup examples
 *
 * The first audio example with ZERO SPC700 assembly: the lib's `audio`
 * module uploads its own resident driver, and everything below —
 * loading four BRR samples into APU RAM, playing them with pan and
 * pitch, shaping envelopes, switching a concert-hall echo on and off —
 * is plain C calls.
 *
 * Controls:
 * - A: cello note (center) — B: "AU" vowel (left) — X: "SS" hiss
 *   (right) — Y: "PP" pop (center)
 * - L/R: replay the cello a fifth down / up (pitch demo)
 * - Up/Down: master volume
 * - START: toggle the echo hall
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Dynamic BRR loading: audioLoadSample() streams samples over the
 *   APU I/O ports at runtime — no fixed APU memory image
 * - audioPlaySampleEx(): volume/pan/pitch per play, voices
 *   round-robin allocated by the engine
 * - ADSR set once per voice at init — notes ring and die naturally
 * - Echo: audioSetEcho() clears and sizes the ring, FIR tap 0,
 *   audioEnableEcho() routes voices in
 *
 * @par What to Observe
 * Every button plays immediately, polyphonic up to 8 voices; START
 * adds a hall around everything. The backdrop tints with each action.
 *
 * @par Modules Used
 * console, audio, input
 *
 * @see lib/include/snes/audio.h — the full engine API
 * @see .claude/notes/chantiers/audio_v2.md — engine design
 */

#include <snes.h>
#include <snes/audio.h>
#include <snes/input.h>

extern u8 brr_cello[], brr_cello_end[];
extern u8 brr_au[], brr_au_end[];
extern u8 brr_ss[], brr_ss_end[];
extern u8 brr_pp[], brr_pp_end[];

/** @brief Sample slots */
#define SMP_CELLO 0
#define SMP_AU    1
#define SMP_SS    2
#define SMP_PP    3

/** @brief Cello DSP pitch: krom's C5 ($8BB0 >> 4, as in pitch_mod) */
#define PITCH_CELLO   0x08BB
/** @brief A fifth below / above (x2/3 and x3/2) */
#define PITCH_CELLO_LO 0x05D2
#define PITCH_CELLO_HI 0x0D18

/** @brief Probe oracles (WRAM) */
u8 last_voice;
u16 play_count;

/** @brief Master volume state (Up/Down) */
static u8 master_vol;
static u8 echo_on;

/** @brief Play + tint the backdrop + update the probe oracles */
static void play(u8 sample, u8 vol, u8 pan, u16 pitch, u16 tint) {
    u8 v = audioPlaySampleEx(sample, vol, pan, pitch);
    if (v != 0xFF) {
        last_voice = v;
        play_count++;
    }
    setColor(0, tint);
}

int main(void) {
    u8 i;
    u16 keys;

    consoleInit();
    audioInit();

    /* Stream the four samples into APU RAM. The cello loops at krom's
     * 4167; the vowel, hiss and pop are one-shots (the vowel's decay
     * is baked into the PCM — see gen_au_boost.py). */
    audioLoadSample(SMP_CELLO, brr_cello, (u16)(brr_cello_end - brr_cello), 4167);
    audioLoadSample(SMP_AU, brr_au, (u16)(brr_au_end - brr_au), 0);
    audioLoadSample(SMP_SS, brr_ss, (u16)(brr_ss_end - brr_ss), 0);
    audioLoadSample(SMP_PP, brr_pp, (u16)(brr_pp_end - brr_pp), 0);

    /* One natural envelope for every voice: instant attack, decay to
     * 6/8, then a gentle fade — looping samples ring out and die
     * instead of sustaining forever. Set once; playback respects it. */
    for (i = 0; i < AUDIO_MAX_VOICES; i++) {
        audioSetADSR(i, AUDIO_ATTACK_INSTANT, 5, 6, 10);
    }

    master_vol = AUDIO_VOL_MAX;
    setColor(0, RGB(4, 4, 8));
    setScreenOn();

    while (1) {
        WaitForVBlank();
        keys = padPressed(0);

        if (keys & KEY_A) {
            play(SMP_CELLO, 127, AUDIO_PAN_CENTER, PITCH_CELLO, RGB(20, 12, 0));
        }
        if (keys & KEY_B) {
            play(SMP_AU, 127, AUDIO_PAN_LEFT + 3, 0x1000, RGB(0, 16, 8));
        }
        if (keys & KEY_X) {
            play(SMP_SS, 100, AUDIO_PAN_RIGHT - 3, 0x1000, RGB(8, 8, 20));
        }
        if (keys & KEY_Y) {
            play(SMP_PP, 127, AUDIO_PAN_CENTER, 0x1000, RGB(20, 4, 12));
        }
        if (keys & KEY_L) {
            play(SMP_CELLO, 127, AUDIO_PAN_LEFT + 2, PITCH_CELLO_LO, RGB(12, 8, 0));
        }
        if (keys & KEY_R) {
            play(SMP_CELLO, 127, AUDIO_PAN_RIGHT - 2, PITCH_CELLO_HI, RGB(24, 16, 0));
        }

        if ((keys & KEY_UP) && master_vol <= AUDIO_VOL_MAX - 16) {
            master_vol += 16;
            audioSetVolume(master_vol);
        }
        if ((keys & KEY_DOWN) && master_vol >= 16) {
            master_vol -= 16;
            audioSetVolume(master_vol);
        }

        if (keys & KEY_START) {
            if (echo_on) {
                audioDisableEcho();
                echo_on = 0;
                setColor(0, RGB(4, 4, 8));
            } else {
                static const s8 hall_fir[8] = { 127, 0, 0, 0, 0, 0, 0, 0 };
                audioSetEcho(5, 70, 35, 35);
                audioSetEchoFilter(hall_fir);
                audioEnableEcho(0xFF);
                echo_on = 1;
                setColor(0, RGB(10, 4, 16));
            }
        }
    }

    return 0;
}
