/*
 * A7 runtime-correctness fixture — 32-bit (s32/u32, QBE Kl class) arithmetic.
 *
 * Computes a battery of 32-bit operations into WRAM globals, which the harness
 * (test_a7_32bit.py, via `luna state --assert`) checks against the expected
 * values. This is the RUNTIME gate the A7 chantier needs: the static C->ASM
 * pattern checks pass even on a half-correct Kl implementation, so only an
 * actual execution check proves the codegen.
 *
 * Globals live in bank $00 WRAM (< $2000), so `--assert 00:<off>=<bytes>` reads
 * them. u32 is little-endian: value V -> bytes V&FF, V>>8, V>>16, V>>24.
 */
#include <snes.h>

/* Results — kept as explicit u32/s32/u16 so each Kl op path is exercised. */
u32 r_add;     /* 0x0000FFFF + 1            -> 0x00010000 (carry low->high) */
u32 r_sub;     /* 0x00010000 - 1            -> 0x0000FFFF (borrow high->low) */
u32 r_mul;     /* 0x00001234 * 0x10         -> 0x00012340 */
u32 r_mulhi;   /* 0x00010000 * 0x10         -> 0x00100000 (result in high half) */
u32 r_shl;     /* 1u << 20                  -> 0x00100000 */
u32 r_shr;     /* 0x80000000 >> 4 (logical) -> 0x08000000 */
u32 r_sar;     /* (s32)-256 >> 4 (arith)    -> 0xFFFFFFF0 */
u32 r_div;     /* 0x00100000 / 0x10         -> 0x00010000 */
u32 r_mod;     /* 0x00100003 % 0x10         -> 0x00000003 */
u32 r_and;     /* 0x0F0F0F0F & 0x00FFFF00   -> 0x000F0F00 */
u32 r_or;      /* 0x0F0F0F0F | 0x00FFFF00   -> 0x0FFFFF0F */
u32 r_xor;     /* 0x0F0F0F0F ^ 0x00FFFF00   -> 0x0FF0F00F */
u16 r_cmp;     /* (0x10000u > 0xFFFFu)      -> 1 (high-half-aware compare) */
/* Latent siblings of the Osar fold bug: signed Kl ops on negative 32-bit consts. */
u32 r_sdiv;    /* (s32)-256 / 16  (signed)  -> 0xFFFFFFF0 (-16) */
u32 r_smod;    /* (s32)-257 % 16  (signed)  -> 0xFFFFFFFF (-1) */
u16 r_slt;     /* ((s32)-1 < 0)             -> 1 (signed compare) */
u16 r_sgt;     /* ((s32)-5 > (s32)3)        -> 0 (signed compare) */
u32 r_sar8;    /* (s32)-1 >> 8    (arith)   -> 0xFFFFFFFF */

int main(void) {
    u32 a, b;
    s32 s;

    a = 0x0000FFFFu; r_add = a + 1u;
    a = 0x00010000u; r_sub = a - 1u;
    a = 0x00001234u; r_mul = a * 0x10u;
    a = 0x00010000u; r_mulhi = a * 0x10u;
    a = 1u; r_shl = a << 20;   /* a is u32 — shift the 32-bit operand, not a 16-bit `1u` */
    a = 0x80000000u; r_shr = a >> 4;
    s = -256;        r_sar = (u32)(s >> 4);
    a = 0x00100000u; r_div = a / 0x10u;
    a = 0x00100003u; r_mod = a % 0x10u;
    a = 0x0F0F0F0Fu; b = 0x00FFFF00u;
    r_and = a & b;
    r_or  = a | b;
    r_xor = a ^ b;
    r_cmp = (0x00010000u > 0x0000FFFFu) ? 1u : 0u;
    s = -256; r_sdiv = (u32)(s / 16);
    s = -257; r_smod = (u32)(s % 16);
    s = -1;   r_slt  = (s < 0) ? 1u : 0u;
    s = -5;   r_sgt  = (s > 3) ? 1u : 0u;
    s = -1;   r_sar8 = (u32)(s >> 8);

    consoleInit();
    setScreenOn();
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
