# Pixel Text

![Screenshot](pixel_text.png)

Four lines of a narrow font at a 6-pixel pitch, none of them on the tile
grid, composed in RAM and sent to VRAM: the `text` module puts one glyph per
tile, and a game that wants proportional text, or a label at any x and y,
composes its own 2bpp layer. The primitive is `tileBlitGlyph2bpp()`, which
ORs rows of a glyph into one tile and the tile to its right at a pixel
shift; two calls per glyph cover the four tiles a glyph at (x, y) can touch.
Press A and every line moves one pixel right, the glyphs crossing tile
boundaries as they go. No assets: a 5x7 font declared as bits becomes 2bpp
glyphs at boot.

## SNES Concepts

- A 2bpp tile is 16 bytes, planes 0 and 1 interleaved per row. A layer of
  160 tiles is 2560 bytes in FAR RAM, and tile n of the layer is tile n of
  the sheet in VRAM: the tilemap only places the tiles, the pixels are
  whatever the layer holds.
- A glyph at x covers tile x / 8 and, shifted by x % 8, the tile after it;
  at y it covers rows y % 8 to 7 of one tile row and rows 0 to y % 8 - 1 of
  the next. The slot to the right is the next tile of the layer, the slot
  below is one row of tiles further.
- The call only ORs, so the layer is cleared with `memset()` before a new
  layout, and glyphs may overlap.
- After a change the layer goes to VRAM in two halves of 1280 bytes, one
  per VBlank, inside the DMA budget of a frame.
- Cost: about 12 000 master cycles a glyph for its two calls (measured on
  luna), where the same work in C costs 40 000 to 50 000.

## Controls

| Button | Action |
|--------|--------|
| A | Shift every line one pixel to the right (six steps, then back) |

## How to Build

```bash
make
```

## Modules Used

`console`, `dma`, `background`, `input`, `tile`, `string`
