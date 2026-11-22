/*
 * The audio functions from an nmiSetLag() callback, one sender
 * (stress fixture, 2026-11-19).
 *
 * audio.h used to say "main thread only". What is true: the library's audio
 * commands are plain writes of the four APU ports by long address, and the
 * state they keep is in WRAM below $2000 (the callback's data bank is $7E,
 * which mirrors it), so a callback that is the ONLY sender may call them.
 * This ROM runs the same script of STEPS steps twice, each step one frame:
 *   - from an nmiSetLag() callback, one step per lag frame, while the main
 *     thread is busy and never waits for VBlank (so every VBlank is a lag
 *     frame), working on a 16- and a 32-bit chain whose result it checks
 *     (the callback must leave the main thread's scratch registers alone);
 *   - from the main thread, one step per frame, after an audioInit().
 * A step plays a note on voice i & 7 (audioPlaySampleOn, up to four commands),
 * sets the volume and pitch of another voice (audioSetVoiceVolumePitch),
 * its volume (audioSetVoiceVolume) and pitch (audioSetVoicePitch), sets the
 * master volume, the envelope (the default one) and the pan of the voice,
 * switches the echo off (audioSetVolume, audioSetADSR, audioSetVoicePan,
 * audioEnableEcho, audioDisableEcho), then stops
 * the voice that was started five steps ago (audioStopVoices): five voices
 * stay playing at the end, voices 3 to 7 (mask 0xF8).
 * It checks, for each run: every call returned AUDIO_OK, audio_error_count
 * did not move, the sound CPU reports the voices of the mask playing and the
 * others not (audioGetVoiceState, after the run, from the main thread), and
 * the WRAM mirror of the voices; the two runs must end identically.
 *
 * The fault seen: `make clean && make FAULT=1` makes the main thread a second
 * sender (it stops every voice in its loop while the callback sends): the
 * callback's run fails the mask. Read by audio_nmi.toml.
 */
#include <snes.h>
#include <snes/audio.h>
#include <snes/apu.h>

#define STEPS 64u                /* a multiple of 8 */
#define SMP   0
#define MASK_END 0xF8u           /* voices started in the last five steps */

extern u8 audio_voice_mirror[];  /* audio.c: 8 voices of 10 bytes */

static const u8 loop_brr[9] = { 0xB3, 0x77, 0x77, 0x77, 0x77, 0x99, 0x99, 0x99, 0x99 };

u16 r_cb_steps;          /* steps run by the callback                   -> STEPS */
u16 r_cb_bad;            /* calls of the callback not AUDIO_OK          -> 0 */
u16 r_cb_errs;           /* audio_error_count moved in the callback run -> 0 */
u16 r_cb_mask;           /* voices the sound CPU reports playing        -> 0xF8 */
u16 r_cb_sum;            /* checksum of the voice mirror                */
u16 r_work_bad;          /* rounds where the main thread's two chains differed -> 0 */
u16 r_main_steps;        /* steps run by the main thread                -> STEPS */
u16 r_main_bad;          /* calls not AUDIO_OK                          -> 0 */
u16 r_main_errs;         /* audio_error_count moved in the main run     -> 0 */
u16 r_main_mask;         /* as r_cb_mask                                -> 0xF8 */
u16 r_main_sum;          /* as r_cb_sum                                 */
u16 r_same;              /* both runs ended with the same mirror        -> 1 */
u16 r_done;              /* 0xC0DE at the end */

static u16 step;

static void do_step(u16 i, u16 *bad)
{
    u8 v = (u8)(i & 7u);
    u8 w = (u8)((i + 3u) & 7u);
    u16 pitch = (u16)(0x0800u + (i & 15u) * 0x40u);

    if (audioPlaySampleOn(v, SMP, (u8)(96 + (i & 31)), AUDIO_PAN_CENTER, pitch)
        == AUDIO_VOICE_NONE) {
        (*bad)++;
    }
    if (audioSetVoiceVolumePitch(w, (u8)(64 + (i & 15)), (u16)(pitch + 0x100u)) != AUDIO_OK) {
        (*bad)++;
    }
    if (audioSetVoiceVolume(w, (u8)(80 + (i & 7)), (u8)(70 + (i & 7))) != AUDIO_OK) {
        (*bad)++;
    }
    if (audioSetVoicePitch(w, (u16)(pitch + 0x80u)) != AUDIO_OK) {
        (*bad)++;
    }
    if (audioSetVolume(AUDIO_VOL_MAX) != AUDIO_OK) {
        (*bad)++;
    }
    /* the default envelope, set by hand: the notes keep sounding */
    if (audioSetADSR(w, 15, 0, 7, 0) != AUDIO_OK) {
        (*bad)++;
    }
    audioSetVoicePan(w, AUDIO_PAN_CENTER);
    if (audioEnableEcho(0) != AUDIO_OK || audioDisableEcho() != AUDIO_OK) {
        (*bad)++;
    }
    if (audioStopVoices((u8)(1u << w)) != AUDIO_OK) {
        (*bad)++;
    }
}

static void lag_tick(void)
{
    if (step < STEPS) {
        do_step(step, &r_cb_bad);
        step++;
    }
}

static u16 mirror_sum(void)
{
    u16 i, s = 0;
    for (i = 0; i < 80; i++) {
        s = (u16)(s * 31u + audio_voice_mirror[i]);
    }
    return s;
}

/* The voices the sound CPU reports playing, after the sound has settled. */
static u16 playing_mask(void)
{
    AudioVoiceState st;
    u16 m = 0;
    u8 v;
    for (v = 0; v < 8; v++) {
        audioGetVoiceState(v, &st);
        if (st.active) {
            m |= (u16)(1u << v);
        }
    }
    return m;
}

static void settle(void)
{
    u8 i;
    for (i = 0; i < 6; i++) {
        WaitForVBlank();
    }
}

int main(void)
{
    u16 i, err0;
    u32 b, c;
    volatile u16 k = 5;           /* a real multiply, not shifts */

    consoleInit();
    setScreenOn();

    /* the callback is the only sender */
    audioInit();
    audioLoadSample(SMP, loop_brr, sizeof(loop_brr), 0);
    WaitForVBlank();
    err0 = audio_error_count;
    b = 1;
    c = 1;
    step = 0;
    nmiSetLag(lag_tick);
    while (step < STEPS) {
        /* work that must come out right whatever the callback does */
        b = b * k + 11u;                /* 32-bit software multiply */
        c = (c << 2) + c + 11u;         /* the same value, by shifts */
        if (b != c) {
            r_work_bad++;
            c = b;
        }
#ifdef NMI_FAULT
        audioStopVoices(0xFF);          /* a second sender */
#endif
    }
    nmiSetLag(0);
    r_cb_steps = step;
    r_cb_errs = audio_error_count - err0;
    settle();
    r_cb_mask = playing_mask();
    r_cb_sum = mirror_sum();
    /* the main thread is the only sender */
    audioStopAll();
    settle();
    apuReset();
    audioInit();
    audioLoadSample(SMP, loop_brr, sizeof(loop_brr), 0);
    WaitForVBlank();
    err0 = audio_error_count;
    for (i = 0; i < STEPS; i++) {
        WaitForVBlank();
        do_step(i, &r_main_bad);
        r_main_steps++;
    }
    r_main_errs = audio_error_count - err0;
    settle();
    r_main_mask = playing_mask();
    r_main_sum = mirror_sum();
    r_same = (r_cb_sum == r_main_sum);

    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
