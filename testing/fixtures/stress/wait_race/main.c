/*
 * WaitForVBlank against an NMI that lands between its store and its WAI
 * (stress fixture, 2026-11-08).
 *
 * WaitForVBlank sets vblank_flag = 1 ("the main thread is ready") and then
 * halts with WAI until the NMI clears it. An NMI that arrives after the store
 * and before the WAI runs the VBlank work and clears the flag; the WAI then
 * sleeps until the NEXT NMI, which finds the flag at 0 and counts as a lag
 * frame: a frame is lost although the main thread was on time.
 *
 * The main loop here calls WaitForVBlank after a delay that grows by one
 * 5-cycle step per iteration (race_delay, delay.asm), so that over the sweep
 * the store falls on every phase of the frame, the window of the one
 * instruction included. Two callbacks count the frames of each iteration:
 * on_work (nmiSet) for a frame that did the VBlank work, on_lag (nmiSetLag)
 * for a lag frame. The order tells a real delay from the lost wake-up:
 *   - the lag frame comes FIRST: the store was after the NMI, the main
 *     thread really was late (counted in r_late);
 *   - the lag frame comes AFTER a work frame of the same iteration: the NMI
 *     had already done the work for this call and the WAI slept through to
 *     the next one: the bug (counted in r_bug, expected 0).
 *
 * Read by wait_race.toml.
 */
#include <snes.h>

#define START 9000u         /* first delay step of the sweep */
#define COUNT 600u          /* steps: the NMI instant falls at about step 9440 (a full
                             * sweep from 0 found the one hit there, 2026-11-08); the
                             * range keeps a margin of 400 steps (16 000 master cycles)
                             * for a longer or shorter NMI handler */

volatile u16 race_n;        /* read by race_delay (delay.asm) */
extern void race_delay(void);

u16 r_iters;                /* iterations run                           -> COUNT */
u16 r_bug;                  /* work frame, then a lag frame, one call   -> 0 */
u16 r_late;                 /* lag frame first: the main thread was late (> 0) */
u16 r_normal;               /* one work frame per call                  (> 0) */
u16 r_done;                 /* 0xC0DE at the end */
u16 r_i;                    /* progress: the iteration in hand (diagnosis) */
u16 r_bug_i;                /* the first step that lost its wake-up (0 = none) */

static volatile u16 nw, nl; /* work / lag frames seen */
static volatile u16 nw_at_lag;  /* work frames seen when the last lag frame came */

static void on_work(void) { nw++; }
static void on_lag(void)
{
    nl++;
    nw_at_lag = nw;
}

int main(void)
{
    u16 i;

    consoleInit();
    nmiSet(on_work);
    nmiSetLag(on_lag);
    setScreenOn();
    WaitForVBlank();
    WaitForVBlank();

    for (i = 0; i < COUNT; i++) {
        u16 w0, l0, dw, dl;
        r_i = i;
        w0 = nw;
        l0 = nl;
        race_n = START + i;
        race_delay();
        WaitForVBlank();
        dw = nw - w0;
        dl = nl - l0;
        if (dl == 0) {
            if (dw == 1) r_normal++;
        } else if (nw_at_lag != w0) {
            /* A work frame had already run in this call when the lag frame
             * came: the NMI did the work for this call, the WAI slept through
             * to the next NMI: the lost wake-up. */
            if (!r_bug) r_bug_i = START + i;
            r_bug++;
        } else {
            /* The lag frame came first: the store was after the NMI. */
            r_late++;
        }
    }
    r_iters = i;
    nmiSet(0);
    nmiSetLag(0);
    r_done = 0xC0DE;
    while (1) { }
    return 0;
}
