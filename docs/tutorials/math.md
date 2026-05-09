# Fixed-Point Math Tutorial {#tutorial_math}

This tutorial covers `<snes/math.h>`: the lib's fixed-point arithmetic,
sin/cos lookup tables, and safe integer multiply/divide. The SNES has
**no floating-point unit** — every smooth movement, every rotation,
every interpolation in your game is built on fixed-point or LUTs.

It assumes you have read the [Graphics](graphics.md) tutorial. The
[Mode 7 tutorial](mode7.md) is the natural pair — Mode 7's matrix
maths goes through `fixSin`/`fixCos`/`fixMul` constantly.

## Why fixed-point on the SNES

Floating point on the SNES is **possible** but **expensive**. cproc /
QBE can emit software-float code (the lib avoids it deliberately —
`PHILOSOPHY.md` calls out "no `printf` in core lib" partly because
formatted-output helpers force software floats), but a single
`float` multiply costs ~500 cycles. At 60 fps with 1369 cycles per
scanline, a few floats per frame is fine; per-sprite or per-particle is
infeasible.

Fixed-point is the canonical alternative: integers that **interpret**
some bits as fractional. With 8.8 fixed-point (16-bit total), the
high byte is the integer part and the low byte is `n/256`. A multiply
is a 16 × 16 → 32-bit integer multiply followed by a shift — the SNES
has hardware for the first part (multiplier registers `$4202`-`$4206`)
and the lib does the shift.

Cost comparison (rough):

- `float` multiply: ~500 cycles.
- `fixMul` 8.8 × 8.8 → 8.8: ~30 cycles.
- `fixSin(angle)`: ~20 cycles (LUT lookup).
- `mul16(a, b)` (16 × 16 → low 16): ~10 cycles (hardware).

Order-of-magnitude: fixed-point is 10–25× faster than software floats.
For real-time game logic, that's the difference between "playable" and
"choppy".

## The 8.8 fixed-point format

The lib's `fixed` type is a 16-bit signed value (`s16`) interpreted as:

| Bits | Meaning | Range |
|---|---|---|
| 15–8 | Integer part (signed) | -128 to 127 |
| 7–0 | Fractional part | 0 to 255 (representing 0.0 to 0.996…) |

