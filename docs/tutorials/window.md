# Window Masking Tutorial {#tutorial_window}

![window example](window.png)

*`examples/windows/window`: two windows clipping the backgrounds, their edges driven by HDMA tables.*

This tutorial covers the SNES window-masking hardware: two configurable
rectangles that clip individual BG layers, sprites, and the colour-math
area on a per-pixel basis. The reach of this feature is wider than its
name suggests — windows are how you do spotlights, scene transitions
(an iris fade), split-screen status bars, animated triangle / diamond /
circle reveals, and the area gate for transparency / blending effects.

It assumes you have read the [Graphics](graphics.md) tutorial. The
animated-shape patterns also depend on the [HDMA tutorial](hdma.md).
The pair tutorial is [Colormath](colormath.md), which uses the window
to gate which screen region gets blended.

## What windows are

The SNES PPU keeps two independent windows: `Window 1` and `Window 2`.
Each window is defined by a left and right pixel position (registers
`WH0`/`WH1` for Window 1, `WH2`/`WH3` for Window 2 — `0`–`255`
inclusive). Per scanline, every screen pixel is either *inside* or
*outside* each window; that boolean is then fed to a per-layer mask
that decides whether to draw the pixel.

Three orthogonal axes:

1. **Window position** — `windowSetPos(window, left, right)` writes
   the boundaries.
2. **Per-layer enable** — `windowEnable(window, layers)` decides
   *which* layers consult this window. Layers can be any subset of
   BG1, BG2, BG3, BG4, OBJ (sprites), and the colour-math area.
3. **Inside vs outside** — `windowSetInvert(window, layers, invert)`
   flips whether pixels *inside* the window are masked or pixels
   *outside* are masked. The default ("inside is masked") shows the
   layer everywhere except in the window — used to *cut out* a region.
   Inverted ("outside is masked") shows the layer only in the window —
   used to *spotlight* a region.

When **both** windows are enabled for a single layer, they combine via
a logic operation (set with `windowSetLogic(layer, op)`):

| Op | `WINDOW_LOGIC_*` | Mask if pixel is… |
|---|---|---|
| OR | `_OR` | inside W1 **or** W2 |
| AND | `_AND` | inside W1 **and** W2 |
| XOR | `_XOR` | inside exactly one of W1, W2 |
| XNOR | `_XNOR` | inside both **or** neither |

The default is `OR`. Use `AND` for "intersect the two windows" (a
smaller, central area). Use `XOR` for "show one or the other but not
the overlap" — useful for split-screen reveals.

Finally, the windowing system has separate gates for the **main
screen** (what's actually displayed) and the **sub screen** (used by
colour math for blending source). `windowSetMainMask(layers)` selects
which layers on the main screen consult their window setup;
`windowSetSubMask(layers)` does the same for the sub screen. Most
common use is `windowSetMainMask(WINDOW_ALL)` to apply windowing on
the visible image.

## The minimum viable window

The smallest useful pattern: hide a strip of BG1 in the middle of the
screen.

```c
#include <snes.h>

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);
    /* … load tiles, palettes, tilemap … */
    setMainScreen(LAYER_BG1);

    windowInit();                                       /* default state */
    windowSetPos(WINDOW_1, 80, 176);                    /* 96-pixel slab in the middle */
    windowEnable(WINDOW_1, LAYER_BG1);                 /* … applied to BG1 */
    windowSetMainMask(LAYER_BG1);                      /* … on the main screen */

    setScreenOn();
    while (1) WaitForVBlank();
}
```

`windowInit()` resets every window-related PPU register to "no
windowing" — every layer renders normally. Always call it before
configuring (or you may inherit stale state from a previous scene).

The default invert state is "show inside, hide outside" — meaning the
slab from x=80 to x=176 *covers* BG1, and pixels outside are kept.
That's usually what you want for "spotlight" effects. To flip:

```c
windowSetInvert(WINDOW_1, LAYER_BG1, 1);   /* show outside, hide inside */
```

## Static shapes vs animated shapes

The window's left/right boundaries are *single registers per window*.
If you write them once, the boundaries stay the same on every scanline
of the next frame — you get a vertical strip the full height of the
screen.

To get any other shape, you write the boundary registers **per
scanline via HDMA**. The shape is encoded as a table of "for these N
scanlines, the window should be from `left` to `right`":

| Shape | Pattern of left/right per scanline |
|---|---|
| Static rectangle | constant `left`, constant `right` (no HDMA needed) |
| Vertical strip changing over time | constant table, regenerated each frame |
| Diamond / triangle | linear ramp of `left` and `right` toward the middle, then back out |
| Circle | sin-table or pre-computed (top-half symmetric to bottom-half) |
| Letterbox / iris fade | `left = 0, right = 255` outside the band, `left > right` (empty window) inside |

