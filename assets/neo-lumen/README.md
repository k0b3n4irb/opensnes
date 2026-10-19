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
| `backgrounds/canal_haze.png` | the canal with the haze below, before the cut to 256 colours: 392 colours on the 15-bit grid | the blend below, by hand | `examples/color/hicolor_1792` (`res/canal.png`; the converter cuts each 64x8 segment to 15 colours) |
| `backgrounds/canal_256.png` | the same, with a haze added by hand: each row blended towards a magenta glow (`#e05090`, at most 42 %, strongest on the horizon row 118) and towards the night (`#101028`, up to 30 % at the top, 22 % at the bottom), snapped to the 15-bit grid, then cut to 256 colours by median cut without dithering: 253 colours | the blend above, by hand | `examples/backgrounds/mode3` (`res/canal.png`) |
| `backgrounds/city_far.png` | far skyline at night in the rain, 256x224 | `generate-image-v2`, seed 1018, the city's 15 colours | a source of `parallax_city.png` |
| `backgrounds/city_near.png` | close apartment blocks and a noodle sign, 256x224 | the same, seed 1018 | a source of `parallax_city.png` |
| `backgrounds/parallax_city.png` | three bands on 512x256, composed by hand: rows 20-91 of `city_far` (72 lines), rows 92-179 of `city_far` under rows 136-223 of `skyline_mid` (88 lines), rows 160-223 of `city_near` (64 lines); the left 256 pixels, then their mirror; 16 colours by median cut | composition | `examples/scrolling/parallax_scroll` (`res/back.png`) |
| `backgrounds/mist.png` | mist wisps, 256x224, 2 colours (0 transparent, 1 = `#283050`), seamless at 256 | procedural, below | `examples/color/transparency` (`res/mist.png`, BG3 added over the street) |
| `backgrounds/street.png` | a rainy street at night, 256x224, 12 colours of the city palette | `generate-image-v2`, seed 112, `target_palette` = the city's 15 colours | `examples/backgrounds/mode1` (`res/street.png`: indexed, padded to 256x256 with the ink colour) |


### Characters (32x32, index 0 transparent once indexed)

Each one is the candidate kept from a sheet of 64 that `generate-image-v2`
returns at 32x32 for 10 generations, with Kit's front pose as `style_image`
and the character's own 15 colours as `target_palette`.

| File | Who | Seed, candidate | Used by |
|---|---|---|---|
| `characters/kit.png` | Kit, the fox-kit courier, front | 1018, #0 | `examples/sprites/simple_sprite` (`res/kit.png`) |
| `characters/owl.png` | the owl who keeps the noodle bar | 33, #2 | — |
| `characters/raccoon.png` | the raccoon hacker, the second player | 101, #22 | — |
| `characters/pangolin.png` | the pangolin guard | 102, #1 | — |
| `characters/frog.png` | the frog mechanic | 103, #16 | — |
| `characters/rat.png` | the rat junk dealer | 104, #9 | — |
| `characters/drone.png` | a one-eyed security drone | 105, #32 | — |
| `characters/automaton.png` | a patrol automaton with a riot shield, side view | 106, #8 | — |
| `characters/insect.png` | a crawling robot insect, side view | 107, #13 | — |
| `characters/hunter.png` | a security hunter aircraft, from above | 108, #10 | — |
| `characters/bike.png` | Kit on a hover-bike, from behind | 109, #40 | — |
| `characters/glider.png` | Kit's delivery glider, from above | 110, #51 | — |

`characters/topdown/<who>_<south|east|north|west>.png`: the first six
registered as PixelLab characters with `create-character-v3` (`view: high
top-down`, 4 directions, the front pose above as `reference_image`, seeds
2000 to 2005 in the order Kit, raccoon, owl, pangolin, frog, rat).

`characters/topdown/walk/<who>_<direction>_<0-4>.png`: a walk of five
frames per direction for each of the six, `POST /characters/animations` in
`mode: v3` on the character above, `frame_count` 4 with
`keep_first_frame: false` (PixelLab returned five), the character's 15
colours as `target_palette`, seeds 3000 to 3005 in the same order. Frames
come on a 40x40 canvas.

`characters/topdown/kit_walk_sheet.png`: Kit's walk down, up and right
(15 frames), each cropped to 32x32 with one window for all (feet aligned),
indexed (0 transparent, then his 15 colours) and stacked in a column; used
by `examples/sprites/animated_sprite` (`res/kit_walk.png`).

### Rooftop pieces and Kit from the side

| File | What | Made with | Used by |
|---|---|---|---|
| `tiles/roof_0.png`, `tiles/roof_1.png` | two rooftops, 128x128, transparent sky, the neon strip's top on row 62 | `generate-with-style-v2`, two 128x96 crops of `city_far` as style, the city's 15 colours, seed 31 | `examples/games/likemario`, `examples/maps/map_scroll`, `examples/backgrounds/mode0` (`roof_0`) (through `devtools/art_assets/build.py`) |
| `tiles/roof_2.png` to `tiles/roof_5.png` | four more in their style, the strip on the same row | `generate-with-style-v2`, `roof_0` and `roof_1` as style, seed 4100 | `examples/games/likemario`, `examples/maps/map_scroll` (the same) |
| `characters/kit_side_sheet.png` | Kit seen from the side, 32x32 frames, four a row: idle 0-3, walk 4-11, run 12-19, jump 20-27 (block 28 is the owl's, unused) | PixelLab: the character with an east view, its `breathing-idle`, `running-8-frames` and `jumping-1` templates and a v3 walk, all put back on his 15 colours with `reduce-colors` (2026-10-18) | `examples/games/likemario`, `examples/maps/map_scroll` (`res/kit.png`) |

### More backgrounds, not used yet

| File | What | Seed | Note |
|---|---|---|---|
| `backgrounds/skyline_mid.png` | mid-distance towers, 512x224, transparent sky | 113 | a parallax layer; its bottom rows are in `parallax_city.png` |
| `backgrounds/noodlebar.png` | the noodle bar from above, 256x224 | 114 | an RPG interior |
| `backgrounds/aerial.png` | the city from above, 512x512 | 115 | to bring down to 256 tiles for Mode 7 |
| `backgrounds/title.png` | moon and skyline with room for a logo, 256x224 | 116 | its top and bottom four rows are an amber frame: repaint them before use |

All four with `generate-image-v2` and the city's 15 colours.

An example keeps its own copy in its `res/` folder, in the form its
converter takes (indexed, padded, cut); this folder keeps the picture as
it came out, so the next example starts from the same source.

## The mist's recipe

For each pixel, with `t = 2*pi*x/256` (every term has a whole number of
periods across the width, so the picture wraps):

```
yy    = y + 9 sin(2t + 0.6) + 5 sin(3t + 0.03 y) + 3 sin(5t + 1.9)
v     = sin(2*pi*yy / 60) + 0.55 sin(t + 0.05 y) + 0.30 sin(4t - 0.07 y + 0.8)
d     = v - 0.75
pixel = 1 if d > 0.35, or if d > -0.05 and (d + 0.05) / 0.4 * 16 > the 4x4 Bayer value at (x, y)
```
