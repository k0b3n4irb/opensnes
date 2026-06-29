# fix32_orbit

![Screenshot](screenshot.png)

A single 8×8 sprite orbits the screen centre, driven entirely by the lib's
**16.16 fixed-point** API (`<snes/fixed32.h>`). It's the simplest possible
`fixed32` fixture: one trig-driven motion equation, no input, no game logic —
just the `radius × cos / radius × sin` pattern that every 2D game uses for
orbits, aiming, and parallax-around-a-point.

Each frame the example advances a 16.16 angle accumulator, reads `fix32Cos` /
`fix32Sin` for that angle, scales them by the radius with `fix32Mul`, and
projects the result to integer screen coordinates centred at (128, 96).

## SNES Concepts

- **`fixed32` (s32 16.16)** — a 16.16 accumulator keeps the angle smooth across
  hundreds of frames without the rounding drift an 8.8 type would accumulate.
- **`fix32Sin` / `fix32Cos`** — the trig LUTs lifted to 16.16 (≈25 cycles each),
  enough fractional precision for sub-pixel animation.
- **`fix32Mul(radius, fix32Cos(angle))`** — the canonical trig-driven motion
  term, reused in `aim_target`, `likemario`, and `breakout`.
- **Minimal sprite setup** — one 4bpp tile DMAed to VRAM `$4000`, a two-colour
  palette at the sprite CGRAM base, `OBJSEL` pointed at the tile base, then a
  single `oamSet` per frame.

## What to Observe

The white dot traces a smooth circle around the screen centre — about one full
revolution every ~3.9 s at 60 Hz (`g_omega = FIX32(1)`, one LUT step per frame).
The motion is continuous and sub-pixel-smooth because the angle lives in 16.16,
not in screen pixels. The orbit state is exposed as globals (`g_angle`,
`g_radius`, `g_sx`, `g_sy`) for easy inspection while debugging.

## How to Build

```sh
make -C examples/basics/fix32_orbit
```

The ROM lands at `examples/basics/fix32_orbit/fix32_orbit.sfc`.

## Modules Used

`console`, `sprite`, `dma`, `background`, `fixed32`, `math`.

## See also

- [`docs/tutorials/math.md`](../../../docs/tutorials/math.md) — the fixed-point
  math tutorial.
- [`lib/include/snes/fixed32.h`](../../../lib/include/snes/fixed32.h) — Doxygen
  reference for the 16.16 API.
- [`examples/basics/aim_target`](../aim_target/) — the same trig primitives
  composed into live distance/angle/aiming readouts.
