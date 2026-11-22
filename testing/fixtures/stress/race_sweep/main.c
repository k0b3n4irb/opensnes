/*
 * The interrupt gate's subject (stress fixture, 2026-11-10).
 *
 * probe() does the 32-bit operations of the runtime once, straight through, and stores the results. The NMI lag
 * callback does 32-bit operations of its own with other operands, so a NMI that lands inside a runtime helper
 * and clobbers its state changes a stored result. testing/race_gate.py asks `luna race-sweep` to force an NMI
 * before EVERY instruction of each helper, from one snapshot taken at probe(), and to compare the results with
 * the run that was not interrupted. The fixture itself carries no verdict: the gate does.
 */
#include <snes.h>

volatile u32 ma = 0x00003039UL, mb = 0x00010001UL;
volatile u32 na = 0x12345678UL, nb = 0x00001001UL;
volatile s32 sa = -305419896L, sb = 0x00012345L;

/* The invariant: read by the gate (race_gate.py compares these bytes). */
volatile u32 r_mul, r_div, r_rem;
volatile s32 r_sdiv;
volatile u16 r_done;

/* The callback's own operands: other values, so a clobbered slot gives a different result. */
volatile u32 cb_a = 0x00012345UL, cb_b = 0x00000777UL, cb_c = 0x87654321UL, cb_d = 0x00010003UL;
volatile s32 cb_e = -1234567L, cb_f = 89L;
volatile u32 cb_res;

#ifndef CB_OPS
#define CB_OPS 15
#endif
static void tick(void) {
#if CB_OPS & 1
    cb_res = cb_a * cb_b;
#endif
#if CB_OPS & 2
    cb_res = cb_c / cb_d;
#endif
#if CB_OPS & 4
    cb_res = cb_c % cb_d;
#endif
#if CB_OPS & 8
    cb_res = (u32)(cb_e / cb_f);
#endif
}

/* noinline: the gate warms up at probe_begin and ends at probe_end. */
void probe_begin(void) { r_mul = 0; }
void probe(void) {
    r_mul = ma * mb;
    r_div = na / nb;
    r_rem = na % nb;
    r_sdiv = sa / sb;
}
void probe_end(void) { r_done = 0xC0DE; }

int main(void) {
    consoleInit();
    nmiSetLag(tick);
    setScreenOn();
    WaitForVBlank();   /* the window below then holds no natural NMI: a sweep adds its own */
    probe_begin();
    probe();
    probe_end();
    while (1) { }
    return 0;
}
