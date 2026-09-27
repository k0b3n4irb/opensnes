/**
 * @file main.c
 * @brief Super FX fixture: a GSU job run from the code cache while the CPU
 *        keeps running from ROM (gsuCacheLoad / gsuStartCached / gsuBusy /
 *        gsuWait)
 *
 * The job (gsu_job.sfx) adds 7 six hundred thousand times, about seven
 * frames at 21 MHz. The main loop waits for VBlank in ROM meanwhile and
 * counts the frames: with the ROM left to the CPU (SCMR RON = 0) the game
 * runs during the job, which gsuLaunch() cannot offer.
 */
#include <snes.h>
#include <snes/superfx.h>

extern const u8 gsu_job[], gsu_job_end[];
extern void gsuJobReadResults(void);
extern u16 r_sum, r_outer, r_marker;

/** @brief gsuInit(): 1 on a Super FX cart */
u16 r_present;
/** @brief gsuBusy() right after gsuStartCached(): the job is running */
u16 r_busy_after_start;
/** @brief main-loop frames (WaitForVBlank from ROM) while the GSU ran */
u16 r_frames_during;
/** @brief gsuBusy() after gsuWait(): 0 */
u16 r_busy_after_wait;
/** @brief end marker for the test script */
u16 r_done;

int main(void) {
    consoleInit();
    setScreenOn();
    r_present = gsuInit();
    gsu_scmr = 0x18;     /* RAN + RON asked: gsuStartCached keeps RAN, drops RON */
    WaitForVBlank();

    gsuCacheLoad(gsu_job, (u16)(gsu_job_end - gsu_job));
    gsuStartCached(0);
    r_busy_after_start = gsuBusy();
    r_frames_during = 0;
    while (gsuBusy()) {
        WaitForVBlank();                 /* the game's frame, from ROM */
        r_frames_during++;
    }
    gsuWait();
    r_busy_after_wait = gsuBusy();
    gsuJobReadResults();
    r_done = 0xD0E5;

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
