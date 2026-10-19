# Néo-Lumen: the examples' art

The pictures, sprites and tiles the examples are being redrawn with: one
city at night in the rain (the Rooftops, the Low street, the Canal, the Sky,
the Arcade), Kit the fox-kit courier and the people around him, and the
security machines. One palette family, on the 15-bit grid.

## Licence: CC0

Everything in this folder is dedicated to the public domain under
[CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/): copy it,
change it, ship it in a commercial game, no credit needed.

The pictures are **AI-generated** with [PixelLab](https://www.pixellab.ai/)
for OpenSNES, then cut, padded or re-indexed by hand where a row below says
so. The table names the tool and the seed of each, so a picture can be
generated again or varied.

## What is here, and where it is used

| File | What | Made with | Used by |
|---|---|---|---|
| `backgrounds/canal.png` | a city canal at night, 256x224, 29 colours | `generate-image-v2`, seed 113, no palette given | the source of `canal_256.png` |
| `backgrounds/canal_256.png` | the same, with a haze added by hand: each row blended towards a magenta glow (`#e05090`, at most 42 %, strongest on the horizon row 118) and towards the night (`#101028`, up to 30 % at the top, 22 % at the bottom), snapped to the 15-bit grid, then cut to 256 colours by median cut without dithering: 253 colours | the blend above, by hand | `examples/backgrounds/mode3` (`res/canal.png`) |
| `backgrounds/street.png` | a rainy street at night, 256x224, 12 colours of the city palette | `generate-image-v2`, seed 112, `target_palette` = the city's 15 colours | `examples/backgrounds/mode1` (`res/street.png`: indexed, padded to 256x256 with the ink colour) |

An example keeps its own copy in its `res/` folder, in the form its
converter takes (indexed, padded, cut); this folder keeps the picture as
it came out, so the next example starts from the same source.
