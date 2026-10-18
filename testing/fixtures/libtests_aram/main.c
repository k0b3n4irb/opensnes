/**
 * @file main.c
 * @brief Library fixture: the sound CPU's memory — a sample loaded in steps,
 *        and the echo ring taking what its delay needs, at the top.
 *
 * The ring first (below); then a 1000-byte sample sent 63 bytes at a time,
 * with what a game does between two steps (a voice command, another sample
 * loaded), a 10-byte one whose last step takes the odd byte, and a load
 * given up. The ring:
 * Three "samples" of 16 KB each are loaded (a fill, never played): the
 * sample area then ends at $CB00. A delay of 7 (14 KB, ring at $C700) does
 * not fit and is refused with nothing changed; a delay of 6 (ring at $CF00)
 * fits. Two samples are given back and the delay goes from 6 to 1: the
 * S-DSP keeps the old ring's length until it reaches its end, and from the
 * new start ($F700) a 12 KB ring passes $FFFF and goes on over the driver.
 * The driver waits with the ring's writes off; it must still answer after.
 *
 * Read by test_libtest_aram.py.
 */
#include <snes.h>
#include <snes/audio.h>

#define CHUNK 0x4000

extern const u8 echo_fill[];   /* data.asm: CHUNK bytes, none of them 0 */
extern const u8 step_pat[];    /* data.asm: 1000 bytes, (n * 7 + 3) | 1 */

/* --- a sample loaded in steps (slots 10-13, after the ring's part) --- */
u16 s_begin;         /* audioLoadSampleBegin(10, 1000 bytes)          -> 0 */
u16 s_free_begin;    /* the place is taken at once: 0x7C00 - 1000     -> 0x7818 */
u16 s_begin_twice;   /* a second Begin: AUDIO_ERR_INVALID_ID          -> 2 */
u16 s_play_early;    /* audioPlaySample(10) before it is whole        -> 0xFF */
u16 s_step1;         /* audioLoadSampleStep(64): 63 sent              -> 937 */
u16 s_other;         /* audioLoadSample(11, 9 bytes) meanwhile        -> 0 */
u16 s_other_addr;    /* ...above the place taken: 0x4B00 + 1000       -> 0x4EE8 */
u16 s_steps;         /* steps until 0, the first included             -> 16 */
u16 s_addr;          /* slot 10: where it is                          -> 0x4B00 */
u16 s_play;          /* audioPlaySampleOn(2, 10, ...) once whole      -> 2 */
u16 s_idle;          /* a Step with nothing under way                 -> 0 */
u16 s_odd1;          /* 10 bytes, Step(8): 6 sent                     -> 4 */
u16 s_odd2;          /* Step(3): the last 4 together                  -> 0 */
u16 s_cancel;        /* Begin(13) + one step, then audioUnloadSample  -> 0 */
u16 s_free_cancel;   /* the memory came back: same count as before    -> 1 */
u16 s_begin_after;   /* and another Begin is accepted                 -> 0 */

u16 r_init;          /* audioInit()                                  -> 0 */
u16 r_load;          /* the three loads, or'ed                       -> 0 */
u16 r_free_full;     /* no echo: 0xFFC0 - 0xCB00                     -> 0x34C0 */
u16 r_refuse7;       /* audioSetEcho(7): AUDIO_ERR_NO_MEMORY         -> 1 */
u16 r_free_refused;  /* nothing changed                              -> 0x34C0 */
u16 r_ok6;           /* audioSetEcho(6)                              -> 0 */
u16 r_free6;         /* 0xCF00 - 0xCB00                              -> 0x0400 */
u16 r_ok1;           /* audioSetEcho(1) after two unloads            -> 0 */
u16 r_free1;         /* 0xF700 - 0x4B00: the shorter ring gave back  -> 0xAC00 */
u16 r_alive;         /* a command twenty frames later: answered      -> 0 */
u16 r_ok7;           /* audioSetEcho(7) now that it fits             -> 0 */
u16 r_free7;         /* 0xC700 - 0x4B00                              -> 0x7C00 */

/* --- the ring given back (slots 14-16; the samples above end at $4EFB) --- */
u16 e_rel;           /* audioReleaseEcho()                            -> 0 */
u16 e_free_rel;      /* up to $FFC0 again: 0xFFC0 - 0x4EFB            -> 0xB0C5 */
u16 e_load;          /* 2 x 16 KB + 0x30C5, over the old ring         -> 0 */
u16 e_free_full;     /* nothing left                                  -> 0 */
u16 e_enable;        /* audioEnableEcho(0xFF) with no ring            -> 0 */
u16 e_set_busy;      /* audioSetEcho(1): samples at $F700             -> 1 */
u16 e_set_again;     /* after slot 16 is given back                   -> 0 */
u16 e_free_again;    /* 0xF700 - 0xCEFB                               -> 0x2805 */
u16 r_done;          /* 0xBEEF */

