---
name: cc65816 Kl shift-by-constant — high half reads unstored stack slot
description: Discovered 2026-05-21 while implementing fix32Sin. The qbe codegen for `(s32)small_val << 8` (where small_val came from a memory load that wasn't pre-spilled) computes the low half by mutating A then tries to recover the original value from a stack slot that was never written. Read garbage, return broken. Workaround for fix32Sin: write in asm. Real fix: qbe should spill the original before destructive shifts on Kl temps.
type: tech
---

# cc65816 Kl shift-by-constant — high half reads stale stack slot

Found while writing `fix32Sin` as a one-line C inline:

```c
inline fixed32 fix32Sin(u8 angle) {
    return (fixed32)((u32)(s32)fixSin(angle) << 8);
}
```

For `fixSin(64) = 256` (= 1.0 in 8.8), this should produce
`0x00010000` (= 1.0 in 16.16). Instead it produced `0x00000000`.

## Reproduction

Compile `/tmp/test_sin.c`:

```c
#include <snes/fixed32.h>
fixed32 r;
void test(void) { r = fix32Sin(64); }
```

The qbe-emitted `fix32Sin` body:

```asm
fix32Sin:
    ; ... prologue + arg setup ...
    lda 16,s
    tax
    lda.l $0000,x     ; A = sine_table[angle] = 0x0100 for angle=64
    asl a             ; \
    asl a             ;  |
    asl a             ;  |  shift A left 8 (= "* 256" or "<< 8")
    asl a             ;  |  After 8 ASLs of 0x0100:
    asl a             ;  |    0x0100 → 0x0200 → 0x0400 → ... → 0x8000 → 0x0000
    asl a             ;  |  HIGH BYTE 0x01 IS LOST.
    asl a             ;  |
    asl a             ; /
    sta 24,s          ; result_lo = 0 (correct mod 2^16, but high half needs special handling)
    lda 22,s          ; ← BUG: reads stack slot that was NEVER written
    xba
    and.w #$00FF
    sta 26,s          ; result_hi = junk
```

The codegen treats `<< 8` of a Kl value as two halves:
- **result_lo**: `(orig_lo) << 8` in 16-bit accumulator (high byte of `orig` is lost into bit 16+).
- **result_hi**: should be `(orig_lo >> 8) | (orig_hi << 8 sign-extended)`.

For positive values (sign-extended high = 0), result_hi should be just
`orig_lo >> 8`. The codegen attempts to read the ORIGINAL `orig_lo`
from `22,s` to compute this... but never stored anything there. The
8 ASLs on A destroyed the original value in A, and the codegen
forgot to spill A before the shift.

## Visible impact

Any C code doing `(s32)small_val << k` where small_val came from a
direct memory load (without going through a stack temp) produces wrong
results for the high half. Only the LOW half is correct.

Patterns that hit this:
- `(s32)fixSin(angle) << 8` — fix32Sin
- `(s32)getU16() << 16` — promoting u16 → upper half of s32
- Likely any `(s32)expression << constant` where `expression` is a
  value-producing function call or memory access.

Patterns that DON'T hit this (worked fine):
- `(s32)variable * 256` — IF the constant isn't power-of-2 (compiler
  emits a real multiply call to tcc_mul32, which handles s32
  correctly).
- BUT: `* 256` IS reduced to `<< 8` by the constant-fold pass and
  hits the same bug. Confirmed.
- `(s32)variable + variable2` — Kl add doesn't use the destructive
  shift path.

## Workaround applied for fix32Sin / fix32Cos

Implemented in `lib/source/fixed32.asm` as direct asm:

```asm
fix32Sin:
    php
    rep #$30
    ; ... look up sine_table[angle] into A ...
    sta.w f32_res_lo       ; SPILL the original value BEFORE shifting
    and #$00FF
    xba                    ; A = (orig_lo & 0xFF) << 8 (= shift-left-8 of low byte)
    sta.w f32_res_lo       ; result_lo

    lda.w f32_res_lo + 0   ; (re-load original via the spill)
    ; ... compute result_hi from orig_hi byte with sign extension ...
```

`xba` is the 1-cycle 65816 "exchange B and A" — does the byte swap
that `<< 8` simulates. Combined with pre-spilling the original
value, this is both faster (no 8 ASLs) and correct.

## Proposed qbe fix

The bug is in the Kl shift-by-constant codegen path (`emit.c`,
the Oshl / Oshr cases for Kl class). When the shift amount is
exactly 8 (or another byte-aligned amount), the codegen tries an
optimised path:
- low_half_shifted = low << amount (in 16-bit A)
- high_half = (low >> (16-amount)) | (high << amount) ← needs ORIGINAL low

The codegen emits `lda <high>,s` before the destructive ASL and
spills, but FORGETS to spill `<low>` to a known slot when low
came from a non-spilled source (RTmp without an assigned slot,
or a constant-pool slot that's been reused).

The fix: in the Kl shift codegen, ALWAYS spill the LOW half to a
known slot before the destructive ASL sequence. Then the high-half
computation re-loads from that slot.

Cost: one extra store + load per Kl-shift-by-constant. Negligible
vs the correctness gain.

## When this rule does NOT apply

- Shifts BY a variable amount (not constant) take a different
  codegen path (loop or call to a helper); they spill operands
  correctly.
- Shifts by amounts >= 16 are handled differently (the low half is
  often just zero or copied from another register).
- Bitwise AND / OR / XOR don't have this issue — they don't destroy
  the operand the same way ASL does.

## Cross-references

- Workaround: `lib/source/fixed32.asm` (fix32Sin / fix32Cos)
- The original chantier where this surfaced:
  [[b5_fix32_orbit_sketch]]
- The first Kl-related bug found and fixed in qbe:
  [[cc65816_kl_return_incomplete]]
- qbe emit.c around line 2400-2700 (the Omul/Oshl/Oshr handlers)
