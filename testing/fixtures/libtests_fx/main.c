/*
 * libtest_fx — second runtime fixture for the library (coverage lot C).
 *
 * Same contract as testing/fixtures/libtests: every vector asserts the EFFECT of a
 * public function nothing else executes, in a global test_libtest_fx.py
 * reads by symbol, or on luna's PPU / DMA view of the machine.
 *
 *   hdma    hdmaWaveInit, hdmaWaveH, hdmaGetEnabled, hdmaGradient,
 *           hdmaWindowShape
 *   mode7   mode7SetMatrix, mode7SetCenter, mode7Rotate, mode7Transform
 *   snesmod snesmodFlush,
 *           snesmodGetPosition
 *   console consoleInit, nmiSet
 *   combo   irq + SNESMOD: a V-timer IRQ armed before snesmodInit keeps
 *           firing once per frame while snesmodProcess runs, and the
 *           driver leaves STAT78's latch flag clear (testing audit T5)
 *
 * nmiSet takes the bank from the far pointer it is given, so the callback
 * may live in any bank (the header's "must be in bank 0" note is stale; the
 * function that does drop the bank is irqSet — API audit 2026-09-20, B2).
 */

#include <snes.h>
#include <snes/hdma.h>
#include <snes/mode7.h>
#include <snes/snesmod.h>
#include <snes/interrupt.h>
#include <snes/sa1.h>
#include <snes/superfx.h>
#include "soundbank.h"

u16 r_hdma_init;     /* hdmaGetEnabled() after hdmaWaveInit            -> 0 */
u16 r_hdma_wave;     /* ... after hdmaWaveH(6, 0, 8, 4)                -> 0x40 */
u16 r_hdma_setup;    /* ... after hdmaGradient(5) + hdmaWindowShape(4): setup
                      * does not enable                                 -> 0x40 */
u16 r_hdma_both;     /* ... after hdmaEnableMask(ch 5 | ch 4)              -> 0x70 */
u16 r_hdma_chan;     /* hdmaDisable(4) / hdmaEnable(4) take a channel; 8, 0x10 and
                      * 0x40 (0.x masks) are refused and change nothing       -> 1 */
u16 r_m7_rot_sin;    /* m7_sin after mode7Rotate(90): table[64]         -> 127 */
u16 r_chips;         /* plain LoROM: sa1IsReady | gsuIsPresent<<2, both 0           -> 0 */
u16 r_nmi_calls;     /* nmiSet callback invocations over 5 frames       -> 5 */
u16 r_nmi_after;     /* ... 3 more frames after nmiClear                -> 5 */
u16 r_mod_pos;       /* snesmodGetPosition() as a u16: high byte clean  -> lt 0x100 */
u16 r_mod_flush;     /* spc_fread == spc_fwrite after snesmodFlush      -> 1 */
u16 r_hdma_speed;    /* hdma_wave_speed after hdmaWaveSetSpeed(3)       -> 3 */
extern u8 hdma_wave_speed;   /* the wave module's speed byte: internal, no public header declares it */
u16 r_irq_mod;       /* V-timer IRQs over 10 frames of snesmodProcess   -> 10 */
u16 r_mod_latch;     /* STAT78 & 0x40 right after snesmodProcess        -> 0 */
volatile u16 irq_count;              /* counted by irqProbeHandler (irq.asm) */
extern void irqProbeHandler(void);   /* irq.asm, bank 0 */
u16 r_done;          /* reached the end                                 -> 0xBEEF */

extern u8 spc_fread, spc_fwrite;

/* COLDATA gradient: 3 entries then the terminator */
static const u8 grad_table[] = { 80, 0x20 | 4, 80, 0x40 | 8, 64, 0x80 | 12, 0 };
/* window shape: left/right pairs */
static const u8 win_table[] = { 100, 40, 200, 124, 60, 180, 0 };

static volatile u16 nmi_calls;
static void nmiProbe(void) { nmi_calls++; }

