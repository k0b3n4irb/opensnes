---
name: B5 fixed32 — chantier COMPLETE (2026-05-21)
description: Multi-day chantier that started as a sketch on 2026-05-20 (preserved orphan from an unstaged submodule file), evolved through 4 phases, surfaced 2 compiler defects (Kl return + Kl shift-by-const), and landed as a complete 16.16 fixed-point math suite in lib/include/snes/fixed32.h + lib/source/fixed32.asm + examples/basics/fix32_orbit/. Archived here for historical context.
type: project
---

## Status: COMPLETE (B5 chantier closed 2026-05-21)

All five fixed32 primitives shipped, validated, and exercised end-to-end
by the `examples/basics/fix32_orbit/` demo example. Suite passes 289/289
with the new example + functional probe in the canonical `--quick` path.

| Phase | API | Cost | Implementation |
|-------|-----|------|----------------|
| 1 | `FIX32/UNFIX32/FIX32_FRAC/FIX32_MAKE` macros + `fix32Abs/Clamp/Min/Max` | 0 (inline) | `lib/include/snes/fixed32.h` |
| 2 | `fix32Mul` (16.16 × 16.16 → 16.16) | ~280 cyc | `lib/source/fixed32.asm` |
| 3 | `fix32Div` (16.16 / 16.16 → 16.16) | ~1500 cyc | 48-iter long divide |
| 4a | `fix32Lerp` | ~290 cyc | inline C using fix32Mul |
| 4b | `fix32Sin / fix32Cos` | ~25 cyc each | asm LUT lookup + xba |

**32 cases validated bit-exact across the 5 primitives** (3 mul + 6 div +
7 lerp + 9 sin/cos + the implicit phase 1 inline cases). Plus the orbit
example proves end-to-end composition works in the canonical
"trig-driven motion" pattern.

## Compiler defects surfaced and resolved

The chantier exposed two latent qbe defects that no existing test had
hit because no prior C code returned s32:

1. **Kl return convention incomplete** — `long bar(long, long)` only
   returned the low 16 bits. FIXED in qbe `3e79c8c` via a global
   `tcc__retval_hi` for the high half. Documented at
   [[cc65816_kl_return_incomplete]].

2. **Kl shift-by-constant loses high half** — `(s32)X << 8` codegen
   destroys A then tries to recover the original from an unstored
   stack slot. WORKED AROUND by writing fix32Sin/Cos in asm. Properly
   FIXABLE in qbe by spilling the operand before the destructive ASL
   sequence. Documented at [[cc65816_kl_shift_high_half]].

Both defects are real chantier-fodder for future qbe work — the second
in particular is the kind of cliff a new contributor could hit anywhere
they shift a `long` by a constant. The tech notes preserve repros so
the next instance is diagnosed in minutes, not days.

## Original sketch (2026-05-20)

The chantier started from an untracked orphan found in
`tools/opensnes-emu/examples/basics/fix32_orbit/main.c` (wrong directory
— examples live in the parent repo, not the test tool's tree). No git
history, no doc references. The original code did a HORIZONTAL motion
hack (no trig, just a linear scroll) because fixed32 didn't exist yet:

```c
/* Original 2026-05-20 sketch — horizontal-only motion */
int main(void) {
    fixed32 angle32 = 0;
    fixed32 vel = FIX32(1) >> 2;  /* 0.25 step per frame */
    /* ... setup ... */
    while (1) {
        angle32 += vel;
        /* Sprite X = 112 + (angle32 >> 16) mod 64 */
        s16 sx = (s16)(112 + ((s16)(angle32 >> 16) & 0x3F));
        oamSet(0, sx, 96, 0x0010, 0, 3, 0);
        oamSetSize(0, OBJ_LARGE);
        WaitForVBlank();
    }
}
```

The companion assets (`orbiter` tile, `palorbiter`) were referenced via
`extern` but never authored. The chantier replaced the sketch with a
real circular orbit using fix32Sin/Cos and an inline 8×8 tile:

```c
/* Shipped 2026-05-21 — examples/basics/fix32_orbit/main.c */
fixed32 g_angle  = 0;
fixed32 g_omega  = FIX32(1);
fixed32 g_radius = FIX32(60);
while (1) {
    g_angle  = g_angle + g_omega;
    u8 a     = (u8)UNFIX32(g_angle);
    fixed32 ox = fix32Mul(g_radius, fix32Cos(a));
    fixed32 oy = fix32Mul(g_radius, fix32Sin(a));
    s16 sx = 128 + (s16)UNFIX32(ox);
    s16 sy = 96  + (s16)UNFIX32(oy);
    oamSet(0, sx, sy, 0, 0, 3, 0);
    WaitForVBlank();
}
```

## Outstanding follow-ups (separate chantiers)

These were considered in-scope at the start but were either deferred or
moved to dedicated chantiers because they don't gate B5's usefulness:

- **16-bit sine LUT for sub-1/256 precision**: the current fix32Sin
  lifts the 8.8 LUT, so the bottom 8 bits of the fractional field are
  always zero. Replacing with a 256-entry 16-bit LUT (+512 bytes ROM)
  would give 16 bits of fractional precision. Open if/when an example
  needs it.

- **Fix the qbe Kl shift-by-constant bug**: the workaround in
  fix32Sin/Cos asm is a Band-Aid. The proper fix is in qbe's Oshl/Oshr
  Kl path (~10 lines of C). Open if more C code hits the same
  pattern.

- **fix32Sqrt / fix32Atan2**: not in B5's MVP scope. The existing 8.8
  `fixSqrt` and `atan2_8` cover most needs; extend to 16.16 when an
  example genuinely needs the range.

## Cross-references

- Shipped example: `examples/basics/fix32_orbit/main.c`
- Lib header: `lib/include/snes/fixed32.h`
- Lib asm: `lib/source/fixed32.asm`
- Functional probe: `tools/opensnes-emu/test/functional/fix32_orbit.test.mjs`
- Compiler defect notes: [[cc65816_kl_return_incomplete]],
  [[cc65816_kl_shift_high_half]]
- Final commit on develop: `df37da1` (ships the example + bumps the
  emu submodule pointer for the probe)
