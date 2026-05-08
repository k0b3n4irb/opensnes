# Colour Math Tutorial {#tutorial_colormath}

This tutorial covers the SNES colour-math unit: the per-pixel hardware
that *adds* or *subtracts* the colour of one layer onto another, with an
optional half-divide step. It's how every SNES game does
*transparency*, *shadows*, *fades to black/white*, *underwater tints*,
*lighting effects*, and *additive overlays* (think clouds, mist,
explosions, magic spells).

It assumes you have read the [Graphics](graphics.md) and
[Window](window.md) tutorials. The window section is the natural pair —
windows define **where** colour math applies, colour math defines
**what** to blend.

## What colour math actually is

Every pixel that the SNES PPU draws goes through a per-pixel pipeline
that, late in rendering, can add or subtract a *second* colour onto
the *first* colour. The two colours come from:

- **Main screen** — the primary image you see, composed from the BG /
  OBJ layers you enabled via `setMainScreen` / `REG_TM`.
- Either:
  - the **sub screen** — a parallel render target you enable via
    `setSubScreen` / `REG_TS`, OR
  - a **fixed colour** stored in `COLDATA` (`$2132`) — a single 5-bit
    R/G/B value applied uniformly.

The math operation:

| Op | Formula | Use |
|---|---|---|
| `COLORMATH_ADD` | `out = main + source` | Brighten, additive overlay (clouds, fire, sparkles) |
| `COLORMATH_SUB` | `out = main - source` | Darken, fade-to-black, underwater |
| `+ half` | `out = (main ± source) / 2` | True 50 % blend (transparency) |

A "half" flag (`colorMathSetHalf(1)`) divides the result by 2 — the
canonical 50 % transparency effect. Without half, addition saturates
(values clip at 31 per channel) and subtraction can clip to zero.

## What "main" and "sub" mean here

A common stumbling block: the same layer can be on **both** main and
sub screen. They aren't physical layers; they're *gates*.

- `setMainScreen(LAYER_BG1)` says "BG1 contributes to main screen".
- `setSubScreen(LAYER_BG2)` says "BG2 contributes to sub screen".
- Setting both `LAYER_BG1` flags on main *and* sub puts BG1 in both
  composites — colour math sees BG1's pixel on both sides.

For a simple "BG2 is 50 % transparent over BG1" effect:

1. Put BG1 on **main only** — the opaque base.
2. Put BG2 on **both main and sub** — main draws BG2, and the colour
   math reads BG2 from sub to blend it with BG1 underneath.

This is the canonical pattern. For more advanced layouts (BG2 on sub
only, BG1 on main, math gate on a window region), the gates flex
independently.

## The minimum viable colour math

Cloud layer (BG2) blended additively over a landscape (BG1):

```c
#include <snes.h>

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);
    /* … load BG1 (landscape) and BG2 (clouds) tilemaps + tiles … */

    /* Both layers on main, BG2 also on sub */
    setMainScreen(LAYER_BG1 | LAYER_BG2);
    setSubScreen(LAYER_BG2);

    /* Colour math: BG2's pixels get added (with half) to whatever is on
     * the main screen below them. The result is a 50% blend. */
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetHalf(1);
    colorMathEnable(COLORMATH_BG1);   /* math applies on BG1 pixels */

    setScreenOn();
    while (1) WaitForVBlank();
}
```

The non-obvious bit: **`colorMathEnable(layer)` selects which
*destination* layer participates in the math, not which source.** The
source is always determined by `colorMathSetSource` (sub screen or
fixed colour); the layer mask says "for pixels on BG1 (or BG2, or OBJ),
allow this pixel to be modified by math".

A typical "let everything blend" setup would be
`colorMathEnable(COLORMATH_ALL)` — but more often you restrict it to a
specific layer to control which surfaces look transparent.

## Fade-to-black / fade-to-white

Fixed-colour mode subtracts (or adds) a uniform RGB value across the
whole frame, ignoring the sub screen. Animate the fixed colour over
several frames to fade in or out.

```c
/* Fade out to black over 16 frames */
for (u8 step = 0; step <= 15; step++) {
    u8 c = step;   /* 0 = no fade, 15 = full black */
    colorMathSetFixedColor(c, c, c);
    colorMathSetSource(COLORMATH_SRC_FIXED);
    colorMathSetOp(COLORMATH_SUB);
    colorMathEnable(COLORMATH_ALL);
    WaitForVBlank();
}
```

