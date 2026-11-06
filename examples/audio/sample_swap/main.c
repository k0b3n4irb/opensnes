/**
 * @file main.c
 * @brief Samples at addresses the game chooses: a resident set and a swap area
 * @ingroup examples
 *
 * audioLoadSample() puts each sample where the last one ended. A game whose
 * samples do not all fit in the sound CPU's memory keeps some resident and
 * replaces others during play, in places of its own. This ROM does it with
 * audioLoadSampleAt() and audioLoadSampleBeginAt(), and measures what a
 * game needs to know:
 *
 * 1. **Placing**: two samples at $2000 and $3000, an ordinary load after
 *    them (it goes above), one just below the first (the allocator does not go back),
 *    and what audioUnloadSample() gives back.
 * 2. **Refusals**: a place below the sample area, at its end, wrapping past
 *    $FFFF, on another sample, on the load in steps; a size under 3; the
 *    slot being loaded. Each returns its code and changes nothing.
 * 3. **Replacing during play**: voice 0 keeps playing the sample at $2000
 *    while the one at $3000 is replaced in steps, then played.
 * 4. **Silence before overwriting**: the voice playing the sample to be
 *    replaced is left on, stopped just before the first step, or stopped
 *    one or two frames before; a luna manifest reads its envelope when the
 *    first byte goes, and the ROM times the release itself.
 * 5. **Timing of a note on another voice** while a load in steps runs.
 *
 * Nothing to press: the phases run on their own, the backdrop colour names
 * the phase, and the results are globals the manifests read.
 *
 * @par SNES Concepts
 * - The sample directory: two 16-bit addresses per sample, any byte
 * - A swap area of fixed places in ARAM, filled while voices play
 * - Key-off is a release, not a cut: the voice reads its sample until the
 *   envelope is down
 *
 * @par What to Observe
 * Backdrop: blue while placing, red for the refusals, green while voice 0
 * plays over the replacement, yellow for the silence cases, purple for the
 * timing, grey when done.
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

/** @brief The hand-written BRR samples of data.asm */
extern u8 lead_brr[], lead_brr_end[];
extern u8 pad_brr[], pad_brr_end[];
extern u8 bell_brr[], bell_brr_end[];
extern u8 swap_brr[], swap_brr_end[];
extern u8 bulk_brr[], bulk_brr_end[];

/** @brief Sample slots */
#define SMP_LEAD   1    /**< resident, at ADDR_LEAD */
#define SMP_SWAP   2    /**< the swap place: pad, then bell */
#define SMP_ABOVE  3    /**< an ordinary load after the placed ones */
#define SMP_BELOW  4    /**< placed below the allocator's pointer */
#define SMP_BULK   5    /**< the 1728-byte load timed against a note */
#define SMP_PROBE  6    /**< the slot the refusals ask for */

/** @brief The places of the swap area */
#define ADDR_LEAD  0x2000
#define ADDR_SWAP  0x3000
#define ADDR_BELOW 0x1FB8   /**< ends where lead starts: touching is not overlapping */
#define ADDR_BULK  0x4000

/** @brief Voices: the one that plays on, the one whose sample is replaced, the witness */
#define VOICE_LEAD    0
#define VOICE_SWAP    1
#define VOICE_WITNESS 2

/** @brief Phases, written to `phase` so a manifest knows where the run is */
#define PH_PLACE    1
#define PH_REFUSE   2
#define PH_REPLACE  3
#define PH_SILENCE  4
#define PH_TIMING   5
#define PH_DONE     6

/** @brief Silence cases (q3_case): the voice left on, stopped at once, stopped 1 or 2 frames before, stopped late in a frame then 1 frame */
#define CASE_ON       1
#define CASE_STOP     2
#define CASE_WAIT1    3
#define CASE_WAIT2    4
#define CASE_LATE1    5

