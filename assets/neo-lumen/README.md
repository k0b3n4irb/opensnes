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
| `backgrounds/canal_256.png` | the same, with a haze added by hand: each row blended towards a magenta glow (`#e05090`, at most 42 %, strongest on the horizon row 118) and towards the night (`#101028`, up to 30 % at the top, 22 % at the bottom), snapped to the 15-bit grid, then cut to 256 colours by median cut without dithering: 253 colours | the blend above, by hand | `examples/backgrounds/mode3` (`res/canal.png`); `examples/backgrounds/mode2` (`res/canal.png`, eight mirrored rows added below, `mode2_canal`) |
| `backgrounds/city_far.png` | far skyline at night in the rain, 256x224 | `generate-image-v2`, seed 1018, the city's 15 colours | a source of `parallax_city.png`; `examples/chips/sa1_starfield` (`res/skyline.png`, behind the moths of `build.py`'s `MOTE`) |
| `backgrounds/city_near.png` | close apartment blocks and a noodle sign, 256x224 | the same, seed 1018 | a source of `parallax_city.png`; `examples/color/shadow_tint` (`res/street.png`) |
| `backgrounds/parallax_city.png` | three bands on 512x256, composed by hand: rows 20-91 of `city_far` (72 lines), rows 92-179 of `city_far` under rows 136-223 of `skyline_mid` (88 lines), rows 160-223 of `city_near` (64 lines); the left 256 pixels, then their mirror; 16 colours by median cut | composition | `examples/scrolling/parallax_scroll` (`res/back.png`) |
| `backgrounds/mist.png` | mist wisps, 256x224, 2 colours (0 transparent, 1 = `#283050`), seamless at 256 | procedural, below | `examples/color/transparency` (`res/mist.png`, BG3 added over the street) |
| `backgrounds/alley.png` | a dark back alley at night in the rain, 256x224, 11 colours | `generate-image-v2`, seed 1501, `street.png` as `style_image` (its colours off), the city's 15 colours as `target_palette` | `examples/windows/window` (`res/alley.png`), `examples/sprites/sprite_swarm` (`res/alley.png`, behind the moths of `build.py`'s `MOTH`) |
| `backgrounds/street.png` | a rainy street at night, 256x224, 12 colours of the city palette | `generate-image-v2`, seed 112, `target_palette` = the city's 15 colours | `examples/backgrounds/mode1` (`res/street.png`: indexed, padded to 256x256 with the ink colour), and the same file in `examples/backgrounds/mode2` |


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
| `characters/drone.png` | a one-eyed security drone | 105, #32 | `examples/basics/aim_target` (`res/units.png`: as is, and its cyan twin, the orbiter) |
| `characters/automaton.png` | a patrol automaton with a riot shield, side view | 106, #8 | — |
| `characters/insect.png` | a crawling robot insect, side view | 107, #13 | — |
| `characters/hunter.png` | a security hunter aircraft, from above | 108, #10 | — |
| `characters/bike.png` | Kit on a hover-bike, from behind | 109, #40 | — |
| `characters/glider.png` | Kit's delivery glider, from above | 110, #51 | `examples/mode7/rotate_scale` (`res/glider.png`) |