int main(void) {
    u8 i;

    consoleInit();
    r_init = audioInit();

    r_load = audioLoadSample(0, echo_fill, CHUNK, 0);
    r_load |= audioLoadSample(1, echo_fill, CHUNK, 0);
    r_load |= audioLoadSample(2, echo_fill, CHUNK, 0);
    r_free_full = audioGetFreeMemory();

    r_refuse7 = audioSetEcho(7, 40, 20, 20);
    r_free_refused = audioGetFreeMemory();
    r_ok6 = audioSetEcho(6, 40, 20, 20);
    r_free6 = audioGetFreeMemory();
    audioEnableEcho(0xFF);

    audioUnloadSample(2);
    audioUnloadSample(1);
    r_ok1 = audioSetEcho(1, 40, 20, 20);
    r_free1 = audioGetFreeMemory();
    for (i = 0; i < 20; i++) {
        WaitForVBlank();
    }
    r_alive = audioSetVoiceVolume(0, 64, 64);
    r_alive |= audioSetVoiceVolume(0, 64, 64);   /* waits for the ack of the one before */
    r_ok7 = audioSetEcho(7, 40, 20, 20);
    r_free7 = audioGetFreeMemory();

    /* --- a sample loaded in steps, above the one the ring's part left at
     * $0B00-$4AFF: from $4B00, under the ring of delay 7 at $C700 --- */
    s_begin = audioLoadSampleBegin(10, step_pat, 1000, 0);
    s_free_begin = audioGetFreeMemory();
    s_begin_twice = audioLoadSampleBegin(12, step_pat, 10, 0);
    s_play_early = audioPlaySample(10);
    s_step1 = audioLoadSampleStep(64);
    audioSetVoiceVolume(1, 11, 22);             /* a voice command between two steps */
    s_other = audioLoadSample(11, echo_fill, 9, 0);
    {
        AudioSample info;
        u16 left;
        s_steps = 1;
        do {
            left = audioLoadSampleStep(64);
            s_steps++;
        } while (left != 0 && left != AUDIO_LOAD_FAILED && s_steps < 100);
        if (left == AUDIO_LOAD_FAILED) s_steps = 0xFFFF;
        if (audioGetSampleInfo(11, &info) == AUDIO_OK) s_other_addr = info.spcAddress;
        if (audioGetSampleInfo(10, &info) == AUDIO_OK) s_addr = info.spcAddress;
    }
    s_play = audioPlaySampleOn(2, 10, 100, AUDIO_PAN_CENTER, 0x1000);
    audioStopVoice(2);
    s_idle = audioLoadSampleStep(64);

    s_odd1 = 0xEEEE;
    if (audioLoadSampleBegin(12, step_pat, 10, 0) == AUDIO_OK) {
        s_odd1 = audioLoadSampleStep(8);
        s_odd2 = audioLoadSampleStep(3);
    }
    {
        u16 before = audioGetFreeMemory();
        s_cancel = audioLoadSampleBegin(13, step_pat, 300, 0);
        audioLoadSampleStep(64);
        audioUnloadSample(13);
        s_free_cancel = (u16)(audioGetFreeMemory() == before);
        s_begin_after = audioLoadSampleBegin(13, step_pat, 30, 0);
        audioUnloadSample(13);
    }

    /* --- the ring given back: the echo of one scene, then samples where it
     * was. The ring of delay 7 ($C700-$FEFF) is running and written. --- */
    e_rel = audioReleaseEcho();
    e_free_rel = audioGetFreeMemory();
    e_load = audioLoadSample(14, echo_fill, CHUNK, 0);
    e_load |= audioLoadSample(15, echo_fill, CHUNK, 0);
    e_load |= audioLoadSample(16, echo_fill, (u16)(0xFFC0 - 0xCEFB), 0);
    e_free_full = audioGetFreeMemory();
    e_enable = audioEnableEcho(0xFF);           /* no ring: must not clear one */
    for (i = 0; i < 20; i++) {
        WaitForVBlank();
    }
    e_set_busy = audioSetEcho(1, 40, 20, 20);   /* the samples are where it goes */
    audioUnloadSample(16);
    e_set_again = audioSetEcho(1, 40, 20, 20);
    e_free_again = audioGetFreeMemory();

    r_done = 0xBEEF;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
