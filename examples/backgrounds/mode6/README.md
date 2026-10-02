# mode6 — one hi-res layer with offset-per-tile

**Family 2 (Backgrounds) · the offset-per-tile modes (2/4/6), after
[`mode2`](../mode2/README.md) and [`mode4`](../mode4/README.md).**

![Screenshot](mode6.png)

*The screenshot is luna's native 512×448 output: the stripes are one
half-pixel wide.*

Mode 6 is the hi-res member of the offset-per-tile family. It gives you one
4bpp (16-colour) BG1 drawn 512 half-pixels wide, like Mode 5, and BG3 as the
offset table, read like Mode 2's: one row of horizontal offset words, one row
of vertical ones. Its tiles are always 16 half-pixels wide, and so are the
table's columns, so 32 words still cover the screen.

BG1 here is diagonal colour bands drawn in one-half-pixel stripes, which no
256-pixel mode can show. A sine wave written into the vertical row makes the
columns ripple; press **A** and the wave moves to the horizontal row, and the
columns slide sideways.

## Build & Run

```bash
cd $OPENSNES_HOME
make -C examples/backgrounds/mode6
```

Open `mode6.sfc` in luna (or any SNES emulator) and press A.

## SNES Concepts

- **Mode 6**: `setMode(BG_MODE6, 0)` — BG1 4bpp in hi-res, BG3 the offset
  table (not drawn), no BG2
- **Hi-res needs both screens**: the sub screen draws the even half-pixel
  columns and the main screen the odd ones, so BG1 goes on both
  (`setMainScreen(LAYER_BG1)` and `setSubScreen(LAYER_BG1)`), as in Mode 5
- **16-half-pixel tiles**: a map entry for character N draws N on the left
  and N + 1 on the right; this applies to BG3 too, so each offset word moves
  a 16-half-pixel column (anomie-regs, *Backgrounds / Mode 6*)
- **The table, as in Mode 2**: BG3 row 0 = horizontal words, row 1 =
  vertical words; bit 13 = apply to BG1; value in bits 0-9; column 0 is
  never offset
- **BG3VOFS picks the rows**: `bgSetScroll(2, 0, 0)`; the lib writes BG3's
  VOFS raw in Modes 2/4/6
- **4bpp tiles at run time**: 16 characters from pixels with
  `tileEncode4bpp()`, no asset
- **Upload timing**: the row is built during the picture and DMA'd at the
  start of VBlank, as in `mode2`

## What You'll See

- Diagonal rainbow bands with fine vertical stripes, whose columns ripple up
  and down as a travelling wave; the leftmost column stays put.
- **A**: the columns slide sideways instead. **A** again: back to vertical.

## Testing

`tools/luna-test/manifests/backgrounds_mode6.toml` checks the mode, both
screens and BG3VOFS, the wave in the vertical row at frame 100, then in the
horizontal row after A with the vertical row back to zero (VRAM `$6000` and
`$6040`), and that every upload lands in blank. Without the A press it
fails.

## Modules Used

| Module | Purpose |
|--------|---------|
| `console` | System initialization, `setMode`, `WaitForVBlank` |
| `dma` | Palette, tiles, map and the offset rows |
| `background` | BG1 and BG3 bases, BG3 scroll |
| `input` | The A button |
| `math` | `fixSin` for the wave |
| `tile` | The 4bpp characters, built from pixels |
