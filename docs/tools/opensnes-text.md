# opensnes-text — bitmap fonts for the text module {#tools_opensnes_text}

The text tool of the 1.x family (@ref tools_conventions). The text module
ships a font, so you may never need it; when you want your own typeface —
a chunky title face, a themed UI face — `opensnes-text font` turns a
picture of the glyphs into the tiles the text routines draw from. The tile
packer is `font2snes`'s (@ref tools_font2snes), linked as a library: the
bytes are its own.

## Font

```sh
opensnes-text font res/font.png --save          # 2 bpp: the text module's BG3 / Mode 0 font
opensnes-text font res/title.png --bpp 4        # 4 bpp, for a Mode 1 BG1 or BG2 text layer
```

**In:** a PNG of the **96 glyphs of ASCII 32..127**, the space first, in
8x8 cells on any grid: 16 x 6 (128x48 pixels, the usual sheet), 96 x 1
(768x8), 32 x 3. An indexed PNG gives its colour index as is; a grey or
RGB picture is ranked by brightness into the 4 (2 bpp) or 16 (4 bpp)
levels — black is colour 0, the transparent background, white the brightest
colour.

**Out**, beside the input or in `--out`:

| File | What it is |
|---|---|
| `font.pic` | the 96 tiles in glyph order, `--bpp` deep (16 or 32 bytes each: 1536 or 3072 bytes of VRAM) |
| `font.pal` | a grey ramp of 4 or 16 colours, so the font shows before the game sets its colours (`setColor()` writes the real ones) |
| `font.inc`, `font_data.as` | the glue: `font.inc` declares `font_tiles` and `font_pal` (each with `_end`) and `FONT_GLYPHS`, `FONT_FIRST_CHAR`, `FONT_BPP`, `FONT_BYTES_PER_GLYPH`, `FONT_TILES_SIZE`; `font_data.as` is the `.incbin` fragment the build gathers into `assets_gen.asm` |

Loading it is two calls: put the tiles where the text module reads, and
tell it which tile the space is.

```c
#include "res/font.inc"

dmaCopyVram(font_tiles, 0x0000, font_tiles_end - font_tiles);
textInit(TEXT_DEFAULT_TILEMAP_ADDR, 0, 0);   /* first tile 0, palette slot 0 */
```

Glyph pixels are colours 1..3 (or 1..15) of the palette slot; `setColor()`
on colour 1 of the slot is the text colour (@ref tutorial_text).

A picture that is not a font is refused with the reason: sizes that are not
multiples of 8, a cell count other than 96, an index above what `--bpp`
holds (`--bpp 2` of a 16-index picture says so and points at `--bpp 4`).

## Inspect

```sh
opensnes-text inspect res/*.png
# font.png: 128x48 px (grey), 96 cells of 8x8, a font of 96 glyphs, 4 colours used; 1536 bytes of VRAM at 2 bpp, 3072 at 4 bpp
```

The grid, whether it is a font, the colours it uses, the VRAM at each depth;
`--json` for a script.

## Settings beside the asset

`--save` writes `font.png.toml` (`[font]`: `bpp`); the build runs the
conversion from it and links `font_data.as`, like every settings file
(@ref tools_conventions).

## From font2snes

| font2snes | opensnes-text |
|---|---|
| `font2snes font.png font.pic` | `font font.png` |
| `-b 4` | `--bpp 4` |
| `-c font.h` (a C array in the source) | no equivalent: the tiles are `.incbin` data in the asset banks, declared by `font.inc` |

The same tiles, byte for byte (font2snes's golden suite is the family's).
String tables — the strings of a game by language — are the other half of
this tool's name and come after 1.0; the text module draws C strings.

## See also

- @ref tutorial_text — drawing text, colours, the built-in font.
- @ref tools_font2snes — the 0.x tool; shipped for one more version.
