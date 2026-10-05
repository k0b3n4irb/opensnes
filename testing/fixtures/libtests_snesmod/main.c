/*
 * libtest_snesmod — what snesmodProcess and the command queue do to the rest
 * of the machine (testing/manifests/libtest_snesmod.toml).
 *
 * The messages are module-volume commands: harmless with no module loaded,
 * and one queue entry each. No soundbank is needed.
 */
#include <snes.h>
#include <snes/snesmod.h>
#include <snes/profile.h>
#include <snes/interrupt.h>

/* the driver's queue indexes (lib/source/snesmod.asm) */
extern u8 spc_fwrite;
extern u8 spc_fread;

u16 r_latch;      /* STAT78 bit 6 after a snesmodProcess that had to wait      -> 0 */
u16 r_lines;      /* scanlines that snesmodProcess spent waiting               -> 5 or 6 */
u16 r_drain;      /* frames to send 4 queued messages, one snesmodProcess each */
u16 r_depth;      /* queue bytes in use after 100 sends with no process        -> 255 */
u16 r_drain_full; /* frames to send what the full queue kept */
u16 r_vline;      /* profileGetScanline() right after a waiting snesmodProcess:
                   * a sane line number (the OPVCT read pointer was not left
                   * shifted)                                                   -> < 262 */
u16 r_irq;        /* V-timer IRQs in the 10 frames after snesmodInit(), the
                   * IRQ having been enabled before it: snesmodInit must
                   * leave NMITIMEN as it found it                              -> 10 */
u16 r_done;       /*                                                           -> 0xBEEF */

volatile u16 irq_count;
extern void irqTestHandler(void);

static u16 depth(void) {
    return (u8)(spc_fwrite - spc_fread);
}

int main(void) {
    u8 i;
    u16 before;

    consoleInit();

    /* a V-timer IRQ armed BEFORE the driver is loaded */
    irqSet((void *)irqTestHandler);
    irqSetVTimer(100);
    WaitForVBlank();
    irqEnable(IRQ_VTIMER);

    snesmodInit();
    setScreenOn();
    irq_count = 0;
    for (i = 0; i < 10; i++)
        WaitForVBlank();
    r_irq = irq_count;
    irqDisable();
    WaitForVBlank();

    /* four messages, then one snesmodProcess: it sends the first and waits
     * for the SPC with the others still queued */
    for (i = 0; i < 4; i++)
        snesmodSetModuleVolume((u8)(100 + i));
    WaitForVBlank();
    (void)REG_STAT78;                       /* clear the latch flag */
    before = profileGetScanline();
    (void)REG_STAT78;                       /* the profile read latched: clear again */
    snesmodProcess();
    r_latch = (u16)(REG_STAT78 & 0x40);
    r_vline = profileGetScanline();
    r_lines = (u16)(r_vline - before);

    r_drain = 1;
    while (depth() != 0 && r_drain < 600) {
        WaitForVBlank();
        snesmodProcess();
        r_drain++;
    }

    /* 100 messages with no process in between: the queue holds 85 */
    for (i = 0; i < 100; i++)
        snesmodSetModuleVolume(i);
    r_depth = depth();
    r_drain_full = 0;
    while (depth() != 0 && r_drain_full < 2000) {
        WaitForVBlank();
        snesmodProcess();
        r_drain_full++;
    }

    r_done = 0xBEEF;
    while (1)
        WaitForVBlank();
    return 0;
}
