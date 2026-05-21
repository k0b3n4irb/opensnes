# B5 fixed32 — orbit example sketch (draft)

Found 2026-05-20 as an untracked orphan in `tools/opensnes-emu/examples/basics/fix32_orbit/main.c` (wrong directory — examples live in the parent repo, not in the test tool's tree). No git history, no doc references. Preserved here as the seed sketch for a future B5 fixed32 chantier example, then deleted from the submodule.

The code uses `fixed32` arithmetic to drive a sprite along a horizontal orbit: angle accumulator + velocity step per frame, fractional bits sliced off the high word to derive screen-space X. Minimal — single sprite, no palette animation, no audio — but exercises the `fixed32`/`FIX32`/`>>` operators end-to-end on a real SNES OAM target. Good baseline fixture if/when B5 lands.

Companion assets (`orbiter` tile data, `palorbiter`) are referenced via `extern u8` and were never authored — would need to ship with the example.

```c
#include <snes.h>
#include <snes/math.h>

extern u8 orbiter[], orbiter_end[];
extern u8 palorbiter[];

int main(void) {
    fixed32 angle32 = 0;
    fixed32 vel = FIX32(1) >> 2;  /* 0.25 step per frame */
    consoleInit();
    WaitForVBlank();
    dmaCopyVram(orbiter, 0x2100, orbiter_end - orbiter);
    dmaCopyCGram(palorbiter, OBJ_CGRAM_BASE, PALETTE_16_SIZE);
    oamInit(OBJ_SIZE8_L32, 1);
    oamSet(0, 112, 96, 0x0010, 0, 3, 0);
    oamSetSize(0, OBJ_LARGE);
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        angle32 += vel;
        /* Sprite X = 112 + (angle32 >> 16) mod 64 */
        s16 sx = (s16)(112 + ((s16)(angle32 >> 16) & 0x3F));
        oamSet(0, sx, 96, 0x0010, 0, 3, 0);
        oamSetSize(0, OBJ_LARGE);
        WaitForVBlank();
    }
    return 0;
}
```

## When B5 starts

- Move this to `examples/basics/fix32_orbit/` in the parent repo (with companion `.pic` + `.pal` assets converted via `gfx4snes`).
- Add a `Makefile` following the canonical pattern (see `.claude/rules/new_example.md`).
- The sketch currently only drives horizontal motion — for a real orbit, swap the linear X derivation for `sinf32(angle32)` + `cosf32(angle32)` once B5 implements those.
- The `& 0x3F` mask is a hack to wrap motion to 64 px — drop it once trig is available.

## B5 chantier progress (2026-05-21)

**Phase 1 — SHIPPED**: `lib/include/snes/fixed32.h`. Type alias + macros
(`FIX32`, `UNFIX32`, `FIX32_FRAC`, `FIX32_MAKE`) + inline helpers
(`fix32Abs`, `fix32Clamp`, `fix32Min`, `fix32Max`). All work via the
compiler's existing Kl support — no asm needed because the operations
stay within the function boundary (stack slots / globals, no Kl return
crossings). `FIX32` macro casts through `u32` before shifting to avoid
the C UB on left-shifting negative signed values.

**Compiler prerequisite — SHIPPED**: Discovered mid-implementation
that cc65816's Kl (32-bit) return path was **incomplete**:

  ```c
  long bar(long a, long b) { return a + b; }
  ```

  generated correct in-frame 32-bit addition (sum_lo at 18,s, sum_hi
  at 20,s) but the epilogue only carried the LOW 16 to A. The caller
  read the high half from uninitialised stack.

  Resolved 2026-05-21 (qbe commit `3e79c8c` on
  `fix/a6-a7-leaf-opt-kl-frameless`) by adding a global
  `tcc__retval_hi` (declared in `templates/crt0.asm`'s `.registers`
  RAMSECTION). Convention:
  - Callee: `emit_load_high(ret, fn, 0); sta.b tcc__retval_hi;
    emitload(ret, fn); rtl` (low in A, high in tcc__retval_hi)
  - Caller: after JSL, the existing `emitstore(low)` is followed by
    `lda.b tcc__retval_hi; emit_store_high(i->to, fn)`

  Mirrors the existing `mul32_hi` / `div32_qh` pattern from runtime
  helpers but works for arbitrary C functions. Test repro in
  `.claude/notes/tech/cc65816_kl_return_incomplete.md`.

**Phase 2 — SHIPPED**: `lib/source/fixed32.asm` ships `fix32Mul`
(16.16 fixed-point multiply). Algorithm via the algebraic decomposition
`(a*b) >> 16 = ml1 + ml2 + ll_hi + (hh_lo << 16)`:
  - 3 × unsigned 16×16→32 partial products (a_l·b_l, a_l·b_h, a_h·b_l)
  - 1 × unsigned 16×16→16 (low half of a_h·b_h)
  Each 16×16→32 uses 4 hardware 8×8 multiplies = ~280 cycles total.

  Sign-magnitude: XOR signs, take absolute values, multiply unsigned,
  negate result if XOR sign bit set.

  Validated against 5 cases (smoke test in `/tmp/fix32_test/main.c`,
  ran via mesen2-rpc): 2.5×4=10, -1×2=-2, 0.5×0.5=0.25, 100×200=20000,
  -3×-4=12. All bit-exact. 285/285 full suite green; no regressions
  in any of the 50+ examples (none previously used Kl returns).

**Phase 3-4 — Deferred (still)**:
- `fix32Div` — unsigned 32/32 via existing `tcc_udivmod32`, plus sign
  handling. ~100 lines of asm, straightforward now that the Kl
  return convention exists.
- `fix32Sin` — extended sin LUT or interpolation from the existing
  8.8 LUT. Design choice between memory (~512 bytes for a 16-bit
  LUT) and CPU (linear interp from the 8.8 LUT).
- `fix32Lerp` — `a + ((b-a) * t) >> 16` where t is fixed32 in [0,1].
  Uses `fix32Mul` once it's in place. ~50 lines of C (compiler
  emits the Kl arithmetic; no new asm needed).

The orbit example (sketch in this note) is the canonical end-to-end
fixture once phase 3 (Sin) lands — replace the horizontal-motion
hack with `fix32Mul(radius, fix32Sin(angle))` / `fix32Cos(angle)`.

## Source

Originally drafted ~2026-05-16, found unstaged in submodule on
2026-05-20. B5 chantier opened 2026-05-21 with phase 1 shipping
that day; phases 2-4 blocked on a separate compiler chantier.
