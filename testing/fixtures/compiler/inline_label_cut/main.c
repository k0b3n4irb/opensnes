/*
 * The label of a copied block, cut where the assembler reads a size suffix.
 *
 * The inliner names each block it copies inl<n>_<callee>.<index>.<block>,
 * the block part cut at 40 characters. A static's name is name.<unit>, and a
 * copy of a function that already holds copies puts a whole label in its
 * block part, so three levels down the cut can end right after a dot and
 * one letter: `.b` (of body), `.w` (of while_cond, while_body, while_join),
 * `.l` (of logic_right, logic_join). The assembler reads `jmp @x.l` as the
 * label x with a size suffix: x does not exist, and the link fails.
 *
 * The three statics below are `static inline` (copied at every call site)
 * and nest three deep into run(): their names are six letters long, the
 * length that makes the cut of the third level fall right after each of
 * `.b`, `.w` and `.l` (devtools/compiler-tests/cases/inline_label_cut pins
 * that the repaired labels are there, so a change of numbering that moves
 * the cut fails there instead of letting this ROM pass for nothing).
 *
 * run() sweeps from the main loop and from the NMI callback; each side
 * folds its results into a checksum the test compares with the same C
 * computed on the host (test_inline_label_cut.py).
 */
#include <snes.h>

u16 sum_main, sum_nmi, want_nmi, bad_main, bad_nmi;
u16 runs_main, runs_nmi, done;

/* level 1: a loop and a && (blocks body, while_*, logic_*) */
static inline u16 bitsum(u16 x) {
    u16 s = 0;
    while (x) {
        if ((x & 1) && s < 900)
            s = (u16)(s + x);
        x >>= 1;
    }
    return s;
}

/* level 2: two copies of bitsum */
static inline u16 pairer(u16 x) {
    return (u16)(bitsum(x) + bitsum((u16)(x ^ 0x55)));
}

/* level 3: two copies of pairer, each holding two of bitsum */
static inline u16 f(u16 x) {
    return (u16)(pairer(x) ^ pairer((u16)(x + 3)));
}

u16 run(u16 x) {
    return (u16)(f(x) + f((u16)(x * 7)));
}

static u16 sweep(u16 from, u16 n) {
    u16 i, acc = 0;
    for (i = 0; i < n; i++)
        acc = (u16)(acc * 3 + run((u16)(from + i * 97)));
    return acc;
}

void in_nmi(void) {
    if (done)
        return;
    sum_nmi = sweep(5, 4);
    if (sum_nmi != want_nmi)
        bad_nmi++;
    runs_nmi++;
}

int main(void) {
    u16 first;

    consoleInit();
    want_nmi = sweep(5, 4);
    first = sweep(0, 64);
    sum_main = first;
    nmiSet(in_nmi);
    setScreenOn();
    while (runs_main < 16) {
        if (sweep(0, 64) != first)
            bad_main++;
        WaitForVBlank();
        runs_main++;
    }
    done = 0xBEEF;
    while (1)
        WaitForVBlank();
    return 0;
}