/** @brief Oracles (WRAM): read by testing/manifests/audio_sample_swap*.toml */
u8  phase;
u16 frame;               /**< frames since the screen came on */
u16 free_start;          /**< audioGetFreeMemory() after audioInit() */
u16 free_lead;           /**< ...after the sample at $2000 */
u16 free_swap;           /**< ...after the sample at $3000 */
u16 free_above;          /**< ...after the ordinary load (it goes above) */
u16 free_below;          /**< ...after the sample placed just below lead (unchanged) */
u16 free_unload_below;   /**< ...after unloading it (unchanged: not on top) */
u16 free_unload_above;   /**< ...after unloading the ordinary one (back) */
u16 addr_above;          /**< where the ordinary load went */
u8  place_errs;          /**< the codes of the five placing calls, OR-ed: 0 */
u8  refusals[10];        /**< the code of each refused call, in order */
u16 free_refused;        /**< audioGetFreeMemory() after them: unchanged */
u16 errors_refused;      /**< audio_error_count after them */
u16 begin_lines;         /**< scanlines of one audioLoadSampleBeginAt() (it checks the 64 slots) */
u8  replace_steps;       /**< steps of 24 bytes for the 72-byte bell */
u8  replace_play_during; /**< audioPlaySampleOn() on the slot mid-load: AUDIO_VOICE_NONE */
u8  lead_active;         /**< voice 0 still sounding after the replacement */
u8  swap_play_after;     /**< audioPlaySampleOn() on the slot once whole */
u8  q3_case;             /**< the silence case whose first step is running, 0 between: a hook for a luna checkpoint (until_if) at that step */
u16 release_lines;       /**< scanlines from audioStopVoice() to a silent envelope */
u16 release_polls;       /**< envelope reads it took */
u16 q3_gap_lines[6];     /**< per case: scanlines from the key-off (or the same point, voice left on) to the first step */
u16 q3_gap_frames[6];    /**< per case: frames from the key-off to the first step */
u16 kon_frame[12];       /**< frame of each witness note: 4 alone, 4 after a step, 4 before one */
u16 kon_line[12];        /**< scanline at which its key-on command was sent */
u8  bulk_steps;          /**< steps the bulk load took */
u8  kon_steady;          /**< 1: the twelve notes went in twelve frames running */

/** @brief Backdrop colour per phase */
static void tint(u16 c) {
    setColor(0, c);
}

/** @brief Wait one frame */
static void tick(void) {
    WaitForVBlank();
    frame++;
}

/** @brief Wait n frames */
static void ticks(u8 n) {
    while (n--) {
        tick();
    }
}

/** @brief Start a phase */
static void enter(u8 p, u16 c) {
    phase = p;
    tint(c);
}

/** @brief The scanline the PPU is drawing (latched by reading SLHV) */
static u16 vline(void) {
    u16 lo;
    (void)*(volatile u8 *)0x213F;      /* STAT78: resets the OPVCT read toggle */
    (void)*(volatile u8 *)0x2137;      /* SLHV: latch the counters */
    lo = *(volatile u8 *)0x213D;
    return (u16)(lo | ((u16)(*(volatile u8 *)0x213D & 1) << 8));
}

/**
 * @brief Scanlines since the last VBlank began (line 225 of 262, NTSC): with
 *        `frame`, which tick() bumps then, a clock that runs across frames
 */
static u16 vpos(void) {
    u16 l = vline();
    return (u16)(l >= 225 ? l - 225 : l + 37);
}

/** @brief Size of a data.asm sample */
#define SIZE(name) ((u16)(name##_end - name))

/** @brief Run a load in steps of n bytes to its end, one step a frame */
static u8 steps_to_end(u16 n) {
    u8 count = 0;
    u16 left;
    do {
        tick();
        left = audioLoadSampleStep(n);
        count++;
    } while (left != 0 && left != AUDIO_LOAD_FAILED);
    return count;
}

/**
 * @brief One silence case: pad plays on the swap voice, then bell replaces
 *        it in its place after the action the case names
 *
 * pad is first put back where bell will go (a blocking replacement), so each
 * case starts from the same bytes. A control build whose swap_brr holds
 * pad's bytes (data.asm) runs the same code at the same cycles: its sound
 * differs from this one's only where the voice read a byte that changed.
 */