The `lib/include/snes/video.h` `setBrightness()` is a *different*
mechanism — it scales every channel uniformly by a 4-bit factor via
`INIDISP`, and is the right tool for "global brightness". Colour math
gives finer control: per-layer fades, asymmetric R/G/B fades for
underwater tints, etc.

## The window gate

Colour math can be restricted to a *window region* — applying only
inside or only outside one of the windows. This is the bridge between
the [Window tutorial](window.md) and this one:

```c
/* Spotlight: clouds blend only inside a window */
windowInit();
windowSetPos(WINDOW_1, 64, 192);
windowEnable(WINDOW_1, WINDOW_MATH);          /* gate on the math area */
windowSetMainMask(WINDOW_BG1 | WINDOW_BG2);   /* layers windowed too */

colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
colorMathSetOp(COLORMATH_ADD);
colorMathSetHalf(1);
colorMathSetMaskMain(COLORMATH_INSIDE);       /* math inside window only */
colorMathEnable(COLORMATH_BG1);
```

The four constants for `colorMathSetMaskMain(condition)`:

| `COLORMATH_*` | Math fires when… |
|---|---|
| `_ALWAYS` | always (default — full screen) |
| `_INSIDE` | pixel is inside the window-gated math area |
| `_OUTSIDE` | pixel is outside it |
| `_NEVER` | never (use to disable math without unwinding all enables) |

The pair `colorMathSetMaskSub(condition)` does the same for the sub
screen — usually less interesting, but `_INSIDE` on sub is the right
gate for "blend with sub-screen colour only inside the window".

## The lib API

| Function | Purpose |
|---|---|
| `colorMathInit()` | Reset all colour-math registers to "off". Call before configuring a new scene. |
| `colorMathEnable(layers)` | Bitmask of which layers' pixels can be modified by math (`COLORMATH_BG1`, …, `COLORMATH_OBJ`, `COLORMATH_BACKDROP`, `COLORMATH_ALL`). |
| `colorMathDisable(layers)` | Take layers out of the per-pixel math gate. |
| `colorMathSetOp(op)` | `COLORMATH_ADD` or `COLORMATH_SUB`. |
| `colorMathSetHalf(half)` | 0 = don't divide result, 1 = divide by 2 (true 50 % blend). |
| `colorMathSetSource(src)` | `COLORMATH_SRC_SUBSCREEN` or `COLORMATH_SRC_FIXED`. |
| `colorMathSetFixedColor(r, g, b)` | Set the fixed-colour value (5-bit per channel). Used when source is `_FIXED`. |
| `colorMathSetMaskMain(condition)` | Window gate for the main-screen path: `_ALWAYS` / `_INSIDE` / `_OUTSIDE` / `_NEVER`. |
| `colorMathSetMaskSub(condition)` | Same for the sub screen. |

## Worked patterns (the two shipped examples)

### Additive cloud overlay — `examples/graphics/effects/transparency`

The simplest blend pattern. BG1 (main) shows a 4 bpp landscape; BG3
(main + sub) shows 2 bpp scrolling clouds. Colour math is set to
**add** with half disabled (saturating add), applied to BG1 and the
backdrop. The clouds *brighten* what's underneath without dimming
themselves — characteristic of additive blending.

The example uses Mode 1 with BG3 priority high (so the 2 bpp cloud
layer renders above BG1). It uses bare-metal `REG_CGWSEL` /
`REG_CGADSUB` writes for byte-perfect parity with the PVSnesLib
original, but the lib's helpers cover the same configuration.

### Window-gated spotlight blend — `examples/graphics/effects/transparent_window`

Combines colour math with a *window-gated* math area. Inside the
window, BG1 is half-blended with BG2 (true transparency); outside, BG1
is fully opaque. The example also uses HDMA to animate the window
shape — see the [Window](window.md) and [HDMA](hdma.md) tutorials for
those sides.

This is the canonical demonstration of how three SNES PPU subsystems
(window, colour math, HDMA) compose: window says where, colour math
says what to blend, HDMA animates the shape per scanline.

## Gotchas

### 🔴 The "BG2 is sub-screen-only" trap

Putting a layer **only** on the sub screen and not on the main screen
does *not* show that layer through the colour math. The main screen
must contain *something* for the math to modify; the sub screen pixel
contributes only as the *source* of the math operation, not as a
visible layer.

