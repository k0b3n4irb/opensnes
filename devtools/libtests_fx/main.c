/*
 * libtest_fx — second runtime fixture for the library (coverage lot C).
 *
 * Same contract as devtools/libtests: every vector asserts the EFFECT of a
 * public function nothing else executes, in a global test_libtest_fx.py
 * reads by symbol, or on luna's PPU / DMA view of the machine.
 *
 *   hdma    hdmaWaveInit, hdmaWaveH, hdmaGetEnabled, hdmaGradient,
 *           hdmaWindowShape
 *   mode7   mode7SetMatrix, mode7SetPivot, mode7Rotate, mode7Transform
 *   snesmod snesmodSetSoundTable, snesmodAllocateSoundRegion, snesmodFlush,
 *           snesmodGetPosition
 *   console consoleInitEx, nmiSet
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
#include "soundbank.h"

u16 r_hdma_init;     /* hdmaGetEnabled() after hdmaWaveInit            -> 0 */
u16 r_hdma_wave;     /* ... after hdmaWaveH(6, 0, 8, 4)                -> 0x40 */
u16 r_hdma_setup;    /* ... after hdmaGradient(5) + hdmaWindowShape(4): setup
                      * does not enable                                 -> 0x40 */
u16 r_hdma_both;     /* ... after hdmaEnable(ch 5 | ch 4)              -> 0x70 */
u16 r_m7_rot_sin;    /* m7_sin after mode7Rotate(90): table[64]         -> 127 */
u16 r_nmi_calls;     /* nmiSet callback invocations over 5 frames       -> 5 */
u16 r_nmi_after;     /* ... 3 more frames after nmiClear                -> 5 */
u16 r_mod_pos;       /* snesmodGetPosition() as a u16: high byte clean  -> lt 0x100 */
u16 r_mod_flush;     /* spc_fread == spc_fwrite after snesmodFlush      -> 1 */
u16 r_done;          /* reached the end                                 -> 0xBEEF */

extern u8 spc_fread, spc_fwrite;

/* COLDATA gradient: 3 entries then the terminator */
static const u8 grad_table[] = { 80, 0x20 | 4, 80, 0x40 | 8, 64, 0x80 | 12, 0 };
/* window shape: left/right pairs */
static const u8 win_table[] = { 100, 40, 200, 124, 60, 180, 0 };
/* a streaming sound table: only its address matters to the vector */
static const u8 sound_table[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

static volatile u16 nmi_calls;
static void nmiProbe(void) { nmi_calls++; }

int main(void) {
    u8 i;

    consoleInitEx(0);            /* the documented alias of consoleInit */

    /* SNESMOD first: boot the driver and start the module so the position
     * and the command queue mean something. */
    snesmodInit();
    snesmodSetSoundbank(SOUNDBANK_BANK);
    snesmodSetSoundTable(sound_table);      /* SoundTable asserted by symbol */
    snesmodAllocateSoundRegion(8);          /* before the module: it resizes SPC RAM */
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

    /* nmiSet: five frames of callbacks, none after nmiClear */
    nmi_calls = 0;
    nmiSet(nmiProbe);
    for (i = 0; i < 5; i++) { WaitForVBlank(); snesmodProcess(); }
    nmiClear();
    r_nmi_calls = nmi_calls;
    for (i = 0; i < 3; i++) { WaitForVBlank(); snesmodProcess(); }
    r_nmi_after = nmi_calls;

    /* hdma: the wave helper enables its channel; the table helpers only set
     * a channel up and leave enabling to the caller */
    hdmaWaveInit();
    r_hdma_init = hdmaGetEnabled();
    hdmaWaveH(6, 0, 8, 4);
    r_hdma_wave = hdmaGetEnabled();
    hdmaGradient(5, grad_table);
    hdmaWindowShape(4, win_table);
    r_hdma_setup = hdmaGetEnabled();
    hdmaEnable((1 << 5) | (1 << 4));
    r_hdma_both = hdmaGetEnabled();
    /* hdmaColorGradient on colour 37 (not 0): the index was written as
     * [index, 0] to a register written twice, so every gradient landed on
     * colour 0. Red at the top, blue at the bottom; the last chunk leaves
     * CGRAM[37] near-blue and CGRAM[0] untouched. */
    hdmaColorGradient(3, 37, 0x001F, 0x7C00);

    /* mode 7: Transform and Rotate go through the PPU multiplier and leave
     * the matrix behind; SetMatrix and SetPivot then write known values the
     * runner reads back from luna's PPU view (signed fields). */
    mode7Init();
    mode7Transform(90, 100);
    mode7Rotate(90);
    mode7SetMatrix(0x0100, 0x0020, (s16)-0x0020, 0x0080);
    mode7SetPivot(64, 48);

    setScreenOn();
    WaitForVBlank();
    r_done = 0xBEEF;
    while (1) { WaitForVBlank(); snesmodProcess(); }
    return 0;
}