static void silence_case(u8 c) {
    u16 f0, p0;

    audioLoadSampleAt(SMP_SWAP, ADDR_SWAP, pad_brr, SIZE(pad_brr), 0);
    tick();
    audioPlaySampleOn(VOICE_SWAP, SMP_SWAP, 100, AUDIO_PAN_CENTER, 0x1000);
    ticks(20);
    if (c == CASE_LATE1) {
        while (vline() >= 200) {        /* out of this VBlank... */
        }
        while (vline() < 200) {         /* ...down to line 200 */
        }
    }
    f0 = frame;
    p0 = vpos();
    if (c != CASE_ON) {
        audioStopVoice(VOICE_SWAP);
    }
    if (c == CASE_WAIT1 || c == CASE_LATE1) {
        tick();
    } else if (c == CASE_WAIT2) {
        ticks(2);
    }
    audioLoadSampleBeginAt(SMP_SWAP, ADDR_SWAP, swap_brr, SIZE(swap_brr), 0);
    /* the time from the key-off to the first step, against release_lines */
    q3_gap_lines[c] = (u16)((frame - f0) * 262 + vpos() - p0);
    q3_gap_frames[c] = frame - f0;
    q3_case = c;               /* marks this step for the audio comparison */
    audioLoadSampleStep(24);
    q3_case = 0;
    steps_to_end(24);
    ticks(20);
    audioStopVoice(VOICE_SWAP);
    ticks(10);
}

/** @brief Time one witness note: the frame and the scanline its key-on goes */
static void witness_note(u8 i) {
    kon_frame[i] = frame;
    kon_line[i] = vline();
    audioPlaySampleOn(VOICE_WITNESS, SMP_LEAD, 80, AUDIO_PAN_CENTER, 0x0800);
}

