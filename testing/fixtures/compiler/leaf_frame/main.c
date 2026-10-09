/*
 * A leaf function keeps its temporaries in the direct page (tcc__lf) since
 * 2026-10-09. What makes that safe is that a leaf cannot be re-entered on
 * the same direct page: it calls nothing, and the NMI handler — the only
 * thing that runs C behind the main program's back — works on its own
 * page (tcc__nmi_registers), which mirrors tcc__lf.
 *
 * This ROM runs the same leaf from the main loop and from the NMI
 * callback, with different arguments, and counts every wrong result on
 * either side. If the NMI page stopped covering tcc__lf, the callback's
 * frame would land on whatever lies after the page; if the callback ran on
 * the main page, the two would trample each other.
 */
#include <snes.h>

u16 tab[16];
u16 bad_main, bad_nmi, runs_main, runs_nmi, done;
u16 want_main, want_nmi;

/* a loop long enough to be worth interrupting; 5 temps, no call */
u16 leaf(u16 a, u16 b) {
    u16 i, s = 0, t = a;
    for (i = 0; i < b; i++) {
        s += tab[i & 15] ^ t;
        t += 3;
    }
    return s;
}

void in_nmi(void) {
    if (done)
        return;
    if (leaf(7, 40) != want_nmi)
        bad_nmi++;
    runs_nmi++;
}

int main(void) {
    u16 i;

    consoleInit();
    for (i = 0; i < 16; i++)
        tab[i] = i * 257 + 1;
    want_main = leaf(100, 200);
    want_nmi = leaf(7, 40);
    nmiSet(in_nmi);
    setScreenOn();
    while (runs_main < 24) {
        if (leaf(100, 200) != want_main)
            bad_main++;
        WaitForVBlank();
        if (leaf(100, 200) != want_main)
            bad_main++;
        runs_main++;
    }
    done = 0xBEEF;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