The shipped `examples/windows/window` uses HDMA channels 4
and 5 in repeat mode to write `WH0` (`$2126`) and `WH1` (`$2127`) per
scanline, producing an animated triangle/diamond. Read the example for
the table-build pattern; the principle: compute the `left` and `right` window edge for each scanline `s`, lay them out as an HDMA table, then trigger.

## When the window register acts strange

Two non-obvious behaviours:

### `left > right` is an "empty" window

If you set `left = 200, right = 100`, the window is *empty* — no pixel
is inside it. Useful for "turn off the window mid-frame" via HDMA: a
table line that sets `left = 1, right = 0` removes the window for that
scanline group without disabling the whole layer.

### Boundaries are inclusive on both ends

Pixel at `x = left` is inside; pixel at `x = right` is also inside.
A "1-pixel wide" window has `left == right`. A window of width 256
spans the whole screen with `left = 0, right = 255`.

## The window logic op trap

When you call `windowEnable(WINDOW_1, LAYER_BG1)` and
`windowEnable(WINDOW_2, LAYER_BG1)`, both windows now affect BG1 —
and the PPU consults the **logic op** for BG1 to decide. The default
logic is `OR`.

If you only ever use one window per layer, the logic op is irrelevant.
If you use two windows on the same layer, set the op explicitly with
`windowSetLogic(layer, WINDOW_LOGIC_*)` — relying on the default `OR`
is fine but spelled-out logic is easier for the next reader.

## The lib API

| Function | Purpose |
|---|---|
| `windowInit()` | Reset all window registers to "no windowing". Call before configuring a new scene. |
| `windowSetPos(window, left, right)` | Set the boundaries for `WINDOW_1` or `WINDOW_2` (0–255 inclusive). |
| `windowEnable(window, layers)` | Make the given layers (`LAYER_BG1`, `LAYER_BG2`, …, `LAYER_OBJ`, `WINDOW_MATH`) consult this window. |
| `windowDisable(window, layers)` | Stop those layers from consulting this window. |
| `windowDisableAll()` | Cleanly tear down: all layers ignore both windows. |
| `windowSetInvert(window, layers, invert)` | Flip "inside is masked" → "outside is masked" per layer. |
| `windowSetLogic(layer, logicOp)` | Set the W1/W2 combine op for one layer when both windows are enabled. |
| `windowSetMainMask(layers)` | Apply windowing on the main screen for these layers. |
| `windowSetSubMask(layers)` | Apply windowing on the sub screen — relevant for colour-math source gating. |
| `windowCentered(window, width)` | Helper: position window symmetrically around screen centre with given width. |
| `windowSplit(splitX)` | Helper: configure W1 = left half (`0..splitX`), W2 = right half (`splitX..255`) for split-screen. |

## Worked patterns (the shipped examples)

### Animated triangle reveal — `examples/windows/window`

A diamond/triangle shape that reveals BG1 only inside the shape, with
runtime layer selection:

- Press **A**: apply window to BG1 only.
- Press **X**: apply window to BG2 only.
- Press **B**: apply window to both BG1 and BG2.
- Press **Y**: shade the triangle instead (below).

Two HDMA channels (4 and 5) write `WH0`/`WH1` per scanline. The example
configures everything through the lib helpers: `windowDisableAll()`,
`windowEnable()`, `windowSetInvert()` and `windowSetMainMask()` for the
window, `hdmaSetup()` and `hdmaEnableMask()` for the two channels.

### Window + colour math — `examples/windows/window` (press Y)

The same window can mark **where colour math happens** instead of what
is hidden. With Y, no layer is clipped (`windowSetMainMask(0)`); the
colour window (`WINDOW_MATH`, a mask bit that targets the colour-math
area rather than a BG/OBJ) is the triangle, and colour math subtracts a
fixed grey from BG1 and BG2 inside it only (`COLORMATH_INSIDE`). The
triangle is darkened — the way a game draws a shadow or a dialogue box
over the scene. See the [Colormath tutorial](colormath.md) for the
colour-math side.

## Gotchas

### 🟡 Repeat vs non-repeat on `WH0`/`WH1` — pick by data shape, not by register

The window boundary registers latch their value like every other PPU
register: a value written once holds until the next write (that is why
the static-rectangle row in the shapes table needs no HDMA at all).
What the line-count high bit selects is the **table layout**:

