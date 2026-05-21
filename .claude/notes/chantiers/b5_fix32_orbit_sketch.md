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
crossings).

**Phase 2 — BLOCKED on cc65816 Kl return convention**:
`fix32Mul(fixed32, fixed32) → fixed32` cannot ship today. Discovered
mid-implementation that cc65816's Kl (32-bit) return path is
**incomplete**:

  ```c
  long bar(long a, long b) { return a + b; }
  ```

  Generates correct in-frame 32-bit addition (sum_lo at 18,s,
  sum_hi at 20,s) but the function epilogue only emits
  `lda 18,s ; tax ; <teardown> ; txa ; rtl` — **only the low 16 bits
  reach A**. The caller does `lda 4,s` after the call expecting the
  high 16 there, but no convention is implemented for the callee to
  write it there before rtl. The high half reads garbage from the
  caller's frame.

  Test reproduction: `cc65816 /tmp/test_long.c` — see the `bar` and
  `test` functions in the output. The compiler chantier to fix this
  is a prerequisite for B5 phase 2 and any other helper that wants
  to return s32/u32 from C code.

  ASM draft for `fix32Mul` is archived at
  [[b5_fix32mul_asm_draft.asm]] in this directory. It implements
  16.16 multiply via the algebraic decomposition
  `(a*b)>>16 = ml1 + ml2 + ll_hi + (hh_lo << 16)` using three
  16×16→32 unsigned products + one 16×16→16 product. ~250 lines
  of asm, untested because the calling convention can't reach it
  cleanly.

**Phase 3-4 — Deferred**: `fix32Div`, `fix32Sin`, `fix32Lerp` all
depend on phase 2 landing first.

## Source

Originally drafted ~2026-05-16, found unstaged in submodule on
2026-05-20. B5 chantier opened 2026-05-21 with phase 1 shipping
that day; phases 2-4 blocked on a separate compiler chantier.
