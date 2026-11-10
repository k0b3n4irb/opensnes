/*
 * A ROM whose screen is its own timing (stress fixture, 2026-11-10).
 *
 * The NMI callback latches the beam counters and reads the scanline it runs on (OPVCT), then paints the backdrop
 * (CGRAM colour 0) with a colour computed from that line. The next frame shows it on the whole screen. A second
 * emulator, or luna with the NMI fault of one line (225 -> 226), shows another colour: a stabilised game screen cannot
 * show a one-line shift of the NMI, this one can. The comparison is the kit's mesen2_screens, from the corpus
 * (testing/mesen2_corpus.toml); the number itself is also kept in r_line for luna test manifests.
 *
 * What it reads is the line the CALLBACK runs on, not the line the NMI is taken on: the handler's prologue comes first,
 * so luna reads 226 here (the NMI is taken on 225). A longer handler moves it; that is a measure, pinned in the manifest.
 *
 * The colour is a function of the line that changes for every line: R from bits 0-4, G from bits 1-5, B from bits 3-7.
 */
#include <snes.h>

volatile u16 r_line;      /* the scanline the NMI callback ran on (OPVCT, 9 bits)       */
volatile u16 r_count;     /* NMIs seen                                                  */

static void on_nmi(void) {
    u16 lo, hi, line, color;
    (void)REG_SLHV;                 /* latch the counters (the callback is in the vblank: the latch is free) */
    (void)REG_OPVCT;                /* ignore the stale low byte of the previous read ... */
    lo = REG_OPVCT;                 /* ... the latch is read low byte then high byte  */
    hi = REG_OPVCT;
    line = (u16)(lo | ((hi & 1u) << 8));
    r_line = line;
    r_count++;
    color = (u16)((line & 31u) | (((line >> 1) & 31u) << 5) | (((line >> 3) & 31u) << 10));
    REG_CGADD = 0;
    REG_CGDATA = (u8)(color & 0xFF);
    REG_CGDATA = (u8)(color >> 8);
}

int main(void) {
    consoleInit();
    nmiSet(on_nmi);
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