```
.db 32, 80, 176     ; non-repeat: ONE pair, written once, held 32 lines
.db $83, 80,176, 78,178, 76,180   ; repeat: 3 lines, one fresh pair PER line
```

- **Non-repeat** — one scanline worth of data per entry, held for the
  count. Right for any band where the edges are constant (rectangle,
  letterbox bar). If the shape doesn't reach the bottom of the frame,
  end with an empty-window entry (left > right) or the last band's
  latched values run to the bottom.
- **Repeat** — the count says how many scanlines worth of data follow,
  one fresh write per line. Needed only when the edges move every line
  (diamond, circle, wave).

Mixing the layouts is the classic silent failure: per-line data placed
under a non-repeat count makes the hardware hold the first pair and
misparse the following pairs as line counts — the shape appears to
collapse after its first row. (An earlier version of this page blamed
the registers for forgetting their value each scanline; the real
hardware latches, and the collapse comes from the table misparse. See
the table-format section of the [HDMA tutorial](hdma.md).)

### 🟠 `windowEnable(window, WINDOW_MATH)` is the gate to colour math

`WINDOW_MATH` is one of the layer-mask bits, but it doesn't refer to a
BG or OBJ — it refers to the colour-math *area mask*. Setting it tells
the PPU "this window restricts where colour math applies". It has no
visible effect on its own; you also need colour math configured (see
the [Colormath tutorial](colormath.md)) and the math area mask
written via the registers `$2130`–`$2131`. Easy to forget either side
of that handshake.

### 🟡 Always pair `windowEnable` with `windowSetMainMask`

`windowEnable` tells *which* windows affect *which* layers; it does
not by itself say "actually apply this window on the screen". The
final gate is `windowSetMainMask(layers)` (or
`windowSetSubMask(layers)`), which writes `TMW` (`$212E`) for the
main screen.

A scene that calls `windowEnable(WINDOW_1, LAYER_BG1)` but forgets
`windowSetMainMask(LAYER_BG1)` shows BG1 normally — the window is
configured but ignored. The `windowInit()` default leaves both masks
empty, so this is the common newcomer trap.

### 🟡 Window state survives between scenes

`windowInit()` is *not* called by `consoleInit()`. If you switch
scenes in your game, residual window state from the previous scene
can mask the new scene's layers. Call `windowInit()` (or
`windowDisableAll()` plus an explicit reconfiguration) at the start
of each scene's setup.

### 🟡 Boundary register sequencing during HDMA

HDMA writes `WH0`, then `WH1`. If you also poke these registers
manually mid-frame (without HDMA), keep both writes adjacent — a write
to `WH0` followed by an unrelated PPU register write before `WH1` can
land the second value in the wrong register. The lib helpers do this
correctly; bare-metal code needs to keep the pair tight.

## Cycle cost

The window logic itself is free — it's part of the PPU's per-pixel
shader. The cost of *animating* the window comes from HDMA writes, and
follows the [HDMA tutorial's cycle table](hdma.md):

- Static window (no HDMA): zero CPU cost per frame after the initial
  `windowSet*` register writes.
- Animated window with one HDMA channel: the per-scanline cost of a
  1-byte table in the HDMA tutorial, about 1–3 % of the bus time.
- Two-channel HDMA (W1 left/right): about double the above.

Window animation is one of the cheapest scanline-effect patterns
because the data per scanline is small (1–2 bytes per channel).

## See also

- `lib/include/snes/window.h` — full API reference.
- `lib/source/window.c` — implementation (mostly register
  bookkeeping for the four-axis state).
- [`examples/windows/window`](../../examples/windows/window/README.md) — the triangle window as a mask (A, X, B) and as the region of colour math (Y).
- [HDMA tutorial](hdma.md) — required reading for animated window
  shapes.
- [Colormath tutorial](colormath.md) — the pair tutorial; covers the
  colour-math side of the Y mode.
- [Graphics tutorial](graphics.md) — companion read for the BG-layer
  fundamentals the window operates on.

### Both windows per scanline — `examples/windows/window_multi_hdma`

One channel in `HDMA_MODE_4REG` streams WH0-WH3 — both windows' left AND
right edges — per band, cutting a porthole grid into BG1. The window
algebra: W1 and W2 both enabled on the layer, both INVERTED
(`windowSetInvert`), combined with `WINDOW_LOGIC_AND`, masked on the main
screen (`windowSetMainMask`) — the layer is hidden where (outside W1) AND
(outside W2), i.e. visible inside either window. A degenerate window
(left=1, right=0) is the standard "band fully masked" table value.