int main(void) {
    u8 i, v;
    AudioVoiceState st;

    consoleInit();
    dmaClearVRAM();
    audioInit();
    for (v = 0; v < AUDIO_MAX_VOICES; v++) {
        /* instant attack, full sustain: the note holds until key-off */
        audioSetADSR(v, AUDIO_ATTACK_INSTANT, 0, 7, 0);
    }
    tint(RGB(2, 2, 6));
    setScreenOn();

    /* 1. Placing. */
    enter(PH_PLACE, RGB(2, 4, 16));
    free_start = audioGetFreeMemory();
    place_errs = audioLoadSampleAt(SMP_LEAD, ADDR_LEAD, lead_brr, SIZE(lead_brr), 0);
    free_lead = audioGetFreeMemory();
    place_errs |= audioLoadSampleAt(SMP_SWAP, ADDR_SWAP, pad_brr, SIZE(pad_brr), 0);
    free_swap = audioGetFreeMemory();
    place_errs |= audioLoadSample(SMP_ABOVE, lead_brr, SIZE(lead_brr), 0);
    free_above = audioGetFreeMemory();
    {
        AudioSample info;
        place_errs |= audioGetSampleInfo(SMP_ABOVE, &info);
        addr_above = info.spcAddress;
    }
    place_errs |= audioLoadSampleAt(SMP_BELOW, ADDR_BELOW, bell_brr, SIZE(bell_brr), 0);
    free_below = audioGetFreeMemory();
    audioUnloadSample(SMP_BELOW);
    free_unload_below = audioGetFreeMemory();
    audioUnloadSample(SMP_ABOVE);
    free_unload_above = audioGetFreeMemory();
    tick();

    /* 2. Refusals: each one leaves memory, slots and ARAM as they were. */
    enter(PH_REFUSE, RGB(16, 2, 2));
    refusals[0] = audioLoadSampleAt(64, ADDR_BULK, pad_brr, SIZE(pad_brr), 0);
    refusals[1] = audioLoadSampleAt(SMP_PROBE, 0x0AF8, pad_brr, SIZE(pad_brr), 0);   /* below $0B00 */
    refusals[2] = audioLoadSampleAt(SMP_PROBE, 0xFFC0, pad_brr, SIZE(pad_brr), 0);   /* at the end */
    refusals[3] = audioLoadSampleAt(SMP_PROBE, 0xFFB0, pad_brr, SIZE(pad_brr), 0);   /* would wrap */
    refusals[4] = audioLoadSampleAt(SMP_PROBE, ADDR_LEAD + SIZE(lead_brr) - 1,
                                    pad_brr, SIZE(pad_brr), 0);                   /* one byte on lead */
    refusals[5] = audioLoadSampleAt(SMP_PROBE, ADDR_BULK, pad_brr, 0, 0);            /* size 0 */
    refusals[6] = audioLoadSampleBeginAt(SMP_PROBE, ADDR_BULK, pad_brr, 2, 0);       /* size under 3 */
    free_refused = audioGetFreeMemory();
    tick();

    /* 3. Replacing during play: voice 0 holds lead while bell takes pad's place. */
    enter(PH_REPLACE, RGB(2, 14, 4));
    audioPlaySampleOn(VOICE_LEAD, SMP_LEAD, 100, AUDIO_PAN_CENTER, 0x1000);
    audioPlaySampleOn(VOICE_SWAP, SMP_SWAP, 100, AUDIO_PAN_CENTER, 0x1000);
    ticks(30);
    audioStopVoice(VOICE_SWAP);
    ticks(2);
    profileScanlineStart();
    audioLoadSampleBeginAt(SMP_SWAP, ADDR_SWAP, bell_brr, SIZE(bell_brr), 0);
    begin_lines = profileScanlineEnd();
    /* while it loads: the slot cannot be played, its place cannot be taken,
     * a second load in steps waits */
    replace_play_during = audioPlaySampleOn(VOICE_SWAP, SMP_SWAP, 100, AUDIO_PAN_CENTER, 0x1000);
    refusals[7] = audioLoadSampleAt(SMP_PROBE, ADDR_SWAP + 0x10, pad_brr, SIZE(pad_brr), 0);
    refusals[8] = audioLoadSampleAt(SMP_SWAP, ADDR_BULK, pad_brr, SIZE(pad_brr), 0);
    refusals[9] = audioLoadSampleBeginAt(SMP_PROBE, ADDR_BULK, pad_brr, SIZE(pad_brr), 0);
    errors_refused = audio_error_count;
    replace_steps = steps_to_end(24);
    audioGetVoiceState(VOICE_LEAD, &st);
    lead_active = st.active;
    swap_play_after = audioPlaySampleOn(VOICE_SWAP, SMP_SWAP, 100, AUDIO_PAN_CENTER, 0x1000);
    ticks(30);
    audioStopAll();
    ticks(10);

    /* 4. Silence before overwriting. First the release, timed from here. */
    enter(PH_SILENCE, RGB(16, 14, 2));
    audioPlaySampleOn(VOICE_SWAP, SMP_LEAD, 100, AUDIO_PAN_CENTER, 0x1000);
    ticks(20);
    profileScanlineStart();
    audioStopVoice(VOICE_SWAP);
    do {
        audioGetVoiceState(VOICE_SWAP, &st);
        release_polls++;
    } while (st.active);
    release_lines = profileScanlineEnd();
    ticks(10);
    silence_case(CASE_ON);
    silence_case(CASE_STOP);
    silence_case(CASE_WAIT1);
    silence_case(CASE_WAIT2);
    silence_case(CASE_LATE1);

    /* 5. A note on another voice, alone, after a step, before a step. */
    enter(PH_TIMING, RGB(10, 2, 14));
    for (i = 0; i < 4; i++) {
        tick();
        witness_note(i);
    }
    audioLoadSampleBeginAt(SMP_BULK, ADDR_BULK, bulk_brr, SIZE(bulk_brr), 0);
    for (i = 4; i < 8; i++) {
        tick();
        audioLoadSampleStep(192);
        bulk_steps++;
        witness_note(i);
    }
    for (i = 8; i < 12; i++) {
        tick();
        witness_note(i);
        audioLoadSampleStep(192);
        bulk_steps++;
    }
    bulk_steps += steps_to_end(192);
    /* one note a game frame, whatever the load did */
    kon_steady = 1;
    for (i = 1; i < 12; i++) {
        if (kon_frame[i] != (u16)(kon_frame[0] + i)) {
            kon_steady = 0;
        }
    }
    ticks(10);
    audioStopAll();

    enter(PH_DONE, RGB(8, 8, 8));
    while (1) {
        WaitForVBlank();
    }
}
