/*
 * audioCommandDone(): the acknowledgement of the last command, read
 * without waiting (stress fixture, 2026-11-19).
 *
 * A command is posted and the function returns; the next command waits,
 * bounded, for the sound CPU's acknowledgement. audioCommandDone() is that
 * wait reduced to one read of the port. This ROM checks, on luna:
 *   1. nothing pending (after audioLoadSample(), which waits for its own
 *      acknowledgement): it reads 1, twice;
 *   2. ROUNDS times, right after audioStopVoices() (the driver holds the
 *      key-off 132 of its cycles before it acknowledges): the first call
 *      reads 0, a later one reads 1, and the number of calls is kept
 *      (r_calls_min / r_calls_max);
 *   3. once it has read 1, the command is forgotten (audio_pending 0) and
 *      the next send does not wait: it returns AUDIO_OK, costs no scanline
 *      where the same send right after the stop costs some
 *      (r_lines_after_done against r_lines_no_done), audio_error_count
 *      does not move;
 *   4. the use it is made for: an nmiSetLag() callback, the only sender,
 *      sends a command each lag frame on which it reads 1, while the main
 *      thread never waits for VBlank.
 *
 * The fault seen: built with `make clean && make FAULT=1`, the function is
 * replaced by one that always reads 1 (and forgets the command, as the real
 * one does on a 1); r_first_zero is then 0, not ROUNDS, and luna test fails
 * (README.md of this folder quotes the run). Read by audio_done.toml.
 */
#include <snes.h>
#include <snes/audio.h>
#include <snes/profile.h>

#define ROUNDS         64u      /* stop then poll */
#define DONE_CALLS_MAX 4000u    /* bound of one poll: far above the hold */
#define LAG_FRAMES     120u     /* frames of the nmiSetLag phase */
#define SMP            0

extern u8 audio_pending;        /* audio.c: the command not acknowledged yet */

#ifdef DONE_FAULT
/* The injected fault: reads 1 at once and forgets the command. */
static u8 done_fault(void)
{
    audio_pending = 0;
    return 1;
}
#define audioCommandDone done_fault
#endif

static const u8 loop_brr[9] = { 0xB3, 0x77, 0x77, 0x77, 0x77, 0x99, 0x99, 0x99, 0x99 };

u16 r_idle;                 /* 1 + 1 with nothing pending                  -> 2 */
u16 r_idle_errs;            /* audio_error_count moved in phase 1          -> 0 */
u16 r_rounds;               /* rounds run                                  -> ROUNDS */
u16 r_first_zero;           /* rounds whose first call after the send read 0 -> ROUNDS */
u16 r_never;                /* rounds that never read 1 within the bound   -> 0 */
u16 r_calls_min;            /* fewest calls up to the 1 (the 1 included) */
u16 r_calls_max;            /* most calls up to the 1 */
u16 r_pending_left;         /* rounds where audio_pending stayed after a 1 -> 0 */
u16 r_next_bad;             /* next send not AUDIO_OK                       -> 0 */
u16 r_round_errs;           /* audio_error_count moved in phase 2-3        -> 0 */
u16 r_lines_after_done;     /* worst scanlines of a send after a 1 */
u16 r_lines_no_done;        /* least scanlines of the same send right after the stop */
u16 r_lines_empty;          /* worst scanlines of the measure alone */
u16 r_lag_runs;            /* lag callbacks run */
u16 r_lag_sends;            /* commands the callback sent */
u16 r_lag_busy;             /* lag frames on which it read 0 */
u16 r_lag_bad;              /* sends of the callback not AUDIO_OK           -> 0 */
u16 r_lag_errs;             /* audio_error_count moved in phase 4          -> 0 */
u16 r_done;                 /* 0xC0DE at the end */

static void lag_tick(void)
{
    r_lag_runs++;
    if (audioCommandDone()) {
        if (audioSetVolume(AUDIO_VOL_MAX) != AUDIO_OK) {
            r_lag_bad++;
        }
        r_lag_sends++;
    } else {
        r_lag_busy++;
    }
}

int main(void)
{
    u16 i, calls, err0, lines, f0;
    u8 got;

    consoleInit();
    setScreenOn();
    audioInit();
    audioLoadSample(SMP, loop_brr, sizeof(loop_brr), 0);

    /* 1. nothing pending */
    err0 = audio_error_count;
    r_idle = audioCommandDone();
    r_idle += audioCommandDone();
    r_idle_errs = audio_error_count - err0;

    /* 2-3. right after a send */
    err0 = audio_error_count;
    r_calls_min = 0xFFFF;
    r_lines_no_done = 0xFFFF;
    for (i = 0; i < ROUNDS; i++) {
        WaitForVBlank();
        audioStopVoices(0x01);
        got = audioCommandDone();
        if (!got) {
            r_first_zero++;
        }
        calls = 1;
        while (!got && calls < DONE_CALLS_MAX) {
            got = audioCommandDone();
            calls++;
        }
        if (!got) {
            r_never++;
            continue;
        }
        if (calls < r_calls_min) r_calls_min = calls;
        if (calls > r_calls_max) r_calls_max = calls;
        if (audio_pending) {
            r_pending_left++;
        }
        profileScanlineStart();
        if (audioSetVolume(AUDIO_VOL_MAX) != AUDIO_OK) {
            r_next_bad++;
        }
        lines = profileScanlineEnd();
        if (lines > r_lines_after_done) r_lines_after_done = lines;

        /* the control: the same send right after the stop waits for it */
        WaitForVBlank();
        audioStopVoices(0x01);
        profileScanlineStart();
        if (audioSetVolume(AUDIO_VOL_MAX) != AUDIO_OK) {
            r_next_bad++;
        }
        lines = profileScanlineEnd();
        if (lines < r_lines_no_done) r_lines_no_done = lines;

        /* the instrument alone */
        profileScanlineStart();
        lines = profileScanlineEnd();
        if (lines > r_lines_empty) r_lines_empty = lines;
    }
    r_rounds = i;
    r_round_errs = audio_error_count - err0;

    /* 4. the callback is the only sender; the main thread never waits */
    WaitForVBlank();
    err0 = audio_error_count;
    nmiSetLag(lag_tick);
    f0 = getFrameCount();
    while ((u16)(getFrameCount() - f0) < LAG_FRAMES) {
    }
    nmiSetLag(0);
    r_lag_errs = audio_error_count - err0;

    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
