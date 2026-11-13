/*
 * A leaf too big for the direct page (2026-11-04): its costliest temps go to
 * tcc__lf, the rest to a stack frame (leaf_split in compiler/qbe/w65816/
 * emit.c). Until then such a leaf had all of its temps on the stack.
 *
 * Three leaves needing 17, 21 and 25 words (the stack-only colouring's count)
 * accumulate 32-bit values through a loop (with the 20 words of tcc__lf since
 * 2026-11-13, leaf17 fits it whole and the two others are split). Their results are checked against
 * values computed on the host (test_leaf_split.py), and leaf21 also runs in
 * the NMI callback with other arguments, counting every wrong result on
 * either side: the split frame is half in the direct page, half on the
 * stack, and the NMI has its own page and the same stack.
 */
#include <snes.h>

const u32 ktab[64] = {
    0x00010003UL, 0x7FFF0001UL, 0x0000FFFFUL, 0x12345678UL,
    0xFEDCBA98UL, 0x00008000UL, 0x80000000UL, 0x0F0F0F0FUL,
    11, 22, 33, 44, 55, 66, 77, 88,
};
u32 rr[8];
u32 res17[4], res21[5], res25[6];
u32 want_nmi[5], nmi_rr[5];
u16 bad_main, bad_nmi, runs_main, runs_nmi, done;

void leaf17(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d;
}

void leaf21(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3, e = a + 4;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; e += d; }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d; rr[4] = e;
}

/* the same as leaf21, for the NMI: its own output, so that the main loop's
 * rr[] is not overwritten behind its back */
void leaf21n(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3, e = a + 4;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; e += d; }
    nmi_rr[0] = a; nmi_rr[1] = b; nmi_rr[2] = c; nmi_rr[3] = d; nmi_rr[4] = e;
}

void leaf25(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3, e = a + 4, f = a + 5;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; e += d; f += e; }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d; rr[4] = e; rr[5] = f;
}

static u16 same(const u32 *x, const u32 *y, u16 n) {
    u16 k;
    for (k = 0; k < n; k++)
        if (x[k] != y[k])
            return 0;
    return 1;
}

void in_nmi(void) {
    if (done)
        return;
    leaf21n(0x00ABCDEFUL, 0xFFFFFFF0UL, 37);
    if (!same(nmi_rr, want_nmi, 5))
        bad_nmi++;
    runs_nmi++;
}

int main(void) {
    u16 k;

    consoleInit();
    leaf17(0x00010000UL, 0x0000FFFFUL, 50);
    for (k = 0; k < 4; k++) res17[k] = rr[k];
    leaf21(0x7FFFFFFFUL, 3, 70);
    for (k = 0; k < 5; k++) res21[k] = rr[k];
    leaf25(0xFFFF0001UL, 0x00012345UL, 129);
    for (k = 0; k < 6; k++) res25[k] = rr[k];
    leaf21n(0x00ABCDEFUL, 0xFFFFFFF0UL, 37);
    for (k = 0; k < 5; k++) want_nmi[k] = nmi_rr[k];

    nmiSet(in_nmi);
    setScreenOn();
    while (runs_main < 24) {
        leaf21(0x7FFFFFFFUL, 3, 70);
        if (!same(rr, res21, 5))
            bad_main++;
        WaitForVBlank();
        leaf25(0xFFFF0001UL, 0x00012345UL, 129);
        if (!same(rr, res25, 6))
            bad_main++;
        runs_main++;
    }
    done = 0xBEEF;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
