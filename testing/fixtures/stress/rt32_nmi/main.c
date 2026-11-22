/*
 * The 32-bit runtime against interrupts (stress fixture, 2026-11-08).
 *
 * The main thread runs 32-bit multiplies, divides and remainders in a loop
 * that never waits for VBlank, so every NMI is a lag frame and runs the
 * nmiSetLag callback at whatever instruction the main thread was. The
 * callback does 32-bit multiplies and divides of its own. An H-timer IRQ
 * (irq.asm, every scanline) interrupts both the main thread and the
 * runtime's waits on the hardware multiplier.
 *
 * Every result is compared with the same expression folded by the compiler
 * (the operands are volatile at run time, constants at compile time).
 * Before the fix (crt0.asm, mul32.asm, div32.asm) the callback's helper
 * call overwrote the main thread's hardware multiplier state, its result
 * slots (mul32_hi, div32_qh...) and the divide's working words: hundreds of
 * wrong results in 30 000 iterations of each.
 *
 * Read by rt32_nmi.toml: bad counts 0, iteration counts, and "seen" flags
 * that prove the callback and the IRQ ran while the loop did.
 */
#include <snes.h>

#define ITERS 10000u

/* Results, read by the manifest. */
u16 r_bad_mul;      /* main: wrong 32x32 products                  -> 0 */
u16 r_bad_mul16;    /* main: wrong u16 x u16 -> u32 products       -> 0 */
u16 r_bad_div;      /* main: wrong u32 quotients (16-bit divisor)  -> 0 */
u16 r_bad_rem;      /* main: wrong u32 remainders                  -> 0 */
u16 r_bad_sdiv;     /* main: wrong s32 quotients (32-bit divisor)  -> 0 */
u16 r_iters;        /* main iterations run                    -> ITERS */
u16 r_cb_bad;       /* callback: wrong results                     -> 0 */
u16 r_cb_seen;      /* 1 when the callback ran at least 100 times      */
u16 r_irq_seen;     /* 1 when the IRQ ran at least 10 000 times        */
u16 r_done;         /* 0xC0DE at the end                               */

/* The callback's operands and counters. */
volatile u32 cb_a = 0x00012345UL, cb_b = 0x00000777UL;
volatile u32 cb_c = 0x87654321UL, cb_d = 0x00010003UL;
volatile s32 cb_e = -1234567L, cb_f = 89L;
volatile u16 cb_count;
volatile u16 cb_bad;
volatile u32 cb_res;

/* Counted by the IRQ handler (irq.asm), 32 bits. */
volatile u32 irq_count;
extern void rt32IrqHandler(void);

/* The main thread's operands. */
volatile u32 ma = 0x00003039UL, mb = 0x00010001UL;
volatile u16 mc = 0xBEEF, md = 0x1234;
volatile u32 na = 0x12345678UL, nb = 0x00001001UL;
volatile s32 sa = -305419896L, sb = 0x00012345L;
volatile u16 jitter;

static void tick(void) {
    cb_res = cb_a * cb_b;
    if (cb_res != 0x00012345UL * 0x00000777UL) cb_bad++;
    cb_res = cb_c / cb_d;
    if (cb_res != 0x87654321UL / 0x00010003UL) cb_bad++;
    cb_res = cb_c % cb_d;
    if (cb_res != 0x87654321UL % 0x00010003UL) cb_bad++;
    if (cb_e / cb_f != -1234567L / 89L) cb_bad++;
    cb_count++;
}

int main(void) {
    u16 i;
    u16 bad_mul = 0, bad_mul16 = 0, bad_div = 0, bad_rem = 0, bad_sdiv = 0;

    consoleInit();
    nmiSetLag(tick);
    irqSet((void *)rt32IrqHandler);
    irqSetHTimer(200);
    irqEnable(IRQ_HTIMER);
    setScreenOn();

    for (i = 0; i < ITERS; i++) {
        u16 k;
        /* A pause of varying length, so that the NMI does not fall on the
         * same operation frame after frame (the loop is periodic). */
        for (k = (i * 37u) & 63u; k != 0; k--) jitter++;
        if (ma * mb != 0x00003039UL * 0x00010001UL) bad_mul++;
        if ((u32)mc * md != 0xBEEFUL * 0x1234UL) bad_mul16++;
        if (na / nb != 0x12345678UL / 0x00001001UL) bad_div++;
        if (na % nb != 0x12345678UL % 0x00001001UL) bad_rem++;
        if (sa / sb != -305419896L / 0x00012345L) bad_sdiv++;
    }

    irqDisable();
    nmiSetLag(NULL);
    r_bad_mul = bad_mul;
    r_bad_mul16 = bad_mul16;
    r_bad_div = bad_div;
    r_bad_rem = bad_rem;
    r_bad_sdiv = bad_sdiv;
    r_iters = i;
    r_cb_bad = cb_bad;
    r_cb_seen = cb_count >= 100 ? 1 : 0;
    r_irq_seen = irq_count >= 10000UL ? 1 : 0;
    r_done = 0xC0DE;
    while (1) { }
    return 0;
}
