/*
 * Proof that forwarding a 4-byte (Kl) pointer PARAMETER to another call is
 * CORRECT on cc65816/QBE — despite the `lda 8,s / pha / lda 8,s / pha` codegen
 * that looks like it reads the same word twice. See
 * cc65816_kl_pointer_param_forward_nonbug.md.
 *
 * Build as a ROM (LIB_MODULES := console) and run under luna:
 *   luna state --sym r.sym -n 1000000 --peek got_lo:1 --peek got_hi:1 --peek got_bank:1 r.sfc
 * Forwarding $01:9303 yields lo=03 hi=93 bank=01 — the full 24-bit pointer,
 * bank byte included, arrives intact at the callee.
 *
 * Compile-only view (bin/cc65816 fwd.c -o fwd.asm) shows the two `lda 8,s`;
 * the pha between them shifts SP by 2, so the second reads the LOW word.
 */
#include <snes.h>

u8 got_lo, got_hi, got_bank, got_bankarg;
typedef void (*fp)(void);

/* callee: records the 24-bit pointer it actually received */
void sink(fp cb, u8 bankarg) {
    got_lo      = (u8)((u32)cb & 0xFF);
    got_hi      = (u8)(((u32)cb >> 8) & 0xFF);
    got_bank    = (u8)(((u32)cb >> 16) & 0xFF);
    got_bankarg = bankarg;
}

/* the pass-through under test: a Kl param forwarded by value */
void forward(fp cb) { sink(cb, 0); }

/* a real bank-$01 far pointer value, via a volatile so it is a runtime forward */
volatile u32 src = 0x00019303UL;

int main(void) {
    consoleInit();
    forward((fp)src);          /* expect the callee to receive $01:9303 intact */
    while (1) { }
    return 0;
}