int main(void) {
    u8 i;

    consoleInit();

    /* IRQ + SNESMOD (testing audit 2026-10-03, T5): the timer IRQ is armed
     * BEFORE the driver boots. snesmodInit used to end on `lda #$81 / sta
     * $4200` and drop an armed H/V timer (fixed 2026-10-03); the count below
     * is taken once the module plays, so a dropped IRQ reads 0, not 10. */
    irq_count = 0;
    irqSet((void *)irqProbeHandler);
    irqSetVTimer(120);
    WaitForVBlank();
    irqEnable(IRQ_VTIMER);

    /* SNESMOD first: boot the driver and start the module so the position
     * and the command queue mean something. */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodLoadModule(MOD_POLLEN8);
    snesmodPlay(0);
    snesmodSetModuleVolume(90);
    /* Asymmetric, non-zero arguments on purpose: a fade to 45 at speed 3.
     * snesmodFadeVolume read the wrong stack byte for the target (always 0),
     * and its only test used target 0. spc_pr holds the last command sent. */
    snesmodFadeVolume(45, 3);
    snesmodFlush();
    r_mod_flush = (spc_fread == spc_fwrite) ? 1 : 0;
    for (i = 0; i < 30; i++) { WaitForVBlank(); snesmodProcess(); }
    r_mod_pos = (u16)snesmodGetPosition();
    /* Ten frames of the driver's per-frame work with the V-timer at line
     * 120: one IRQ per frame, none lost to the driver's `$4200` writes or
     * its scanline wait. Then the latch flag: until 2026-10-03 the wait
     * latched the counters ($2137), which raised STAT78 bit 6 — the flag
     * the Super Scope code takes for a shot (anomie-timing 626b31bd887c2581:
     * set when latched, cleared on read; snesdev-wiki 6001605c7d4b1daf).
     * The read here is the first since the previous frame's NMI. */
    irq_count = 0;
    for (i = 0; i < 10; i++) { WaitForVBlank(); snesmodProcess(); }
    r_irq_mod = irq_count;
    snesmodProcess();
    r_mod_latch = REG_STAT78 & 0x40;

    /* nmiSet: five frames of callbacks, none after nmiClear */
    nmi_calls = 0;
    nmiSet(nmiProbe);
    for (i = 0; i < 5; i++) { WaitForVBlank(); snesmodProcess(); }
    nmiClear();
    r_nmi_calls = nmi_calls;
    /* N4: the chip presence getters on a cartridge with no chip (crt0 left
     * sa1_status / superfx_status at 0). gsuInit is not callable here — its GSU state
     * lives in superfx.asm, a SuperFX-only object — superfx_hello runs it. */
    r_chips = (u16)sa1IsReady() | ((u16)gsuIsPresent() << 2);
    nmiSet(0);      /* documented as "disable": used to install a jump to $00:0000 */
    for (i = 0; i < 3; i++) { WaitForVBlank(); snesmodProcess(); }
    r_nmi_after = nmi_calls;

    /* hdma: the wave helper enables its channel; the table helpers only set
     * a channel up and leave enabling to the caller */
    hdmaWaveInit();
    r_hdma_init = hdmaGetEnabled();
    hdmaWaveH(6, 0, 200, 4);                /* amplitude clamps to 60 (header's promise) */
    r_hdma_wave = hdmaGetEnabled();
    hdmaGradient(5, grad_table);
    hdmaWindowShape(4, win_table);
    r_hdma_setup = hdmaGetEnabled();
    hdmaEnableMask((1 << 5) | (1 << 4));
    r_hdma_both = hdmaGetEnabled();
    /* D1 at 1.0: the short names take a channel number; a 0.x mask (any
     * value above 7) is refused and leaves HDMAEN alone */
    hdmaDisable(4);
    r_hdma_chan = (hdmaGetEnabled() == 0x60) ? 1 : 0;
    hdmaEnable(4);
    if (hdmaGetEnabled() != 0x70) r_hdma_chan = 0;
    hdmaEnable(8);                  /* the first refused value */
    hdmaEnable(0x40);               /* 1 << HDMA_CHANNEL_6 left as a mask */
    hdmaDisable(0x10);              /* 1 << HDMA_CHANNEL_4 left as a mask */
    hdmaDisable(0xFF);
    if (hdmaGetEnabled() != 0x70) r_hdma_chan = 0;
    /* hdmaColorGradient on colour 37 (not 0): the index was written as
     * [index, 0] to a register written twice, so every gradient landed on
     * colour 0. Red at the top, blue at the bottom; the last chunk leaves
     * CGRAM[37] near-blue and CGRAM[0] untouched. */
    hdmaColorGradient(3, 37, 0x001F, 0x7C00);
    /* A second gradient table on channel 2: the far pointer carries the bank,
     * asserted on luna's DMA view (a_bank is an asset bank, not 0). */
    hdmaSetup(2, HDMA_MODE_1REG, HDMA_DEST_COLDATA, grad_table);

    /* mode 7: Transform and Rotate go through the PPU multiplier and leave
     * the matrix behind; SetMatrix and SetPivot then write known values the
     * runner reads back from luna's PPU view (signed fields). */
    mode7Init();
    mode7Transform(90, 100);
    mode7Rotate(90);
    mode7SetMatrix(0x0100, 0x0020, (s16)-0x0020, 0x0080);
    mode7SetCenter(64, 48);

    setScreenOn();
    WaitForVBlank();
    hdmaWaveSetSpeed(3);
    r_hdma_speed = hdma_wave_speed;

    r_done = 0xBEEF;
    while (1) { WaitForVBlank(); snesmodProcess(); }
    return 0;
}
