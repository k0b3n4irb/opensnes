# opensnes-image — full-screen pictures and their tables {#tools_opensnes_image}

The picture tool of the `opensnes-*` family (@ref tools_conventions), for the
screens that are not a tileset-and-tilemap: the **HiColor** technique, and
the **Mode 7 perspective** tables that turn a flat Mode 7 plane into a
road receding to the horizon. Both were maintainer scripts until 2026-10
(`hicolor64.py`, `m7ptables.py`); they are compiled now, so a game's build
needs no Python. A Mode 7 *picture* is `opensnes-tileset convert --mode 7`
(@ref tools_opensnes_tileset).

## HiColor

```sh
opensnes-image hicolor res/sunset.png --save
```

**In:** a 256x224 picture, RGB or RGBA (nothing is transparent: the
technique reloads whole palettes). **Out**, beside it or in `--out`:

| File | What it is |
|---|---|
| `sunset.pic` | 896 sequential 4 bpp tiles (28 tile rows of 32), no deduplication: 28672 bytes of VRAM |
| `sunset.pal` | one 16-colour palette per 64x8 segment — 28 rows x 4 segments x 32 bytes = 3584 bytes; colour 0 of each is black, the other 15 are the segment's own |
| `sunset.inc`, `sunset_data.as` | the glue: `sunset_tiles`, `sunset_pal` (each with `_end`), `SUNSET_ROWS`, `SUNSET_SEGMENTS`, `SUNSET_TILES_SIZE`, `SUNSET_PAL_SIZE`; the `.incbin` fragment the build gathers |

Load the tiles once and stream the palettes into CGRAM from an H-IRQ, 16
bytes per scanline, while the PPU draws: every tile row shows 64 colours
of its own, 1792 palette entries on a screen whose mode allows 128.
`examples/color/hicolor_1792` is the whole mechanism (krom's
HiColor64PerTileRow), and its tilemap alternates palette banks 0-3 and 4-7
by tile row so the stream refills one bank while the other displays.

Each segment is quantized to 15 colours by median cut and then `--passes`
rounds of k-means (3 by default; 0 keeps the median cut). The result is
deterministic: the same picture gives the same bytes on every OS.

## Perspective

```sh
opensnes-image perspective res/perspective.toml --angles 48 --zoom 80 --save
```

A **composed asset** with no source file: `res/perspective.toml` holds the
parameters and the build regenerates the tables from them.

```toml
tool = "opensnes-image"

[perspective]
angles = 48     # rotation steps over a full turn
lines = 224     # scanlines per table
zoom = 80       # the scale at the first scanline
```

For each angle, one HDMA table per Mode 7 matrix term — cos, sin and -sin —
with `entry(line) = trig(angle) * zoom * 256 / line` in 8.8 fixed point:
an 80x zoom-out at the top shrinking hyperbolically down the screen, the
perspective divide. Each table is `lines` entries of `[count 1][word]` and
a terminator (673 bytes at 224 lines), in `perspective_cos.hdma`,
`perspective_sin.hdma`, `perspective_nsin.hdma`, with `perspective.inc`
(`perspective_cos`, `_sin`, `_nsin`, `PERSPECTIVE_ANGLES`, `_LINES`,
`_STRIDE`, `_ZOOM`) and `perspective_data.as`.

```c
#include "res/perspective.inc"

u16 off = angle * PERSPECTIVE_STRIDE;
hdmaSetup(HDMA_CHANNEL_0, HDMA_MODE_1REG_2X, HDMA_DEST_M7A, perspective_cos + off);
hdmaSetup(HDMA_CHANNEL_1, HDMA_MODE_1REG_2X, HDMA_DEST_M7B, perspective_sin + off);
hdmaSetup(HDMA_CHANNEL_2, HDMA_MODE_1REG_2X, HDMA_DEST_M7C, perspective_nsin + off);
hdmaSetup(HDMA_CHANNEL_3, HDMA_MODE_1REG_2X, HDMA_DEST_M7D, perspective_cos + off);
```

With the defaults, the tables are krom (Peter Lemon)'s Perspective demo
tables byte for byte — `examples/mode7/perspective_rotate` shipped them
verbatim until the tool regenerated them; the tool's golden suite keeps
his bytes as the reference. `--zoom` above 127 is refused: the first
scanline's entry would not fit a signed 16-bit Mode 7 term.

## Inspect

```sh
opensnes-image inspect res/*.png
# sunset.png: 256x224 px, 15885 distinct colours — a HiColor screen; the busiest 64x8 segment has 64 colours in 15-bit (15 kept)
```

Whether a picture is a HiColor screen and how hard the busiest segment is
quantized; `--json` for a script.

## See also

- @ref tutorial_mode7 — the Mode 7 matrix, HDMA per scanline, the perspective.
- @ref craft_planning — colour budgets, and when 1792 colours are worth an IRQ per scanline.
