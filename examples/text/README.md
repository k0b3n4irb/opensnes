# Text Examples

The SNES has no built-in text rendering. Text is displayed the same way as any
other graphics: font characters are stored as tiles in VRAM, and a tilemap
references those tiles to spell out words on screen.

These examples teach the tile/tilemap fundamentals that underpin all SNES graphics.

## Examples

A ladder of developer questions: *how do I show text? → how do I move it?*

| Example | Rung | Description |
|---------|------|-------------|
| [print_string](print_string/) | 1.1 | Print a string with the built-in font (`textModeInit` + `textPrintAt`) |
| [scroll_message](scroll_message/) | 1.4 | Move text -- scroll the text BG layer for a marquee (`bgSetScroll`) |

> Under the hood — how a glyph is raw 2bpp bitplanes + a tilemap — lives in
> [`fundamentals/text_glyphs`](../fundamentals/text_glyphs/).

## Key Concepts

### How Text Works on SNES

```
Font Image (PNG) --> gfx4snes/font2snes --> Tile Data (VRAM)
                                        --> Tilemap (VRAM)
                                                |
                                        PPU renders screen
```

Each character is an 8x8 pixel tile. A font is a collection of tiles covering
the ASCII range. The tilemap is a grid of tile numbers that defines which
character appears at each screen position.

### 2BPP Tile Format

"2bpp" means 2 bits per pixel = 4 possible colors (indices 0-3).
Each 8x8 tile is 16 bytes: 8 bytes for bitplane 0, 8 bytes for bitplane 1.

### VRAM Layout for Text

```
$0000-$0FFF: Tile data (font graphics)
$1000-$17FF: BG1 tilemap (32x32 = 2KB)
```

### Mode 0 Palette

In Mode 0, each background layer gets 4 colors:
- BG1: Colors 0-3
- BG2: Colors 4-7
- BG3: Colors 8-11
- BG4: Colors 12-15

---

Start with **print_string** to get your first ROM running, then **scroll_message**
to make it move. To see what the `text` module hides, read
**fundamentals/text_glyphs**.
