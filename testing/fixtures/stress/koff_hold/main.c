/*
 * The audio driver's key-off against the S-DSP's KOFF period (stress
 * fixture, 2026-11-09).
 *
 * The S-DSP reads KOFF ($5C) once every other sample, 64 SPC700 cycles; a
 * mask cleared before a read has fallen inside it leaves the voice playing.
 * The driver (lib/source/audio_driver.spc700.asm, cmd_koff) holds the mask
 * KOFF_HOLD_TURNS turns of a dbnz loop: 6n + 6 cycles, 132 for n = 21.
 *
 * Eight voices loop a one-block BRR sample at full envelope. Then, ITERS
 * times: a pseudo-random mask of voices (LFSR, fixed seed: the same run every
 * time), a pseudo-random delay of 0-127 five-cycle steps after the VBlank
 * (koff_delay, delay.asm) so that the KOFF write falls at every phase of the
 * 64-cycle period, audioStopVoices(mask), two VBlanks (more than a frame:
 * the release from full envelope takes 256 samples, 8 ms), then the
 * envelope of each voice read back (audioGetVoiceState(), the driver's ENVX):
 *   - a voice of the mask still sounding is a key-off the DSP never saw
 *     (r_not_cut, expected 0);
 *   - a voice outside the mask silent is a key-off that hit the wrong voice
 *     (r_wrong_cut, expected 0).
 * The voices of the mask are keyed on again for the next turn.
 *
 * The fault seen: built against a library whose hold is too short,
 *   make -C ../../../../lib AUDIO_KOFF_HOLD_TURNS=8 && make clean && make
 * (54 cycles), r_not_cut is not 0 (README.md of this folder quotes the run).
 *
 * A last phase measures what a stop costs the CPU, in scanlines
 * (profileScanlineStart/End): a state read alone, then four voices stopped
 * by one audioStopVoices() before the same read, then by four
 * audioStopVoice(). Read by koff_hold.toml.
 */
#include <snes.h>
#include <snes/audio.h>
#include <snes/profile.h>

#define ITERS  1024u        /* key-offs swept */
#define ROUNDS 16u          /* rounds of the cost phase */
#define SMP    0            /* the sample slot */

/* One BRR block, end + loop flags (header $B3: range 11, filter 0): a square
 * wave of 16 samples that loops on itself for as long as the voice is on. */
static const u8 loop_brr[9] = { 0xB3, 0x77, 0x77, 0x77, 0x77, 0x99, 0x99, 0x99, 0x99 };

volatile u16 koff_n;        /* read by koff_delay (delay.asm) */
extern void koff_delay(void);

u16 r_iters;                /* key-offs swept                          -> ITERS */
u16 r_not_cut;              /* voice of the mask still sounding        -> 0 */
u16 r_wrong_cut;            /* voice outside the mask silent           -> 0 */
u16 r_stopped;              /* voices stopped in all (> 0: the masks were not all 0) */
u16 r_not_cut_i;            /* the first turn that left a voice playing (0xFFFF: none) */
u16 r_start_bad;            /* a voice not sounding before the sweep   -> 0 */
u16 r_lines_read;           /* worst scanlines: one state read alone */
u16 r_lines_mask4;          /* ... audioStopVoices(0x0F), then the read */
u16 r_lines_loop4;          /* ... audioStopVoice() x 4, then the read */
u16 r_done;                 /* 0xC0DE at the end */

static u16 lfsr = 0xACE1;

static u16 next_rand(void)
{
    u16 bit = lfsr & 1;
    lfsr >>= 1;
    if (bit) {
        lfsr ^= 0xB400;
    }
    return lfsr;
}

static u8 sounding(u8 v)
{
    AudioVoiceState st;
    audioGetVoiceState(v, &st);
    return st.active;
}

static void key_on(u8 mask)
{
    u8 v;
    for (v = 0; v < AUDIO_MAX_VOICES; v++) {
        if (mask & (1 << v)) {
            audioPlaySampleOn(v, SMP, 40, AUDIO_PAN_CENTER, (u16)(0x0400 + v * 0x80));
        }
    }
}

static void keep_max(u16 *slot, u16 lines)
{
    if (lines > *slot) {
        *slot = lines;
    }
}

int main(void)
{
    u16 i, r, rnd, lines;
    u8 v, mask;

    consoleInit();
    setScreenOn();
    audioInit();
    audioLoadSample(SMP, loop_brr, sizeof(loop_brr), 0);
    for (v = 0; v < AUDIO_MAX_VOICES; v++) {
        /* instant attack, no decay, full sustain: the note holds until key-off */
        audioSetADSR(v, AUDIO_ATTACK_INSTANT, 0, 7, 0);
    }
    key_on(0xFF);
    WaitForVBlank();
    for (v = 0; v < AUDIO_MAX_VOICES; v++) {
        if (!sounding(v)) r_start_bad++;
    }

    r_not_cut_i = 0xFFFF;
    for (i = 0; i < ITERS; i++) {
        rnd = next_rand();
        mask = (u8)rnd;
        koff_n = (rnd >> 8) & 0x7F;
        WaitForVBlank();
        koff_delay();
        audioStopVoices(mask);
        WaitForVBlank();
        WaitForVBlank();
        for (v = 0; v < AUDIO_MAX_VOICES; v++) {
            u8 on = sounding(v);
            if (mask & (1 << v)) {
                r_stopped++;
                if (on) {
                    if (r_not_cut_i == 0xFFFF) r_not_cut_i = i;
                    r_not_cut++;
                }
            } else if (!on) {
                r_wrong_cut++;
            }
        }
        key_on(mask);
    }
    r_iters = i;

    /* The cost of a stop, seen from the CPU. Each measure ends on a state
     * read, which waits for its own acknowledgement and so for every command
     * before it: what the stops cost includes the driver's hold. */
    key_on(0xFF);
    for (r = 0; r < ROUNDS; r++) {
        WaitForVBlank();
        profileScanlineStart();
        sounding(7);
        lines = profileScanlineEnd();
        keep_max(&r_lines_read, lines);

        WaitForVBlank();
        profileScanlineStart();
        audioStopVoices(0x0F);
        sounding(7);
        lines = profileScanlineEnd();
        keep_max(&r_lines_mask4, lines);
        key_on(0x0F);

        WaitForVBlank();
        profileScanlineStart();
        audioStopVoice(0);
        audioStopVoice(1);
        audioStopVoice(2);
        audioStopVoice(3);
        sounding(7);
        lines = profileScanlineEnd();
        keep_max(&r_lines_loop4, lines);
        key_on(0x0F);
    }
    audioStopAll();
    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
