# mode2 — offset-per-tile in Mode 2 and Mode 4: per-column scroll from BG3

**Backgrounds · the offset-per-tile modes (2/4/6).**

## Why it matters for your game

Modes 2, 4 and 6 have a trick no other mode offers: **offset-per-tile
(OPT)** lets every 8-pixel *column* of a layer scroll independently. That is
how you get a flag rippling in the wind, a heat-haze shimmer, water
distortion, or cheap per-column parallax — effects that would otherwise cost
a scanline of HDMA or a full software redraw. It is niche, but when you want
it, nothing else does it as cheaply.

![Screenshot](mode2.png)

## What you'll learn

The single idea: in Mode 2, **BG3 stops being a drawable layer and becomes
an offset table.** For each column of BG1/BG2, the PPU reads a scroll offset
straight out of BG3's tilemap:

- Per screen column *N ≥ 1* (column 0 can't be offset), the PPU reads BG3
  **tile-row 0** for the horizontal offset word and **tile-row 1** for the
  vertical offset word.
- **Offset word:** bit `0x2000` = apply to BG1 (`0x4000` = BG2); the V value
  is bits 0–9, the H value bits 3–9 (8-pixel-granular).

This example puts a stack of horizontal colour bands on BG1 and writes a sine
into BG3's V-offset row, so each column samples the bands at a different
height — the bands ripple. The phase advances each frame. **Modes 4 and 6
reuse this exact data path**, and press **B** for Mode 4.

### Mode 4: one row, one word per column (B)

![Mode 4](mode2_mode4.png)

Mode 4 gives you an 8bpp (256-colour) BG1, a 2bpp BG2, and — like Mode 2 —
BG3 as the offset table. The difference is the table: Mode 4 reads **one
row** of it, so each column gets one word, either vertical or horizontal:

- **The Mode 4 offset word:** bit 15 = 1 vertical, 0 horizontal; bit 13 =
  apply to BG1 (bit 14 = BG2); the value in bits 0–9, of which a horizontal
  offset ignores the low 3 (snesdev-wiki, *Offset-per-tile*).
- **One row:** BG3 row 0 holds the 32 words; word 0 applies to the second
  visible column — the leftmost column is never offset.
- **BG3VOFS picks the row:** `bgSetScroll(2, 0, 0)` reads row 0. The lib
  writes BG3's VOFS raw in Modes 2/4/6 (for a displayed layer it writes
  `y - 1`; see KNOWN_LIMITATIONS, "Vertical scroll is off by one").

Here BG1 is a diagonal 256-colour gradient (32 8bpp tiles built from pixels
with `tileEncode8bpp()`, a 256-entry palette computed at boot), and the same
sine goes into the row as vertical words: the columns ripple. Press **A**
and it goes in as horizontal words: the columns slide sideways, 8 pixels at
a time. **B** again goes back to Mode 2.

Both scenes are built in VRAM at boot (Mode 2 at `$0000`/`$2000`, Mode 4 at
`$1000`/`$2400`, the offset table shared at `$3000`); a switch is
`setMode`, BG1's two pointers and the palette, in VBlank.

## What to observe / if it breaks

- **Correct run:** rainbow bands ripple as a travelling wave; each column is
  at its own vertical offset, all from one 32-entry BG3 table.
- **Bands stay flat:** the offset table never reached VRAM. The classic cause
  is computing it *after* `WaitForVBlank` — the upload then runs past VBlank
  and the PPU drops it. Build the table during active display, DMA it the
  instant VBlank starts (what this example does).
- **Only the leftmost column is wrong:** column 0 cannot be offset by OPT —
  that is hardware, not a bug.
- **Whole layer shifts uniformly (no ripple):** your offset words are missing
  the `0x2000` enable bit, so the PPU ignores them and uses the base scroll.

Probe oracle: `wave_phase` advances every frame. The luna manifests
`dma_mode2_opt_table.toml` (Mode 2: the two rows, word by word, at a given
loop iteration) and `backgrounds_mode2_mode4.toml` (B, then A: the vertical
words, then the horizontal ones) check it.

## Controls

| Button | Action |
|--------|--------|
| B | Switch between Mode 2 and Mode 4 |
| A | In Mode 4: vertical words ⇄ horizontal words |

## Build & run

```bash
make
../../../testing/bin/luna run -n 3000000 mode2.sfc
```

## Modules used

`console`, `dma`, `background`, `input`, `math`, `tile` (the run-time tile encoder, `tileEncode4bpp` and `tileEncode8bpp`)

## Where you are

← the rest of the family covers the everyday modes
([mode1](../mode1/), [mode0](../mode0/), [mode3](../mode3/),
[mode5_hires](../mode5_hires/)); this rung is the specialist OPT modes. For per-scanline
(rather than per-column) tricks, see the [hdma](../../hdma/) family.
