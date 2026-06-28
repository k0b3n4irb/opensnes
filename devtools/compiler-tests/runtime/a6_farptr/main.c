/*
 * A6 far-pointer runtime MATRIX (Tier 2, Phase A — A1).
 *
 * Each global below is one matrix cell: a pointer DEREF crossed by
 *   {byte, word, long, param/RSlot, phi/cond} × {bank $00, bank $02}.
 * A red cell pinpoints exactly which emit path drops the bank byte — so the
 * Phase-A codegen work is a checklist, not "24 examples regressed, which one?".
 *
 * Sentinels carry distinct patterns so a wrong-bank read is *detectable*:
 *   a6_sentinel (BANK 2): 11 22 33 44 55 66 77 88
 *   s0          (bank 0): A0 A1 A2 A3 A4 A5 A6 A7
 * Pointers are `volatile` → opaque, register-indirect (can't fold to a direct
 * symbol access), so they exercise the deref emit, not constant folding.
 *
 * On qbe 1884a20 (no far deref) the bank-$02 cells read bank-$00 garbage and
 * FAIL (the A6 gap); the bank-$00 cells pass. Phase A turns the bank-$02 cells
 * green one storage form at a time.
 */
#include <snes.h>

extern const u8 a6_sentinel[];                                /* BANK 2 */
const u8 s0[8] = {0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7};   /* bank $00 */

volatile const u8 *p2;
volatile const u8 *p0;

/* one global per matrix cell */
u8  b2_0, b2_7, b0_0, b0_7;   /* byte loads        */
u16 w2_0, w2_2, w0_0;         /* word loads        */
u32 l2_0, l0_0;               /* long loads        */
u8  fp2, fp0;                 /* via function param (RSlot path) */
u8  ph2;                      /* via phi / conditional */
u16 hi2, hi0;                 /* pointer high half (bank carried in the value) */

/* non-static -> not inlined -> forces the pointer through a param (RSlot) slot */
u8 rd_byte(volatile const u8 *p, u8 i) { return p[i]; }

int main(void) {
    p2 = a6_sentinel;
    p0 = s0;

    b2_0 = p2[0];  b2_7 = p2[7];
    b0_0 = p0[0];  b0_7 = p0[7];
    w2_0 = *(volatile const u16 *)(p2 + 0);
    w2_2 = *(volatile const u16 *)(p2 + 2);
    w0_0 = *(volatile const u16 *)(p0 + 0);
    l2_0 = *(volatile const u32 *)(p2 + 0);
    l0_0 = *(volatile const u32 *)(p0 + 0);
    fp2  = rd_byte(p2, 3);
    fp0  = rd_byte(p0, 3);
    { volatile const u8 *pp = (b2_0 == 0x11) ? p2 : p0; ph2 = pp[5]; }
    hi2  = (u16)(((u32)p2) >> 16);
    hi0  = (u16)(((u32)p0) >> 16);

    consoleInit();
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
