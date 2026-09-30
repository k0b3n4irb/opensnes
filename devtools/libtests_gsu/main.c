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
extern void gsuJobClearResults(void);
extern u16 r_sum, r_outer, r_marker;
extern void ramRunRomJob(void);
extern u16 r_ram_frames, r_ram_polls;
extern volatile u16 frame_count;

/** @brief fourth job, the wait loop written in C (RAM_CODE): frames, polls */
u16 r_c_frames;
u16 r_c_polls;

/* The same job as ramRunRomJob, in C. Everything here compiles to register
 * and bank-0 RAM accesses, no call: while RON = 1 a single ROM fetch would
 * count as a bus violation, and the manifest asserts there are none. */
RAM_CODE static void ram_c_rom_job(void) {
    u16 start, polls = 0;

    gsu_owns_cart = 1;
    REG_CFGR = gsu_cfgr;
    REG_CLSR = 1;
    REG_SCBR = gsu_scbr;
    REG_SCMR = (u8)(gsu_scmr | 0x18);   /* RON + RAN */
    REG_PBR = gsu_prog_bank;
    start = frame_count;
    REG_GSU_R15 = gsu_prog_addr;        /* the GSU starts */
    while (REG_SFR_L & SFR_GO) {
        if (polls != 0xFFFF)
            polls++;
    }
    REG_SCMR = 0;
    gsu_owns_cart = 0;
    r_c_frames = frame_count - start;
    r_c_polls = polls;
}

/** @brief gsuInit(): 1 on a Super FX cart */
u16 r_present;
/** @brief gsuBusy() right after gsuStartCached(): the job is running */
u16 r_busy_after_start;
/** @brief main-loop frames (WaitForVBlank from ROM) while the GSU ran */
u16 r_frames_during;
/** @brief gsuBusy() after gsuWait(): 0 */
u16 r_busy_after_wait;
/** @brief second job, IRQ on STOP unmasked, I flag clear: frames during it */
u16 r_frames_during_irq;
/** @brief gsu_stop_irqs before the second job, and its increase at the end */
u16 r_stop_irqs_before;
u16 r_stop_irqs_delta;
/** @brief gsuBusy() once gsu_stop_irqs moved: 0, the IRQ comes at STOP */
u16 r_busy_at_irq;
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

    /* Second job, the same one, with the GSU's IRQ on STOP unmasked
     * (CFGR bit 7 = 0) and the CPU's I flag clear, as any game that uses a
     * timer IRQ has it. */
    irqSetVTimer(200);
    irqEnable(IRQ_VTIMER);
    gsu_cfgr = 0x00;
    gsuCacheLoad(gsu_job, (u16)(gsu_job_end - gsu_job));
    r_stop_irqs_before = gsu_stop_irqs;
    gsuStartCached(0);
    r_frames_during_irq = 0;
    while (gsu_stop_irqs == (u8)r_stop_irqs_before) {
        WaitForVBlank();
        r_frames_during_irq++;
    }
    r_busy_at_irq = gsuBusy();
    gsuWait();
    r_stop_irqs_delta = (u8)(gsu_stop_irqs - (u8)r_stop_irqs_before);

    /* Third job, the same program run from ROM (RON = 1): the wait loop is
     * in the RAM code window. Game Pak RAM is cleared first, so the ROM run
     * has to write the results again. */
    gsu_cfgr = 0x80;
    gsuJobClearResults();
    gsuSetProgram(gsu_job);
    ramRunRomJob();
    gsuJobReadResults();

    /* Fourth job: the same, the wait loop in C (RAM_CODE). Results cleared
     * again; r_sum / r_marker below come from this run. */
    gsuJobClearResults();
    ram_c_rom_job();
    gsuJobReadResults();
    r_done = 0xD0E5;

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