`characters/topdown/<who>_<south|east|north|west>.png`: the first six
registered as PixelLab characters with `create-character-v3` (`view: high
top-down`, 4 directions, the front pose above as `reference_image`, seeds
2000 to 2005 in the order Kit, raccoon, owl, pangolin, frog, rat). Their
south and north views are the residents of `examples/sprites/world_sprites`
(`res/residents.png`, laid out by `devtools/art_assets/build.py`'s
`residents`: one OBJ palette each; the frog's two views, 17 colours
together, quantized to 15).

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
| `characters/kit_64.png` | Kit, front, 64x64 (the 32 one drawn larger) | `generate-image-v2`, seed 1301 (#0), `characters/kit.png` as a reference image, its 9 colours as `target_palette` | `examples/sprites/sprite_sizes` (`res/sprite64.png`), `examples/sprites/dynamic_sprite` (`res/spritehero32.png`) |
| `characters/kit_16.png` | Kit, three-quarter, 16x16 | `generate-image-v2`, seed 1302 (#0), the same reference and colours | `examples/sprites/sprite_sizes` (`res/sprite16.png`), `examples/sprites/dynamic_sprite` (`res/spritehero8.png`); its 8x8 one is drawn by `devtools/art_assets/build.py` (`KIT8`) |
| `characters/kit_tall.png` | Kit, front, 32x48 | `generate-image-v2`, seed 1401 (#24), `characters/kit_64.png` as reference, its 7 colours as `target_palette` | the first frame of the two animations below; `examples/sprites/dynamic_sprite` (`res/spritehero16.png`) |
| `characters/kit_tall_sheet.png` | Kit 32x48, eight frames: walk in place 0-3, wave 4-7 | `animate-with-text-v3` from `kit_tall.png`, 4 frames each (seeds 1411 and 1412; the first frame, a copy of the input, dropped), each pixel put back on the 7 colours of `kit_tall.png` | `examples/sprites/aseprite_pipeline` (`res/hero.png`, `res/hero.json`) |
| `characters/kit_torch.png` | Kit, side view facing right, holding a torch out, 32x32 | `generate-image-v2`, seed 1502 (#17), the side sheet's frame 0 as reference, its colours with a white and a grey | `examples/windows/window` (`res/kit.png`) |
| `characters/kit_side_sheet.png` | Kit seen from the side, 32x32 frames, four a row: idle 0-3, walk 4-11, run 12-19, jump 20-27 (block 28 is the owl's, unused) | PixelLab: the character with an east view, its `breathing-idle`, `running-8-frames` and `jumping-1` templates and a v3 walk, all put back on his 15 colours with `reduce-colors` (2026-10-18) | `examples/games/likemario`, `examples/maps/map_scroll` (`res/kit.png`) |

### The machines' cycles

`characters/walk/insect_<0-3>.png`, `automaton_<0-3>.png`, `drone_<0-3>.png`: four
frames each (the frames after the first one PixelLab returns), `animate-with-text-v3`
on `insect.png`, `automaton.png` and `drone.png` (a crawl, a walk with the shield, a
hover), seeds 5000 to 5002; no palette is given there, so the build's conversion
puts them back on the machines' 15 colours (`devtools/art_assets/build.py`). The
insect and the automaton walk in `examples/demos/mapandobjects`.

### The logo

| File | What | Made with | Used by |
|---|---|---|---|
| `logo/kit_mascot.png` | Kit in a logo pose, a fist raised, 128x128 | `generate-image-v2`, Kit's front pose as style, his 15 colours, seed 6000, candidate #0 of 4 | `assets/logo_256x256.png`, `logo_256x224.png`, `logo.svg`; the `res/logo.png` of `examples/hdma/gradient_colors` and `hdma_helpers` |
| `logo/kit_head.png` | his head and shoulders, 64x64 | the same, seed 6001, #0 of 16 | `assets/logo_128x128.png`, `logo_64x64.png` |
| `logo/kit_icon.png` | his head as an icon, 32x32 | the same, seed 6000, #63 of 64 | `assets/logo_32x32.png`, `favicon.ico` |

The wordmark and the 16x16 icon are drawn in `devtools/art_assets/logo.py`
(OPEN in the cyan neon, SNES in the magenta, letters on a 6x7 grid with
two-pixel stems), which composes the eight files of `assets/` (`make art-assets`).

### The Mode 7 city

A Mode 7 map names at most 256 distinct 8x8 tiles, and the aerial view has
3448: cut down by clustering, it smears. So the ground is laid out from
pieces instead (`devtools/art_assets/mode7.py`, `make art-assets`): 235
tiles, 16 colours.

| File | What | Made with | Used by |
|---|---|---|---|
| `tiles/wang_street_roof.png` | 16 Wang tiles of 16x16, street (lower) to roof (upper) with a neon curb; tile `NW<<3 \| NE<<2 \| SW<<1 \| SE` (1 = roof) at `(k % 4, k / 4)` | `create-tileset`, standard, `high top-down`, `transition_size` 0.25, 16x16 crops of `aerial.png` as lower and upper references and the whole of it as `color_image`, seed 7100 | the Mode 7 ground below |
| `tiles/wang_street_canal.png` | the same from street to canal water, on the first set's street tile (`lower_base_tile_id`) | the same, seed 7101 | the same |
| `backgrounds/aerial.png` | the city from above, 512x512 | `generate-image-v2`, seed 115, the city's 15 colours | the plaza (a disk of radius 40 round its centre), two helipads and a skylight, stamped on the ground |
| `backgrounds/skyline_mid.png` | mid-distance towers, 512x224, transparent sky | `generate-image-v2`, seed 113, the city's 15 colours | the bottom rows are in `parallax_city.png`; at half size, mirrored to wrap at 512, the sky of the Mode 7 city |

The ground is `res/ground.png` of `examples/mode7/perspective`, `dsp1_ground`
and `perspective_rotate`, and `res/mode7bg.png` of `rotate_scale`; the sky is
`res/sky.png` of the first two. The sky's 16 colours are the ground's first
sixteen, because its 4bpp layer reads CGRAM 0-15, the start of the ground's
palette.

### The metro over the city (EXTBG)

`examples/mode7/extbg`: `res/plane.png` is a 256x224 window of the same
ground (from x 160, y 400), with three viaducts of the elevated metro, a
three-car train and the viaducts' shadows drawn over it by
`devtools/art_assets/mode7.py` (`extbg_plane`), repeated over 1024x1024:
93 tiles. The viaducts' 13 colours are new (concrete greys, the rails, the
train's roof, windows and stripe) and sit at indices 1-13 with bit 7 set;
the city's colours start at 16. `res/orb.png`, the energy orb (O6), eight
16x16 frames of a pulse, is drawn by the same script (`orb`). No new
PixelLab call.

### Kit on the roofs (continuous_scroll)

`examples/scrolling/continuous_scroll`: `res/roofs.png` is roof pieces 0,
3, 1 and 4 of likemario side by side at one height (512x256, 600 tiles),
`res/skyline.png` likemario's far towers, `res/kit.png` the idle (0-3) and
run (12-19) rows of `characters/kit_side_sheet.png`, the run frames moved
4 px down so that Kit's feet are on the same row in both. Made by
`devtools/art_assets/build.py` (`build_continuous_scroll`); no new
PixelLab call.

### The title screen (mixed_scroll)

`examples/scrolling/mixed_scroll`: `res/logo.png` is the 256x224 logo on a
transparent sky (the one of the HDMA examples, `logo.py`'s `screen_logo`),
`res/rain.png` the night behind it, drawn by `devtools/art_assets/build.py`
(`rain`): a 128x128 motif repeated to 256x256 (102 tiles, 13 colours) of
faint diagonal bands, far lights out of focus and rain streaks falling one
pixel left for two down — the direction the layer scrolls. No PixelLab call.

### The tram's portholes (window_multi_hdma)

`examples/windows/window_multi_hdma`: `res/city.png`, the city at dusk seen
from an aerial tram, 256x256 so that the scroll wraps both ways: the
mid-distance towers (`backgrounds/skyline_mid.png`) at half size over a
gradient of 168 rows (one colour a row), their reflection in a canal, by
`devtools/art_assets/build.py` (`tram_view`): 546 8bpp tiles, 104 colours,
colour 0 the tram's wall (the backdrop around the portholes). No PixelLab
call.

### The noodle bar's sign (palette_cycle)

`examples/color/palette_cycle`: `res/sign.png`, drawn by
`devtools/art_assets/build.py` (`neon_sign`): a brick wall, the word NOODLES
in a neon frame (the library's font at three times its size, each letter on
its own colour index, 8-14 of palette 0) and a coolant pipe (its liquid on
indices 8-15 of palette 1, along x), for the example to rotate.

### The save terminal (save_game)

`examples/memory/save_game`: `res/terminal.png`, a terminal's bezel and
glass with scanlines and the outlines of two slot cards, four colours
(BG2 of Mode 0), drawn by `devtools/art_assets/build.py` (`terminal`);
`res/icon.png` is `logo/kit_icon.png` (`save_icon`).

### The pad test (controller)

`examples/input/controller`: `res/screen.png`, the same terminal's bezel and
glass with two generic controllers (no console maker's shape) and the floor
grid of a test bay, four colours (BG2 of Mode 0, `pad_screen`); `res/cast.png`,
Kit and the raccoon from above in four facings (`characters/topdown/`), then
four 8x8 lamps (a disc, a square, a pill, half a shoulder) that the example
lays over a button while it is held (`pad_sprites`). The lamps' places on a
pad are `PAD_LAMPS` in `build.py`, copied in `main.c`.

### The bar's menu screen (pixel_text)

`examples/text/pixel_text`: `res/terminal.png`, save_game's terminal bezel and
glass with a bowl of noodles (its steam, its chopsticks) over a message panel,
four colours (BG2 of Mode 0, `bar_terminal`). The panel's outline is
`BAR_PANEL` in `build.py`, round the layer `main.c` composes; the text and its
5x7 font are not an asset, they are bits in `main.c`.

### The play square (game_skeleton)

`examples/basics/game_skeleton` (and `basics/scene_stack`, the same field and logo): `res/field.png` is a piece of
world_sprites' square round its ring (`skeleton_field` in
`devtools/art_assets/build.py`) with a kerb round the field and a dark band
for the HUD; `res/logo.png` the screen logo (`screen_logo`, `logo/`);
`res/cast.png` (`skeleton_sprites`) Kit's walk from above and the data shard
(`props/shard.png`) turning in eight frames (`spin`).

### The warehouse (collision_demo)

`examples/basics/collision_demo`: `res/cells.png`, four 16x16 cells drawn
by `devtools/art_assets/build.py` (`warehouse_cells`, colours `WAREHOUSE`):
the concrete floor, a crate seen from above (boards, a cross of straps, a
neon tag), a wall plate of corrugated metal with its cyan light, the floor
with a drain grate. `res/cast.png` (`collision_sprites`) lays out Kit's walk
from above (`characters/topdown/kit_walk_sheet.png`) and the drone's hover
(`characters/walk/drone_*.png` on the cast's MACHINE colours), with a third
palette where the drone's pink lights are green.

### The turret on the roof (aim_target)

`examples/basics/aim_target`: `res/units.png` and `res/roof.png`, by
`devtools/art_assets/build.py`. `turret` draws a 32x32 security turret from
above in sixteen headings (a round mount, a head and a barrel turned by
geometry, a pink lens and muzzle, colours `TURRET`); `aim_sprites` lays them
out four a row with the security drone and its cyan twin after them;
`rooftop` draws the roof, four dark colours (`ROOFTOP`), with the turret's
pad and a stairwell's hatch. No PixelLab call beyond the drone.

### A square of the Rue basse (world_sprites)

`examples/sprites/world_sprites`: `res/plaza.png`, 512x512, drawn by
`devtools/art_assets/build.py` (`plaza`, colours `PLAZA`): running-bond
paving slabs in two tones, a kerb with a cyan neon strip round the 512x448
world, a pink ring inlaid at its centre, five manhole covers and five
puddles holding a sign's neon. 275 tiles with mirrored blocks merged. No
PixelLab call.

### The terminal's cursor (mouse)

`examples/input/mouse`: `res/cursor.png`, a 16x16 arrow drawn pixel by pixel
in `devtools/art_assets/build.py` (`CURSOR`): cyan, a white edge, a dark
outline, a magenta hot spot at its top-left pixel.

### The security range (superscope)

`examples/input/superscope`: `res/aim_adjust_target.png`, the calibration
target (2bpp: rings and a cross over the panels of the range's wall), and
`res/marker.png`, the 16x16 impact marker, both drawn by
`devtools/art_assets/build.py` (`scope_target`, `scope_marker`). They replace
PVSnesLib's 48-colour sprite sheet, of which the example used one dot.

### Neon Breaker (the arcade cabinet)

`examples/games/breakout`: everything in `res/` is drawn by
`devtools/art_assets/breakout.py`, in the tile numbering the game addresses:
the cabinet (a steel bevel with a magenta neon tube, a speaker grille), a
16x8 brick in eight colours (one BG palette each), the paddle with its
cyan emitters, the ball, their shadows, the lives icon, the library's 8x8
font with a highlight and a shadow, and BG3's circuit board in seven
level colours. The colours are the city's, with a green and a red for two
bricks. No PixelLab call.

### The well (blocks)

`examples/games/blocks`: `res/tiles.png` and `res/font2bpp.bin` are drawn by
`devtools/art_assets/blocks.py`, in the tile numbering the game addresses:
seven data containers (two grooves and a lit diode, one colour per piece),
the ghost, breakout's cabinet frame (its steel beam and magenta neon tube,
in three tiles the map flips round the well), and the library's 8x8 font at
its ASCII codes with a shadow, in 4bpp for the HUD and 2bpp for BG3's
messages. The HUD's label colours and the messages' cycle are in `data.asm`
and `main.c`, the gradient behind the well in `render.c`. No PixelLab call.

### The race on the rooftops

`examples/mode7/mode7_racing`: a circuit round a lagoon and three
hover-bikes (`devtools/art_assets/racing.py`, `make art-assets`): 227 tiles,
19 colours. The 64x64 `color_image` of the two tilesets is the city's 15
colours with a curb red (`#e03838`) and white (`#e8e8f0`), in stripes; the
lower and upper references are 16x16 noise in those colours, drawn by a
script (without them the asphalt came out with white lane lines in random
places, seed 7200).

| File | What | Made with | Used by |
|---|---|---|---|
| `tiles/wang_track_gravel.png` | 16 Wang tiles of 16x16, asphalt (lower) to rooftop gravel (upper) with a red and white curb; keyed like the city's | `create-tileset`, standard, `high top-down`, `transition_size` 0.25, seed 7201 | the circuit |
| `tiles/wang_gravel_lagoon.png` | the same from that gravel to the lagoon's water, a quay with amber lamps between, on the first set's gravel tile (`lower_base_tile_id`) | the same, seed 7202 | the infield and the border |
| `props/shard.png` | a data shard, a faceted crystal, 16x16 | `generate-image-v2`, seed 140 (#6), the city's 15 colours with white | `examples/games/likemario`: turned into eight frames by `devtools/art_assets/build.py`; `examples/sprites/dynamic_sprite`: into twenty-four; `examples/sprites/world_sprites`: as is, the shards on the square |
| `props/beacon.png` | a rooftop radio mast with a magenta beacon, 32x64 | `generate-image-v2`, seed 141 (#3), the same colours | `examples/games/likemario`: the goal, stamped on the last roof |
| `props/roof_helipad.png`, `roof_aircon.png`, `roof_aircon_violet.png`, `roof_tank.png`, `roof_solar.png`, `roof_dish.png` | rooftop props from above, 32x32 | `generate-image-v2`, seeds 130 (#24), 131 (#1, #12), 132 (#52), 133 (#0), 134 (#2), the city's 15 colours with the curb's two | scattered on the gravel, solid |
| `characters/racebike_kit.png`, `racebike_white.png`, `racebike_crimson.png` | Kit's racing hover-bike from above, pointing up, and two rivals in other colours; 11 colours for the three | `generate-image-v2`, seed 120 (#0, #23, #33), `glider.png` as `style_image` (its colours off), Kit's colours with the city's magenta, amber and violet | the three bikes; the rivals' turned frames are made from these by the script |

### More backgrounds, not used yet

| File | What | Seed | Note |
|---|---|---|---|
| `backgrounds/noodlebar.png` | the noodle bar from above, 256x224 | 114 | an RPG interior; `examples/basics/panel_hud` (`res/noodlebar.png`, under the HUD, beside `res/boxes.png`, its neon 9-slice and hearts drawn by `devtools/art_assets/build.py`'s `panel_sheet`) |
| `backgrounds/title.png` | moon and skyline with room for a logo, 256x224 | 116 | its top and bottom four rows are an amber frame: repaint them before use; `examples/transitions/fading` (`res/moonrise.png`: `build.py`'s `moonrise` paints the frame out with the rows next to it) |

Both with `generate-image-v2` and the city's 15 colours.

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