For "BG2 visible only inside a window", put BG2 on main + sub, set
window mask to BG2, and gate colour math accordingly. The "sub only"
configuration is for cases where BG2's pixel shouldn't render at all
unless math reads it.

### 🟠 `colorMathSetSource` polarity is opposite of intuitive

The lib's `colorMathSetSource` sets the source register `CGWSEL` bit 1.
At the bare-metal level: bit 1 = 0 → fixed colour, bit 1 = 1 → sub
screen. The lib was historically inverted; commit history (per
`KNOWN_LIMITATIONS.md` and `lib/source/colormath.c`) records the fix.
**Use the lib helper, not bare-metal writes.** If you do go bare metal,
verify polarity against the SNES PPU register reference, not against
intuition.

### 🟠 Half-mode and saturation interact

- **Without half**, `ADD` saturates at 31 per channel (white) and `SUB`
  saturates at 0 (black). Two bright sources → white. Useful for fire,
  sparkles, magic flares.
- **With half**, `ADD` produces a true 50 % blend (no saturation
  possible) and `SUB` produces a 50 % darkening (also clamped, but at
  half the input). Useful for transparency, shadow, water.

If your transparency looks "too bright" or "too washed-out", you
probably forgot half. If your additive glow looks "too washed out and
hits white quickly", you probably *enabled* half.

### 🟡 Colour-math on the backdrop

`COLORMATH_BACKDROP` lets you blend onto the backdrop colour (CGRAM[0])
in regions where no BG/OBJ layer covers a pixel. This is useful for
tinting the gameplay area (the "underwater" scene) where some pixels
are backdrop-coloured.

If you use it: remember the backdrop is a single colour, so the math
applies uniformly across all backdrop pixels. To tint only *some*
backdrop region, use a window gate.

### 🟡 Colour math runs after windowing

The pipeline order matters: per-pixel windowing decides whether the
layer renders, *then* colour math runs on the surviving pixels. A
windowed-out pixel doesn't reach the math stage. This is usually what
you want — but it means you cannot use colour math to *recover* a
pixel that windowing already masked.

To fade out a region while keeping the layer visible underneath (for
example, a vignetting effect): use the window-gated math area
(`COLORMATH_INSIDE` / `COLORMATH_OUTSIDE`), not the layer window mask.

### 🟡 OBJ math depends on palette

When `COLORMATH_OBJ` is enabled, colour math applies *only* to sprite
pixels that use OBJ palettes 4–7 (the upper half). Sprites in OBJ
palettes 0–3 always render opaque, regardless of the math
configuration. A common "why doesn't my sprite blend?" cause.

This is a SNES PPU rule, not an OpenSNES choice — the sprite needs to
be in a "math-eligible" palette to participate.

## Cycle cost

Colour math runs in PPU hardware, not on the CPU, so the per-pixel
blend is free. The cost lives in the configuration writes:

- Initial `colorMathInit` + a typical setup (op + half + source +
  enable + fixed colour) ≈ 30–50 cycles per scene change.
- Animating the fixed colour each frame for a fade ≈ 10 cycles per
  frame. Negligible.
- Animating the math area via HDMA on `CGWSEL`/`CGADSUB`: see the
  [HDMA tutorial](hdma.md) — typical 1-channel HDMA on these registers
  costs ~3–5 K cycles per frame.

Colour math is one of the cheapest large-visible-impact effects on the
SNES. The discipline is configuring it correctly, not avoiding it.

## See also

- `lib/include/snes/colormath.h` — full API reference, polarity-fixed
  helper definitions.
- `lib/source/colormath.c` — implementation; commit history records
  the `CGWSEL` bit-1 polarity fix.
- [`examples/graphics/effects/transparency`](../../examples/graphics/effects/transparency/README.md) — additive cloud overlay.
- [`examples/graphics/effects/transparent_window`](../../examples/graphics/effects/transparent_window/README.md) — window-gated spotlight blend.
- [Window tutorial](window.md) — the pair tutorial; covers
  `WINDOW_MATH` and the window-area gate that the math source mask
  references.
- [HDMA tutorial](hdma.md) — for animating colour-math configuration
  per scanline (gradients on `COLDATA`, etc.).
- [Graphics tutorial](graphics.md) — companion read for the BG-layer
  fundamentals colour math operates on.
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) —
  documents the historical `CGWSEL` polarity inversion and other
  PPU-side traps.