So `fixed value = 0x0140` represents `1.25` (integer = 1, fraction =
0x40 = 64/256 = 0.25). A `fixed` with value `0xFF80` represents
`-0.5` (the standard two's-complement sign-extension applies).

Total range: roughly **-128.0 to +127.996**. Precision: **1/256 ≈
0.0039**. Good enough for screen-space coordinates (the SNES
displays 256 × 224, so 8.8 covers the full screen with sub-pixel
precision). Not enough for distances over a few hundred units —
in that case, drop to integer pixel coordinates and reserve fixed
for velocity/acceleration.

## The conversion macros

```c
#include <snes/math.h>

fixed pos = FIX(50);             /* 50.0 → 0x3200 */
fixed half = FIX(1) / 2;         /* 0.5  → 0x0080 */
fixed neg = -FIX(3);             /* -3.0 → 0xFD00 */

s16 ipart = UNFIX(pos);          /* 50  (truncate toward zero) */
s16 ipart_round = UNFIX_ROUND(pos + 128);  /* 51 (round to nearest) */
u8  fpart = FIX_FRAC(pos);       /* 0   (the fractional byte) */

fixed combined = FIX_MAKE(2, 64);   /* 2.25 — int=2, frac=64/256 */
```

Two non-obvious bits:

1. **`UNFIX` truncates toward zero** — same behaviour as integer C
   division. `UNFIX(FIX(-1) / 2)` = `0`, not `-1`. For round-to-nearest,
   use `UNFIX_ROUND(x + 128)` (the `+128` adds half before truncating).
2. **`FIX(x)` is `(x << 8)`** — only safe for `x` in `-128..127`.
   `FIX(200)` overflows the high byte and wraps to `-56.0`. The
   compiler doesn't warn; check your inputs.

## Fixed-point arithmetic

```c
fixed pos = FIX(100);
fixed velocity = FIX(1) / 4;     /* 0.25 */
pos = pos + velocity;             /* 100.25 — addition is plain int+ */
pos = pos - FIX(50);              /* 50.25  — subtraction is plain int- */

fixed scale = FIX(1) / 2;         /* 0.5 */
fixed half_pos = fixMul(pos, scale);   /* 25.125 — needs fixMul! */
```

The trap: `pos * scale` (using the C `*` operator) treats both as
plain `s16`s and gives you `(pos × scale) >> 0` — an enormous wrong
number. **Always use `fixMul`** when both operands are `fixed`. The
function does a 32-bit intermediate multiply and re-shifts back to
8.8 format.

For division: `fixDiv(a, b)` produces `a / b` as fixed-point.
**Returns 0 on divide-by-zero** — silent failure mode, but predictable.
Validate divisors before passing them.

## Trigonometry (sin/cos lookup tables)

The SDK uses **8-bit angles**: 0–255 maps linearly to 0°–360°. So
`64` = 90°, `128` = 180°, `192` = 270°. Wrapping is automatic — adding
1 to angle 255 wraps to 0 in `u8`.

```c
u8 angle = 0;

while (1) {
    fixed dx = fixCos(angle);              /* x component, 8.8 fixed */
    fixed dy = fixSin(angle);              /* y component */

    fixed speed = FIX(2);
    fixed move_x = fixMul(speed, dx);      /* 2 cos(angle), 8.8 */
    fixed move_y = fixMul(speed, dy);

    player_x = player_x + move_x;
    player_y = player_y + move_y;

    angle++;                               /* rotate 1.4° per frame */
    WaitForVBlank();
}
```

Both `fixSin` and `fixCos` return values in `[-256, +256]` (i.e.,
`-1.0` to `+1.0` in 8.8 fixed). The result of `fixMul(speed, fixSin(a))`
is a velocity vector component in 8.8 fixed.

The LUTs are 256-entry, 2 bytes per entry, 512 bytes total. They live
in ROM in `lib/source/math.asm`. Lookups are O(1) — ~20 cycles per
call.

## Hardware multiplier and divider

The SNES CPU has a hardware 8-bit unsigned multiplier (`WRMPYA` /
`WRMPYB` at `$4202`/`$4203`, result in `RDMPYL`/`RDMPYH`) and a 16-bit
unsigned divider (`WRDIVL`/`WRDIVH`/`WRDIVB` at `$4204`–`$4206`, result
in `RDDIVL`/`RDDIVH` for quotient and `RDMPYL`/`RDMPYH` for remainder).

The lib's `mul16` / `div16` / `mod16` use these:

```c
u16 a = 100, b = 50;
u16 product   = mul16(a, b);    /* 5000 — uses $4202/$4203 */
u16 quotient  = div16(a, b);    /* 2    — uses $4204-$4206 */
u16 remainder = mod16(a, b);    /* 0    — same divide, reads RDMPY */
```

Use these when:

- You need genuine 16 × 16 → 32-bit intermediate (handled internally).
- The compiler's `*` / `/` operator may produce slow code or call a
  runtime helper.

The lib functions wait the required cycle delay (8 cycles for multiply,
16 for divide) before reading the result. Hand-rolled access to the
hardware registers must respect those delays — read too early and you
get garbage.

## Utilities

```c
fixed v = FIX(-3) + 64;          /* -2.75 */
fixed positive = fixAbs(v);      /* 2.75 */

fixed clamped = fixClamp(v, FIX(0), FIX(10));   /* 0.0 — clamped to min */

fixed lerped = fixLerp(FIX(0), FIX(100), 128);  /* 50.0 — t=128/256=0.5 */
```

`fixLerp(a, b, t)` interpolates between `a` and `b` with `t` from `0`
(returns `a`) to `256` (returns `b`). Linear; the canonical way to
animate values smoothly between two endpoints.

## Worked patterns

### Smooth sprite movement

```c
fixed pos_x = FIX(64);
fixed pos_y = FIX(112);
fixed vel_x = FIX(1) / 2;       /* 0.5 px/frame */
fixed vel_y = FIX(1) / 4;       /* 0.25 px/frame */

while (1) {
    pos_x = pos_x + vel_x;
    pos_y = pos_y + vel_y;

    /* Bounce off screen edges */
    if (UNFIX(pos_x) < 0 || UNFIX(pos_x) > 240) vel_x = -vel_x;
    if (UNFIX(pos_y) < 0 || UNFIX(pos_y) > 208) vel_y = -vel_y;

    /* Render: convert fixed to integer screen coords */
    oamSetXYFast(0, UNFIX(pos_x), UNFIX(pos_y));
    WaitForVBlank();
}
```

The fixed-point representation gives sub-pixel precision so the
sprite advances 0.25 px/frame *visibly* — every fourth frame the
integer coordinate increments by one. With pure-integer velocity this
would round to 0 per frame, and the sprite would never move.

### Rotation around a point

```c
u8 angle = 0;
s16 cx = 128, cy = 112;            /* rotation centre on screen */
s16 radius = 32;

while (1) {
    fixed dx = fixMul(FIX(radius), fixCos(angle));   /* x offset */
    fixed dy = fixMul(FIX(radius), fixSin(angle));

    s16 sprite_x = cx + UNFIX(dx);
    s16 sprite_y = cy + UNFIX(dy);

    oamSetXYFast(0, sprite_x, sprite_y);
    angle += 2;                                       /* ~2.8° per frame */
    WaitForVBlank();
}
```

This pattern (radius × cos / radius × sin) is used everywhere a
sprite needs to orbit, swing, or rotate around a fixed point. Mode 7
matrix construction is conceptually the same operation, just emitted
to the M7A–M7D registers instead of OAM coordinates.

### Smooth interpolation

```c
/* Camera follows player, but lags by 25 % each frame for smoothness */
fixed camera_x = FIX(0);
while (1) {
    fixed target = pos_x;     /* where the camera wants to be */
    camera_x = fixLerp(camera_x, target, 64);   /* t=64/256=0.25 */

    bgSetScrollX(0, UNFIX(camera_x));
    WaitForVBlank();
}
```

The lerp toward the target with `t = 0.25` produces a critically-damped
smooth follow — every frame the camera covers a quarter of the
remaining distance. Common pattern for "the camera glides instead of
snapping".

### HDMA gradient table generation

The HDMA gradient example uses fixed-point under the hood: a
brightness ramp from level 15 at the top to level 0 at the bottom of
the screen, computed as a fixed-point linear interpolation over 224
scanlines. The lib's `hdmaBrightnessGradient(channel, top, bottom)`
helper does this internally.

For custom HDMA tables, the same pattern applies: compute fixed-point
fractional values per scanline, `UNFIX` to integer when writing the
table byte.

## Gotchas

### 🟢 `int` and `long` sizes are correct on this target (since chantier A1)

`sizeof(int) == 2`, `sizeof(long) == 4`. Bare `int` is the native 16-bit
word; `long` is 32 bits. C convention says `int` is the natural word size
of the machine, and on the w65816 that's now true.

For **portability** across compilers (cross-platform tooling, code shared
with PC-side helpers), keep using `s16`, `u16`, `s32`, `u32`, `fixed` from
`lib/include/snes/types.h` — they make intent explicit and work
identically everywhere. Bare `int speed = 5;` no longer doubles your
cycle cost on cc65816, but a build of the same code with a different
compiler may behave differently.

The `fixed` type is still `s16`. No change for fixed-point users.

### 🔴 `FIX(x)` overflows for `|x| ≥ 128`

`FIX(x)` is `(x << 8)`. For `x = 128`, the result is `0x8000` =
`-32768` interpreted as `s16` = `-128.0` in fixed-point — wrap
around. There's no compile-time error and no runtime warning.

If you need values larger than `±127`, either:

- Switch to **integer pixel coordinates** for position and reserve
  fixed-point for *velocity* / *acceleration* (which stay small).
- Use a custom **16.16 format** in `s32` for higher range. The lib
  doesn't ship 16.16 helpers — write them per-game.

### 🔴 `fixMul` truncates fractional precision

`fixMul(0x0080, 0x0080)` (0.5 × 0.5) returns `0x0040` (0.25). Correct.
But `fixMul(0x0001, 0x0001)` (~0.004 × ~0.004) returns `0x0000` —
the result underflows to zero. For very small fixed values, the
shift back to 8.8 throws away precision.

This is intrinsic to 8.8 fixed-point: ~0.0039 is the minimum
representable non-zero. Multiplying two such values gives ~1.5e-5,
unrepresentable. If you need tiny scale factors (e.g., very slow
acceleration), use a wider fixed-point format or accumulate without
multiplying.

### 🟠 `fixDiv` returns 0 on divide-by-zero

Silent failure: `fixDiv(FIX(10), FIX(0))` returns `0`, not a sentinel,
not a trap. Validate divisors before calling — especially for
user-derived inputs (e.g., dividing by a velocity that could be zero
when the player isn't moving).

### 🟠 Compiler `*` vs `mul16`

The C `*` operator works correctly on `s16`/`u16` operands; the
"runtime can have bugs" caveat in `mul16`'s docstring is historical
(early QBE backend bugs that have shipped fixes per `compiler/PINS.md`).
Today, `a * b` where both are `u16` is generally safe and faster than
`mul16(a, b)` (the compiler inlines the hardware multiplier write).

Use `mul16` when:

- You need the explicit "I am invoking the hardware multiplier" path.
- You're paranoid about a specific corner case (the 32-bit intermediate
  is guaranteed).
- You're matching a PVSnesLib port byte-for-byte.

### 🟡 Hardware multiplier read timing

The hardware multiplier needs **8 CPU cycles** between writing
`WRMPYB` and reading `RDMPYL`/`RDMPYH`. The hardware divider needs
**16 cycles** between `WRDIVB` and `RDDIVL`/`RDDIVH`/`RDMPYL`. The lib
inserts the right delays in `mul16`/`div16`/`mod16`. Custom assembly
that touches the multiplier registers must respect this — read too
early and you get partial results.

### 🟡 Sin/cos LUTs are unsigned-angle, signed-result

`fixSin(angle)` and `fixCos(angle)` take `u8` (no sign — the
8-bit angle wraps naturally). They return `fixed` (signed `s16`).
This is the right shape for game code, but if you're translating
from a different engine, watch the signedness boundary.

### `atan2` and `sqrt` ship in the lib (chantier B6, 2026-05-09)

The full inverse-trig and square-root surface arrived together so
the canonical "where is the target relative to me, and how far?"
question is one call away. Three entry points cover almost every
game-side use:

| Function | Returns | Cost |
|---|---|---|
| `u16 sqrt16(u16 n)` | floor of `sqrt(n)` (always 0–255) | ~80 cycles, no LUT |
| `fixed fixSqrt(fixed x)` | `sqrt(x)` in 8.8 fixed | ~80 cycles, 4 fractional bits |
| `u8 atan2_8(s16 dy, s16 dx)` | 8-bit angle (0–255 = 0°–360°) | ~120 cycles, 65-byte LUT |

`atan2_8` uses the same 8-bit angle convention as `fixSin`/`fixCos`,
so the natural pattern works:

```c
s16 dy = enemy_y - player_y;
s16 dx = enemy_x - player_x;
u8 aim = atan2_8(dy, dx);

/* Now use aim with the trig LUTs to project a velocity vector */
fixed vx = fixMul(speed, fixCos(aim));
fixed vy = fixMul(speed, fixSin(aim));
```

`atan2_8` is scale-invariant — pass any 16-bit signed dy/dx and
the function reduces magnitudes by power-of-two right shifts
internally to keep its single 16-bit divide overflow-free.
Precision degrades by ≤ 1 angle unit (≈ 1.4°) when reduction
fires, which is below the perceptual floor for projectile aiming
or sprite rotation in any 256-pixel-wide playfield.

`sqrt16` is the canonical "how far is that thing" helper. The
result is bounded to 255 (since `sqrt(65535) ≈ 255.99`), so it
fits in `u8` for tile-grid distances. For the 8.8 fractional
variant use `fixSqrt`. Note that `fixSqrt`'s precision is
intentionally capped at 4 fractional bits — the 32-bit shift
needed for full 8 bits of fraction would currently truncate
under the QBE 32-bit codegen gap (catalogue chantier A7); we
chose deterministic 4-bit precision over deceptive 8-bit
output that's only correct for small inputs.

### What's still missing: `pow`, `exp`, `log`

These remain off the SDK's beaten path because no shipped example
needs them. Integer exponentiation for small exponents is two
lines of repeated `fixMul`; everything else (logarithm, general
power) is per-game territory — bake a LUT for the specific
range your game cares about.

The PHILOSOPHY non-goal "no `printf` in core lib" extends here in
spirit: the lib provides what most games need, and skips the heavy
weight that most games don't.

## Cycle cost summary

| Operation | Cycles (approx) |
|---|--:|
| `FIX(x)` / `UNFIX(x)` (macros) | 0–2 (compile-time or shift) |
| `fixed + fixed` (addition) | 4–8 |
| `fixMul(a, b)` | ~30 |
| `fixDiv(a, b)` | ~80 |
| `fixSin(angle)` / `fixCos(angle)` | ~20 (LUT lookup) |
| `fixLerp(a, b, t)` | ~40 |
| `mul16(a, b)` (hardware multiply) | ~10 |
| `div16(a, b)` (hardware divide) | ~20 |
| `sqrt16(n)` | ~80 (bit-by-bit) |
| `fixSqrt(x)` | ~80 (delegates to sqrt16) |
| `atan2_8(dy, dx)` | ~120 (LUT + one 16-bit divide) |

A typical sprite-physics frame (position update + sin/cos rotation +
two `fixMul` calls per sprite × 64 sprites) is ~6 K cycles, well
under 1 % of the frame. Fixed-point is *cheap*; the budget is rarely
binding.

## See also

- `lib/include/snes/math.h` — full API reference.
- `lib/source/math.asm` — implementation (273 LOC; the sin/cos LUTs
  live here).
- [Mode 7 tutorial](mode7.md) — the natural application: matrix
  building via `fixSin`/`fixCos`/`fixMul`.
- [HDMA tutorial](hdma.md) — for runtime HDMA table generation that
  uses fixed-point under the hood.
- [Animation tutorial](animation.md) — uses lerp and angle-based
  patterns for sprite animation curves.
- [Collision tutorial](collision.md) — the bounding-box collision
  module uses integer math, but advanced collision (circle, swept,
  ray-cast) goes through fixed-point.
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) — covers the
  historical `int = 32 bits` trap (closed by chantier A1 on 2026-05-08).
