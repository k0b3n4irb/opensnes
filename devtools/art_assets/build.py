#!/usr/bin/env python3
"""Write the examples' Néo-Lumen level data from the art in assets/neo-lumen/.

A generator, not a check (like devtools/audio_assets/build.py): the files it
writes are committed, and an example's build only converts them with the
compiled tools. `--check` says whether the committed files are what it
writes.

backgrounds/mode0:
  res/bg0.png .. bg3.png  four 2bpp layers of the city, 256x256, each one
                      palette of 4 (0 transparent): a status panel (BG0,
                      still), the roofs (BG1, fastest), the mid-distance
                      blocks (BG2), the far towers (BG3, slowest); each
                      scrolling layer is a 128-px half and its mirror, so
                      it wraps at 256

maps/map_scroll:
  res/roofs.png       the tiles of one long row of roofs, in the order of
  res/rooftops.tmj    the Tiled map that numbers them, each tile carrying
                      its `attribute` (FF00 solid) for opensnes-level
  res/kit.png         Kit seen from the side, as for likemario

assets/ (the logo, devtools/art_assets/logo.py):
  logo_256x256.png    Kit the mascot over the OPENSNES wordmark; 256x224 the
  logo_128x128.png    same on the night sky (a SNES screen); 128 his head
  logo_64x64.png      over a smaller wordmark; 64 his head; 32 and 16 an
  logo_32x32.png      icon of his head (the 16 drawn by hand); favicon.ico
  logo_16x16.png      the two icons; logo.svg the 256 one as rects
  logo_256x224.png, favicon.ico, logo.svg

mode7/perspective, mode7/dsp1_ground, mode7/perspective_rotate, mode7/rotate_scale
(devtools/art_assets/mode7.py):
  res/ground.png      the city from above, 1024x1024 in 235 distinct 8x8
                      tiles (a Mode 7 map names 256): streets, roofs and
                      two canals from two PixelLab Wang tilesets, the plaza
                      and the helipads cut from the aerial view; rotate_scale
                      calls it res/mode7bg.png
  res/sky.png         the towers over the horizon, 512x256, the ground's
                      first 16 colours (the 4bpp sky reads CGRAM 0-15);
                      perspective and dsp1_ground only
  res/glider.png      Kit's glider from above, 32x32; rotate_scale only

mode7/extbg (devtools/art_assets/mode7.py):
  res/plane.png       a 256x224 window of the same city with three viaducts
                      of the elevated metro and a train over it, repeated
                      over 1024x1024: the viaducts' pixels carry bit 7
                      (colours 0x81-0x8D, repeated at CGRAM 129-141), the
                      city's start at colour 16
  res/orb.png         the energy orb, eight 16x16 frames of a pulse

mode7/mode7_racing (devtools/art_assets/racing.py):
  res/track.png       the circuit on the rooftops round a lagoon, 1024x1024,
                      from two PixelLab Wang tilesets (asphalt and gravel,
                      gravel and water), the start line, two boost pads and
                      the rooftop props
  res/track_class.bin one byte per 8x8 tile: 0 road, 1 gravel, 2 water or a
                      prop, 3 boost
  res/lanes_xy.bin    the rivals' two lanes, 256 points each (x, y as u16)
  res/lanes_heading.bin  their headings, one byte per point (64 = east)
  res/bikes.png       the three hover-bikes in one palette, 32x32 frames four
                      to a row: Kit's, then each rival's turned 0 to 90 degrees

scrolling/continuous_scroll:
  res/roofs.png       four roof pieces of likemario side by side at one
                      height, 512x256
  res/skyline.png     likemario's far towers, 512x256 (the parallax layer)
  res/kit.png         Kit, 32x32: idle (frames 0-3) then run (4-11)

sprites/sprite_sizes:
  res/sprite8.png ... sprite64.png   Kit at 8, 16, 32 and 64 px (the 8
                      drawn here, the 16 and 64 from PixelLab, the 32 is
                      characters/kit.png), each indexed on its own colours

sprites/aseprite_pipeline:
  res/hero.png        Kit 32x48 (characters/kit_tall_sheet.png), eight
                      frames: walk 0-3, wave 4-7
  res/hero.json       the sheet's data as Aseprite's `--data --list-tags`
                      export writes it: per-frame durations, the two tags

sprites/dynamic_sprite:
  res/sprite16_grid.png  the data shard turning, 24 16x16 frames, 8 a row
  res/spritehero8.png, spritehero16.png, spritehero32.png  Kit at 16x16,
                      32x48 and 64x64 (one frame each), on kit.png's colours,
                      the one palette the metasprite scene loads

sprites/sprite_swarm:
  res/moth.png        a neon moth, wings open then closed (two 8x8 frames)
  res/alley.png       the alley of windows/window, behind the swarm

color/palette_cycle:
  res/sign.png        a noodle bar's sign and a coolant pipe on a brick wall,
                      two palettes: the letters on indices 8-14 of palette 0,
                      the coolant on 8-15 of palette 1, the indices the
                      example rotates

chips/sa1_starfield:
  res/mote.png        a moth far away, 4x3 pixels in an 8x8 tile
  res/skyline.png     the far skyline in the rain (backgrounds/city_far.png)

color/shadow_tint:
  res/street.png      the apartment blocks and the noodle sign
                      (backgrounds/city_near.png), indexed

input/mouse:
  res/cursor.png      the terminal's cursor, a 16x16 arrow drawn here

input/superscope:
  res/aim_adjust_target.png  the calibration target, 2bpp: rings and a cross
                      over the panels of the range's wall
  res/marker.png      the impact marker, 16x16

transitions/fading:
  res/moonrise.png    the moon over the skyline (backgrounds/title.png), its
                      amber frame painted out

windows/window:
  res/alley.png       a dark back alley at night (backgrounds/alley.png), BG1
  res/rain.png        rain on nothing, 256x256 of a 128x128 motif, BG2
  res/kit.png         Kit holding a torch, 32x32 (characters/kit_torch.png)

windows/window_multi_hdma:
  res/city.png        the city from an aerial tram at dusk, 256x256 (the
                      mid towers at half size over a gradient, their
                      reflection in a canal), 8bpp; colour 0 is the tram's
                      wall, never in the picture

scrolling/mixed_scroll, the title screen:
  res/logo.png        the 256x224 logo on a transparent sky (as for the
                      HDMA examples), the fixed layer
  res/rain.png        the night behind it, 256x256 of a 128x128 motif
                      (rain falling down-left, far lights), the layer that
                      scrolls

games/breakout (devtools/art_assets/breakout.py), Neon Breaker, the bar's
arcade cabinet, all drawn by the script in the tile numbering the game uses:
  res/tiles.png       128 4bpp tiles shared by BG1 and the sprites (the
                      cabinet, the brick, the paddle, the ball and their
                      shadows, the library's font, the lives icon); its PNG
                      palette is the whole CGRAM image, one BG palette per
                      brick colour
  res/back.png        BG3's circuit board, 13 2bpp tiles
  res/bg1map.bin      the cabinet and the HUD labels, 32x32
  res/bg2map.bin      the board, 32x32, palette 4
  res/backpal.bin     seven levels of BG3 colours (CGRAM 16-23)

hdma/gradient_colors, hdma/hdma_helpers:
  res/logo.png        the 256x224 logo on a transparent sky (index 0, never
                      drawn), so the HDMA gradients show behind Kit

demos/mapandobjects:
  res/tiles.png       roofs with ramps, 8x8 tiles drawn here: the strip and
                      the wall sampled from roof_1, the six slope shapes of
                      the object engine (45 degrees, and 22.5 in two
                      halves) and the vents as code
  res/BG1.m16         the map (the map module's format)
  res/level.t16       one word per tile: its VRAM tile (no palette bits)
  res/level.b16       one word per tile: T_EMPTY, T_SOLID or a T_SLOPE*
  res/level.o16       the objects: Kit, two robot insects, two automatons
  res/kit.png         Kit from the side; res/insect.png, res/automaton.png
                      their walks, 32x32 frames four to a row
  res/skyline.png     BG2, the far towers (likemario's)

games/likemario:
  res/tiles.png       the level's distinct 8x8 tiles, in the order the map
                      numbers them (tile 0 is the empty sky), 128 px wide
  res/BG1.m16         the level map as main.c reads it: width and height in
                      pixels, size in bytes, then one u16 entry per 8x8 cell
  res/map_1_1.b16     one u16 per tile number: $FF00 solid, $0000 empty
  res/skyline.png     BG2: a band of far towers, 512x256 (a half and its
                      mirror), the rest transparent
  res/kit.png         Kit seen from the side, 32x32 frames (the sheet of
                      assets/neo-lumen/characters/kit_side_sheet.png)
  res/enemies.png     the machines, 32x32 four to a row: the robot insect's
                      walk, the drone's hover, each one stomped (squashed by
                      this script), one palette
  res/shard.png       the data shard turning on its axis, eight 16x16 frames
                      made here from props/shard.png
  res/level_objects.h the shards, the machines and their patrols, the
                      checkpoints and the goal, placed from LEVEL's buildings;
                      the beacon (props/beacon.png) is stamped on the last
                      roof of the map

The level is a row of buildings, each one or more 128-px roof pieces at one
height, with gaps between them; it is laid out by LEVEL below. A roof's
walkable surface is the top of its neon strip, row 64 of a piece once the
pieces are moved 2 px down (the strip is on row 62 of every generated
piece): a cell is solid if it is on row 8 of its piece or below.

Run: python3 devtools/art_assets/build.py [--check]
"""
from __future__ import annotations

import argparse
import json
import math
import random
import struct
import sys
from pathlib import Path

from PIL import Image, ImageOps

sys.path.insert(0, str(Path(__file__).resolve().parent))
from logo import build_logo, screen_logo  # noqa: E402  (the logo is a module of its own)
from mode7 import build_city, extbg_plane, glider, orb
from mode7 import indexed as mode7_indexed  # noqa: E402
from breakout import build_breakout  # noqa: E402
from racing import bikes, build_track  # noqa: E402  (and so is the Mode 7 city)

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "assets" / "neo-lumen"
LIKEMARIO = ROOT / "examples" / "games" / "likemario" / "res"
MODE0 = ROOT / "examples" / "backgrounds" / "mode0" / "res"
MAP_SCROLL = ROOT / "examples" / "maps" / "map_scroll" / "res"
CONTINUOUS = ROOT / "examples" / "scrolling" / "continuous_scroll" / "res"
SPRITE_SIZES = ROOT / "examples" / "sprites" / "sprite_sizes" / "res"
ASEPRITE = ROOT / "examples" / "sprites" / "aseprite_pipeline" / "res"
MIXED = ROOT / "examples" / "scrolling" / "mixed_scroll" / "res"
MULTI_HDMA = ROOT / "examples" / "windows" / "window_multi_hdma" / "res"
WINDOW = ROOT / "examples" / "windows" / "window" / "res"
DYNAMIC = ROOT / "examples" / "sprites" / "dynamic_sprite" / "res"
FADING = ROOT / "examples" / "transitions" / "fading" / "res"
SUPERSCOPE = ROOT / "examples" / "input" / "superscope" / "res"
MOUSE = ROOT / "examples" / "input" / "mouse" / "res"
SHADOW_TINT = ROOT / "examples" / "color" / "shadow_tint" / "res"
SWARM = ROOT / "examples" / "sprites" / "sprite_swarm" / "res"
WORLD = ROOT / "examples" / "sprites" / "world_sprites" / "res"
AIM = ROOT / "examples" / "basics" / "aim_target" / "res"
COLLISION = ROOT / "examples" / "basics" / "collision_demo" / "res"
SKELETON = ROOT / "examples" / "basics" / "game_skeleton" / "res"
SCENE_STACK = ROOT / "examples" / "basics" / "scene_stack" / "res"
PANEL_HUD = ROOT / "examples" / "basics" / "panel_hud" / "res"
SAVE_GAME = ROOT / "examples" / "memory" / "save_game" / "res"
CONTROLLER = ROOT / "examples" / "input" / "controller" / "res"
PIXEL_TEXT = ROOT / "examples" / "text" / "pixel_text" / "res"
SA1_FIELD = ROOT / "examples" / "chips" / "sa1_starfield" / "res"
PALETTE_CYCLE = ROOT / "examples" / "color" / "palette_cycle" / "res"
MAPANDOBJECTS = ROOT / "examples" / "demos" / "mapandobjects" / "res"
LOGO = ROOT / "assets"
GRADIENT_COLORS = ROOT / "examples" / "hdma" / "gradient_colors" / "res"
HDMA_HELPERS = ROOT / "examples" / "hdma" / "hdma_helpers" / "res"
MODE7 = ROOT / "examples" / "mode7"
RACING = ROOT / "examples" / "mode7" / "mode7_racing" / "res"
BREAKOUT = ROOT / "examples" / "games" / "breakout" / "res"

# The city's 15 colours (assets/neo-lumen/README.md); index 0 is the sky.
SKY = (16, 24, 40)
CITY = ["#080810", "#101828", "#182838", "#203048", "#281838", "#402858", "#604080", "#8060a8",
        "#183840", "#205058", "#307078", "#48a0a0", "#a02860", "#e05090", "#f0b050"]

ROWS = 30                     # 240 px: the engine's map height
PIECE_SURFACE = 64            # the strip's top inside a piece, after the 2-px move

# (piece numbers left to right, surface row in 8-px rows, gap in px after it).
# A step up is at most 16 px and a gap at most 24, so the normal jump (34 px
# high) crosses every one; the high jump (UP+A) is there to spare.
LEVEL = [
    ((0, 1), 18, 16),
    ((2,), 16, 24),
    ((3, 4), 16, 16),
    ((5,), 14, 24),
    ((1, 0, 2), 16, 16),
    ((4,), 18, 24),
    ((3,), 17, 16),
    ((5, 1), 15, 24),
    ((2, 0), 17, 16),
    ((4, 3), 15, 0),
]


def rgb(h: str) -> tuple[int, int, int]:
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


def palette() -> list[tuple[int, int, int]]:
    return [SKY] + [rgb(c) for c in CITY]


def to_indices(im: Image.Image, pal: list[tuple[int, int, int]]) -> list[list[int]]:
    """RGBA picture -> rows of indices: 0 where transparent, else the nearest colour 1..15."""
    w, h = im.size
    px = im.load()
    out = []
    for y in range(h):
        row = []
        for x in range(w):
            r, g, b, a = px[x, y]
            if a < 128:
                row.append(0)
                continue
            row.append(1 + min(range(15), key=lambda i: sum((p - q) ** 2 for p, q in zip(pal[i + 1], (r, g, b)))))
        out.append(row)
    return out


def load_piece(n: int, pal) -> list[list[int]]:
    """A 128x128 piece moved 2 px down; transparent bottom rows filled by repeating the last 8 opaque ones."""
    rows = to_indices(Image.open(ART / "tiles" / f"roof_{n}.png").convert("RGBA"), pal)
    rows = [[0] * 128, [0] * 128] + rows[:126]
    last = max(y for y, r in enumerate(rows) if any(r))
    for y in range(last + 1, 128):
        rows[y] = list(rows[last - 7 + (y - last - 1) % 8])
    return rows


def compose(level, pieces, stamps=()) -> tuple[int, list[bytes], list[int], list[bool]]:
    """Lay the buildings of `level` out on a ROWS-high canvas: (width in px, the distinct
    8x8 tiles in reading order with the empty one first, one entry per cell, solid per tile).
    `stamps` are (rows of indices, x, y) drawn over it where not 0; they are never solid."""
    width_px = sum(128 * len(p) + gap for p, _, gap in level)
    width_px = (width_px + 7) // 8 * 8
    cols = width_px // 8
    canvas = [[0] * width_px for _ in range(ROWS * 8)]
    solid_cell = [[False] * cols for _ in range(ROWS)]
    x = 0
    for numbers, surface_row, gap in level:
        for n in numbers:
            top = surface_row * 8 - PIECE_SURFACE
            piece = pieces[n]
            for y in range(ROWS * 8):
                py = y - top
                if py < 0:
                    continue
                src = piece[py] if py < 128 else piece[120 + (py - 128) % 8]   # the wall goes on down
                canvas[y][x:x + 128] = src
            for cy in range(ROWS):
                if cy * 8 - top >= PIECE_SURFACE:
                    for cx in range(x // 8, x // 8 + 16):
                        solid_cell[cy][cx] = True
            x += 128
        x += gap
    for rows, sx, sy in stamps:
        for yy, row in enumerate(rows):
            for xx, v in enumerate(row):
                if v:
                    canvas[sy + yy][sx + xx] = v

    empty = bytes(64)
    tiles: list[bytes] = [empty]
    index: dict[bytes, int] = {empty: 0}
    entries = []
    for cy in range(ROWS):
        for cx in range(cols):
            t = bytes(canvas[cy * 8 + yy][cx * 8 + xx] for yy in range(8) for xx in range(8))
            if t not in index:
                index[t] = len(tiles)
                tiles.append(t)
            entries.append(index[t])
    if len(tiles) > 1024:
        sys.exit(f"{len(tiles)} distinct tiles, a map entry numbers 1024")

    solid = [False] * len(tiles)
    seen = [False] * len(tiles)
    clash = 0
    for cy in range(ROWS):
        for cx in range(cols):
            t = entries[cy * cols + cx]
            if seen[t] and solid[t] != solid_cell[cy][cx]:
                clash += 1
            if not seen[t]:
                seen[t] = True
                solid[t] = solid_cell[cy][cx]
    solid[0] = False
    if clash:
        sys.exit(f"{clash} cells whose tile is solid in one place and empty in another")
    return width_px, tiles, entries, solid


def strip_sheet(frames, palname: str) -> Image.Image:
    """Frames of 32x32 RGBA (paths or pictures) on a 128-px wide sheet, four a row,
    indexed on one of the cast palettes."""
    cast = {"MACHINE": ["#080810", "#101828", "#203048", "#384050", "#586878", "#8898a8", "#b0c0d0",
                        "#601838", "#a02860", "#e05090", "#684018", "#b07028", "#f0b050", "#28a0b0", "#e8e8f0"]}[palname]
    pal2 = [(0, 0, 0)] + [rgb(c) for c in cast]
    rows_ = (len(frames) + 3) // 4
    out = Image.new("P", (128, rows_ * 32), 0)
    out.putpalette([v for c in pal2 for v in c] + [0] * (768 - 3 * len(pal2)))
    for n_, f in enumerate(frames):
        im = (f if isinstance(f, Image.Image) else Image.open(f)).convert("RGBA")
        for y in range(32):
            for x in range(32):
                r, g, b, a = im.getpixel((x, y))
                if a < 128:
                    continue
                k_ = min(range(15), key=lambda i: sum((p - q) ** 2 for p, q in zip(pal2[i + 1], (r, g, b))))
                out.putpixel(((n_ % 4) * 32 + x, (n_ // 4) * 32 + y), k_ + 1)
    return out


def tile_sheet(tiles: list[bytes], pal) -> Image.Image:
    """The tiles, 16 to a 128-px row, in their order."""
    sheet = Image.new("P", (128, (len(tiles) + 15) // 16 * 8), 0)
    flat = [v for c in pal for v in c]
    sheet.putpalette(flat + [0] * (768 - len(flat)))
    for k, t in enumerate(tiles):
        tx, ty = (k % 16) * 8, (k // 16) * 8
        for yy in range(8):
            for xx in range(8):
                sheet.putpixel((tx + xx, ty + yy), t[yy * 8 + xx])
    return sheet


def buildings(level) -> list[tuple[int, int, int]]:
    """Each building of `level` as (left x, right x, surface y) in pixels."""
    out, x = [], 0
    for numbers, surface_row, gap in level:
        out.append((x, x + 128 * len(numbers), surface_row * 8))
        x += 128 * len(numbers) + gap
    return out


def spin(im: Image.Image, frames: int = 8) -> list[Image.Image]:
    """A pickup turning on its vertical axis: each frame squeezed by |cos|, mirrored on its back half."""
    out = []
    w, h = im.size
    for k in range(frames):
        a = math.pi * k / frames
        f = max(abs(math.cos(a)), 0.15)
        src = im if math.cos(a) >= 0 else ImageOps.mirror(im)
        fr = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        for x in range(w):
            sx = int((x + 0.5 - w / 2) / f + w / 2)
            if 0 <= sx < w:
                for y in range(h):
                    fr.putpixel((x, y), src.getpixel((sx, y)))
        out.append(fr)
    return out


def squash(im: Image.Image) -> Image.Image:
    """A stomped machine: half its height, standing on the same bottom row."""
    box = im.getbbox()
    body = im.crop(box)
    flat = body.resize((body.width, max(1, body.height // 2)), Image.Resampling.NEAREST)
    out = Image.new("RGBA", im.size, (0, 0, 0, 0))
    out.paste(flat, (box[0], box[3] - flat.height))
    return out


def likemario_objects(level) -> tuple[str, int]:
    """level_objects.h: the shards to pick up, the machines and their patrols, the
    checkpoints (each roof's start) and the goal, from the buildings of `level`."""
    b = buildings(level)
    shards = []
    for x0, x1, sy in b:                      # two at walking height on every roof
        shards += [(x0 + 48, sy - 16), (x0 + 80, sy - 16)]
    for (_, x1, s1), (x2, _, s2) in zip(b, b[1:]):   # one at the top of every jump
        shards.append(((x1 + x2) // 2 - 8, min(s1, s2) - 44))
    # type 0 a robot insect walking on a roof, 1 a drone flying over: x, y (top of the
    # 16x16 box), patrol x min and max
    enemies = []
    for i in (2, 4, 8):
        x0, x1, sy = b[i]
        enemies.append((0, x0 + 128, sy - 16, x0 + 24, x1 - 40))
    x0, x1, sy = b[9]
    enemies.append((0, x0 + 40, sy - 16, x0 + 16, x1 - 112))
    for i, j in ((5, 6), (7, 7)):
        enemies.append((1, b[i][0] + 64, min(b[i][2], b[j][2]) - 52, b[i][0] + 16, b[j][1] - 32))
    checkpoints = [(x0 + 16, sy - 16) for x0, _, sy in b]
    goal = b[-1][1] - 96

    def table(name, ctype, rows):
        body = ",\n".join("    { " + ", ".join(str(v) for v in r) + " }" for r in rows)
        return f"static const {ctype} {name}[{len(rows)}][{len(rows[0])}] = {{\n{body}\n}};\n"

    text = ("/* level_objects.h — generated by devtools/art_assets/build.py; do not edit. */\n"
            f"#define SHARD_COUNT {len(shards)}\n#define ENEMY_COUNT {len(enemies)}\n"
            f"#define CHECKPOINT_COUNT {len(checkpoints)}\n#define GOAL_X {goal}\n\n"
            "/** @brief Each shard: x, y of its 16x16 sprite */\n" + table("shard_pos", "s16", shards) +
            "/** @brief Each machine: type (0 insect, 1 drone), x, y of its 16x16 box, patrol x min, x max */\n" +
            table("enemy_def", "s16", enemies) +
            "/** @brief Where Kit comes back after a fall: each roof's start, x and y of his box */\n" +
            table("checkpoint_pos", "s16", checkpoints))
    return text, goal


def build_likemario() -> dict[str, bytes]:
    pal = palette()
    pieces = {n: load_piece(n, pal) for n in range(6)}
    text, goal = likemario_objects(LEVEL)
    beacon = to_indices(Image.open(ART / "props" / "beacon.png").convert("RGBA"), pal)
    width_px, tiles, entries, solid = compose(LEVEL, pieces, [(beacon, goal, buildings(LEVEL)[-1][2] - 64)])
    flat = [v for c in pal for v in c]
    sheet = tile_sheet(tiles, pal)

    m16 = struct.pack("<3H", width_px, ROWS * 8, len(entries) * 2) + struct.pack(f"<{len(entries)}H", *entries)
    b16 = struct.pack(f"<{len(tiles)}H", *(0xFF00 if s else 0 for s in solid))

    # BG2: the far towers, rows 20-83 of city_far, at rows 40-103 of a 512x256 picture
    far = Image.open(ART / "backgrounds" / "city_far.png").convert("RGBA")
    band = far.crop((0, 20, 256, 84))
    half = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    half.paste(band, (0, 40))
    full = Image.new("RGBA", (512, 256), (0, 0, 0, 0))
    full.paste(half, (0, 0))
    full.paste(ImageOps.mirror(half), (256, 0))
    sky = Image.new("P", (512, 256), 0)
    sky.putpalette(flat + [0] * (768 - len(flat)))
    for yy, row in enumerate(to_indices(full, pal)):
        for xx, v in enumerate(row):
            if v:
                sky.putpixel((xx, yy), v)

    # the machines: the insect's walk, the drone's hover, then each one stomped
    walk = ART / "characters" / "walk"
    insect = [Image.open(walk / f"insect_{i}.png").convert("RGBA") for i in range(4)]
    drone = [Image.open(walk / f"drone_{i}.png").convert("RGBA") for i in range(4)]
    enemies = strip_sheet(insect + drone + [squash(insect[0]), squash(drone[0])], "MACHINE")

    # the shard turning, eight 16x16 frames in a row, its own colours
    shard = Image.open(ART / "props" / "shard.png").convert("RGBA")
    colours = sorted({p[:3] for p in shard.getdata() if p[3] >= 128})
    shards = Image.new("P", (128, 16), 0)
    shards.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    for k, fr in enumerate(spin(shard)):
        for y in range(16):
            for x in range(16):
                q = fr.getpixel((x, y))
                if q[3] >= 128:
                    shards.putpixel((k * 16 + x, y), colours.index(q[:3]) + 1)

    return {"tiles.png": png(sheet), "BG1.m16": m16, "map_1_1.b16": b16, "skyline.png": png(sky),
            "kit.png": (ART / "characters" / "kit_side_sheet.png").read_bytes(),
            "enemies.png": png(enemies), "shard.png": png(shards), "level_objects.h": text.encode(),
            "_report": f"{width_px} px wide, {len(tiles)} tiles, {sum(solid)} solid".encode()}


def font8() -> dict[str, list[int]]:
    """The library's 8x8 font (lib/source/text.asm, 2bpp): each glyph's eight rows of plane 0."""
    import re
    src = (ROOT / "lib" / "source" / "text.asm").read_text()
    body = src[src.index("opensnes_font_2bpp:"):]
    glyphs: dict[str, list[int]] = {}
    for m in re.finditer(r"; (\d+): .*\n\s*\.db ([^\n]+)", body):
        code = int(m.group(1))
        b = [int(v.strip().lstrip("$"), 16) for v in m.group(2).split(",")]
        if len(b) == 16:
            glyphs[chr(code)] = b[0::2]
        if code == 126:
            break
    return glyphs


def layer4(im: Image.Image, colours: list[str], mid: int = 55, bright: int = 90) -> Image.Image:
    """RGBA -> 2bpp indexed: 0 where transparent, else 1..3 by brightness rank: the darkest
    `mid` per cent of the opaque pixels, the next ones up to `bright` per cent, the rest."""
    pal = [rgb(c) for c in colours]
    w, h = im.size
    lum = lambda p: 0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2]
    opaque = sorted(lum(p) for p in getattr(im, "get_flattened_data", im.getdata)() if p[3] >= 128)
    cut1 = opaque[len(opaque) * mid // 100] if opaque else 0
    cut2 = opaque[len(opaque) * bright // 100] if opaque else 0
    out = Image.new("P", (w, h), 0)
    flat = [16, 24, 40] + [v for c in pal for v in c]
    out.putpalette(flat + [0] * (768 - len(flat)))
    px = im.load()
    for y in range(h):
        for x in range(w):
            p = px[x, y]
            if p[3] < 128:
                continue
            l = lum(p)
            out.putpixel((x, y), 1 if l <= cut1 else 2 if l <= cut2 else 3)
    return out


def wrap256(half: Image.Image) -> Image.Image:
    """A 128-px wide picture and its mirror: 256 px that wrap without a seam."""
    full = Image.new("RGBA", (256, half.height), (0, 0, 0, 0))
    full.paste(half, (0, 0))
    full.paste(ImageOps.mirror(half), (128, 0))
    return full


def build_mode0() -> dict[str, bytes]:
    far = Image.open(ART / "backgrounds" / "city_far.png").convert("RGBA")
    mid = Image.open(ART / "backgrounds" / "skyline_mid.png").convert("RGBA")
    out = {}
    # BG3, the far towers: rows 0-111 of city_far, at the top of the screen
    l3 = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    l3.paste(wrap256(far.crop((0, 0, 128, 112))), (0, 8))
    out["bg3.png"] = png(layer4(l3, ["#183840", "#307078", "#f0b050"], 55, 95))
    # BG2, the mid-distance blocks: rows 128-223 of skyline_mid (its sky is transparent),
    # low enough to leave the far towers in sight; amber for the lit windows only
    l2 = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    l2.alpha_composite(wrap256(mid.crop((0, 128, 128, 224))), (0, 80))
    out["bg2.png"] = png(layer4(l2, ["#281838", "#402858", "#f0b050"], 60, 96))
    # BG1, the roofs: roof_0's left half, its strip's top on screen row 136
    roof = Image.open(ART / "tiles" / "roof_0.png").convert("RGBA").crop((0, 0, 128, 119))
    l1 = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    l1.alpha_composite(wrap256(roof), (0, 136 - 62))
    out["bg1.png"] = png(layer4(l1, ["#080810", "#205058", "#48a0a0"]))
    # BG0, the status panel: rows 192-223, ink with a teal frame, amber text
    glyphs = font8()
    hud = Image.new("P", (256, 256), 0)
    flat = [16, 24, 40] + [v for c in ("#080810", "#48a0a0", "#f0b050") for v in rgb(c)]
    hud.putpalette(flat + [0] * (768 - len(flat)))
    for y in range(192, 224):
        for x in range(0, 256):
            edge = y in (193, 222) or x in (1, 254)
            hud.putpixel((x, y), 2 if edge and 1 <= x <= 254 and 193 <= y <= 222 else 1)
    def text(x, y, s):
        for k, ch in enumerate(s):
            rows = glyphs.get(ch, glyphs[" "])
            for r, bits in enumerate(rows):
                for c in range(8):
                    if bits & (0x80 >> c):
                        hud.putpixel((x + k * 8 + c, y + r), 3)
    text(16, 200, "KIT")
    text(16, 210, "COURIER")
    text(104, 200, "SHARDS  0012")
    text(104, 210, "DISTRICT  ROOFS")
    out["bg0.png"] = png(hud)
    return out


# maps/map_scroll: one long row of roofs at one height, no gap (Kit walks along
# it at a fixed height: the example is about the map, not about jumping)
FLAT = [((0, 1, 2, 3, 4, 5, 1, 0, 2, 4, 3, 5, 0, 2, 1, 3, 4, 5), 22, 0)]


def build_map_scroll() -> dict[str, bytes]:
    pal = palette()
    pieces = {n: load_piece(n, pal) for n in range(6)}
    width_px, tiles, entries, solid = compose(FLAT, pieces)
    cols = width_px // 8
    tmj = {
        "compressionlevel": -1, "height": ROWS, "infinite": False, "nextlayerid": 3, "nextobjectid": 1,
        "orientation": "orthogonal", "renderorder": "right-down", "tiledversion": "1.10.2",
        "tileheight": 8, "tilewidth": 8, "type": "map", "version": "1.10", "width": cols,
        "layers": [
            {"data": [e + 1 for e in entries], "height": ROWS, "id": 1, "name": "BG1", "opacity": 1,
             "type": "tilelayer", "visible": True, "width": cols, "x": 0, "y": 0},
            {"color": "#ff0000", "draworder": "topdown", "id": 2, "name": "Entities", "objects": [],
             "opacity": 1, "type": "objectgroup", "visible": True, "x": 0, "y": 0},
        ],
        "tilesets": [{
            "columns": 16, "firstgid": 1, "image": "roofs.png", "imageheight": (len(tiles) + 15) // 16 * 8,
            "imagewidth": 128, "margin": 0, "name": "roofs", "spacing": 0, "tilecount": len(tiles),
            "tileheight": 8, "tilewidth": 8,
            "tiles": [{"id": k, "properties": [
                {"name": "attribute", "type": "string", "value": "FF00" if solid[k] else "0"},
                {"name": "palette", "type": "string", "value": "0"},
                {"name": "priority", "type": "string", "value": "0"}]} for k in range(len(tiles))],
        }],
    }
    return {"roofs.png": png(tile_sheet(tiles, pal)),
            "rooftops.tmj": (json.dumps(tmj, indent=1) + "\n").encode(),
            "kit.png": (ART / "characters" / "kit_side_sheet.png").read_bytes(),
            "_report": f"{width_px} px wide, {len(tiles)} tiles".encode()}


def build_continuous_scroll(skyline: bytes) -> dict[str, bytes]:
    """scrolling/continuous_scroll: one stretch of roofs 512 px wide, the far towers, Kit idle and running."""
    pal = palette()
    pieces = {n: load_piece(n, pal) for n in range(6)}
    width_px, tiles, entries, _ = compose([((0, 3, 1, 4), 18, 0)], pieces)
    cols = width_px // 8
    roofs = Image.new("P", (width_px, 256), 0)
    roofs.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    for cy in range(32):
        for cx in range(cols):
            t = tiles[entries[min(cy, ROWS - 1) * cols + cx]]       # the walls go on down past row 29
            for k, v in enumerate(t):
                roofs.putpixel((cx * 8 + k % 8, cy * 8 + k // 8), v)
    # Kit: idle (frames 0-3) then run (12-19) of the side sheet, 32x32, four a row
    sheet = Image.open(ART / "characters" / "kit_side_sheet.png")
    kit = Image.new("P", (128, 96), 0)
    kit.putpalette(sheet.getpalette())
    for row, src in enumerate((0, 3, 4)):
        drop = 4 if src else 0                      # the run frames' feet are 4 px higher than the idle's
        kit.paste(sheet.crop((0, src * 32, 128, src * 32 + 32 - drop)), (0, row * 32 + drop))
    return {"roofs.png": png(roofs), "skyline.png": skyline, "kit.png": png(kit),
            "_report": f"{width_px} px wide, {len(tiles)} tiles".encode()}


# Kit at 8x8, drawn here: ears, eyes, muzzle, the jacket, the tail on the right
KIT8 = [".5....5.", ".557755.", ".505505.", "..5885..", "..6446.5", ".2666625", "..2..2..", "..0..0.."]


def sprite_sheet(im: Image.Image) -> Image.Image:
    """An RGBA sprite as an indexed PNG: 0 transparent, then its colours by brightness."""
    colours = sorted({p[:3] for p in im.getdata() if p[3] >= 128}, key=sum)
    assert len(colours) <= 15, len(colours)
    out = Image.new("P", im.size, 0)
    out.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))
    out.putdata([colours.index(p[:3]) + 1 if p[3] >= 128 else 0 for p in im.getdata()])
    return out


def build_sprite_sizes() -> dict[str, bytes]:
    """sprites/sprite_sizes: Kit at 8, 16, 32 and 64 px, one picture (and palette) each."""
    kit32 = Image.open(ART / "characters" / "kit.png").convert("RGBA")
    colours = sorted({p[:3] for p in kit32.getdata() if p[3] >= 128}, key=sum)   # the digits of KIT8
    kit8 = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    for y, row in enumerate(KIT8):
        for x, ch in enumerate(row):
            if ch != ".":
                kit8.putpixel((x, y), colours[int(ch)] + (255,))
    return {"sprite8.png": png(sprite_sheet(kit8)),
            "sprite16.png": png(sprite_sheet(Image.open(ART / "characters" / "kit_16.png").convert("RGBA"))),
            "sprite32.png": png(sprite_sheet(kit32)),
            "sprite64.png": png(sprite_sheet(Image.open(ART / "characters" / "kit_64.png").convert("RGBA")))}


def build_aseprite_pipeline() -> dict[str, bytes]:
    """sprites/aseprite_pipeline: Kit 32x48, eight frames (walk 0-3, wave 4-7), and the
    sheet's data as Aseprite exports it (`--sheet --data --list-tags`, JSON array)."""
    sheet = Image.open(ART / "characters" / "kit_tall_sheet.png").convert("RGBA")
    durations = [140, 140, 140, 140, 120, 120, 120, 200]
    frames = [{"filename": f"kit {i}.aseprite", "frame": {"x": i * 32, "y": 0, "w": 32, "h": 48},
               "duration": d} for i, d in enumerate(durations)]
    data = {"frames": frames,
            "meta": {"app": "http://www.aseprite.org/", "version": "1.3.7", "image": "hero.png",
                     "format": "RGBA8888", "size": {"w": 256, "h": 48}, "scale": "1",
                     "frameTags": [{"name": "walk", "from": 0, "to": 3, "direction": "forward", "repeat": "0"},
                                   {"name": "wave", "from": 4, "to": 7, "direction": "pingpong", "repeat": "0"}]}}
    return {"hero.png": png(sprite_sheet(sheet)), "hero.json": (json.dumps(data, indent=1) + "\n").encode()}


# scrolling/mixed_scroll: the night behind the title, rain falling down-left
# (two pixels down for one left) and far lights out of focus
RAIN = ["#000000", "#0c1424", "#121c30", "#1a2840", "#24385a", "#305070", "#48a0a0", "#80d8e0",
        "#602850", "#a02860", "#e05090", "#806020", "#f0b050"]


def rain() -> Image.Image:
    """A 256x256 picture of a 128x128 motif repeated, so the layer wraps seamlessly."""
    rng = random.Random(7)
    m = 128
    a = [[1] * m for _ in range(m)]
    for y in range(m):
        for x in range(m):
            if (x + y) % 64 < 6:
                a[y][x] = 2                         # faint diagonal bands of the night
    for _ in range(16):                             # the far lights: a dim rim, a body, a bright core
        cx, cy, r = rng.randrange(m), rng.randrange(m), rng.choice((2, 3, 4, 5))
        hue = rng.choice(((3, 4, 5), (8, 9, 10), (11, 11, 12), (4, 5, 6)))
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                d = dx * dx + dy * dy
                if d <= r * r:
                    c = hue[0] if d > r * r // 2 else hue[1]
                    if d == 0 and r > 1:
                        c = hue[2]
                    y, x = (cy + dy) % m, (cx + dx) % m
                    a[y][x] = max(a[y][x], c) if c < 6 else c
    for _ in range(34):                             # the rain: short streaks, bright at their head
        x, y, n = rng.randrange(m), rng.randrange(m), rng.choice((4, 5, 6))
        for k in range(n):
            a[(y + 2 * k) % m][(x - k) % m] = 7 if k == 0 else 5
            a[(y + 2 * k + 1) % m][(x - k) % m] = 5 if k == 0 else 4
    im = Image.new("P", (256, 256), 0)
    im.putpalette([v for c in RAIN for v in rgb(c)] + [0] * (768 - 3 * len(RAIN)))
    im.putdata([a[y % m][x % m] for y in range(256) for x in range(256)])
    return im


# windows/window_multi_hdma: the city seen from an aerial tram at dusk, behind
# its portholes — 256x256 so that the D-pad's scroll wraps both ways
TRAM_SKY = ((24, 16, 56), (112, 40, 104), (240, 120, 96))   # the top, the middle, the glow on the horizon
TRAM_HORIZON = 168
TRAM_WALL = (16, 24, 32)                                       # colour 0: the tram's wall, around the portholes


def tram_view() -> Image.Image:
    """The view, indexed: colour 0 is the wall (the backdrop, never in the picture), then its colours."""
    from mode7 import half_size

    def lerp(a, b, t):
        return tuple(int(round(x + (y - x) * t)) for x, y in zip(a, b))

    def snap(c):
        return tuple(v & 0xF8 for v in c)

    towers = half_size(Image.open(ART / "backgrounds" / "skyline_mid.png").convert("RGBA"))   # 256x112
    top, mid, glow = TRAM_SKY
    rgbim = Image.new("RGB", (256, 256))
    for y in range(256):
        if y < TRAM_HORIZON:                        # one colour a row: a row of tiles is one tile
            t = y / TRAM_HORIZON
            c = lerp(top, mid, t / 0.6) if t < 0.6 else lerp(mid, glow, (t - 0.6) / 0.4)
        else:
            c = lerp((40, 16, 48), (8, 8, 24), (y - TRAM_HORIZON) / (256 - TRAM_HORIZON))
        for x in range(256):
            rgbim.putpixel((x, y), snap(c))
    y0 = TRAM_HORIZON - towers.height
    for y in range(towers.height):
        for x in range(256):
            p = towers.getpixel((x, y))
            if p[3] < 128:
                continue
            rgbim.putpixel((x, y0 + y), snap(p[:3]))
            ry = TRAM_HORIZON + (towers.height - 1 - y) // 2   # the reflection in the canal, broken by ripples
            if ry < 256 and (x + ry) % 3:
                q = rgbim.getpixel((x, ry))
                r = tuple((a * 2 + b) // 3 for a, b in zip(snap(p[:3]), q))
                rgbim.putpixel((x, ry), snap(tuple(v * 3 // 5 for v in r)))
    colours = [TRAM_WALL] + sorted(set(rgbim.getdata()) - {TRAM_WALL}, key=sum)
    assert len(colours) <= 256, len(colours)
    return mode7_indexed(rgbim, colours)


def alley_rain() -> Image.Image:
    """windows/window's BG2: rain on nothing (colour 0 transparent), 256x256 of a 128x128 motif."""
    rng = random.Random(15)
    m = 128
    a = [[0] * m for _ in range(m)]
    for _ in range(60):                             # streaks falling a pixel left for four down
        x, y, n = rng.randrange(m), rng.randrange(m), rng.choice((5, 6, 8))
        for k in range(n):
            a[(y + k) % m][(x - k // 4) % m] = 3 if k == n - 1 else (2 if k > n // 2 else 1)
    im = Image.new("P", (256, 256), 0)
    im.putpalette([0, 0, 0, 40, 72, 96, 88, 136, 168, 176, 224, 240] + [0] * 756)
    im.putdata([a[y % m][x % m] for y in range(256) for x in range(256)])
    return im


def build_window() -> dict[str, bytes]:
    """windows/window: the alley (BG1), its rain (BG2), Kit holding his torch."""
    alley = Image.open(ART / "backgrounds" / "alley.png").convert("RGB")
    colours = sorted(set(alley.getdata()), key=sum)
    assert len(colours) <= 16, len(colours)
    return {"alley.png": png(mode7_indexed(alley, colours)), "rain.png": png(alley_rain()),
            "kit.png": png(sprite_sheet(Image.open(ART / "characters" / "kit_torch.png").convert("RGBA")))}


def on_palette(im: Image.Image, colours: list[tuple]) -> Image.Image:
    """An RGBA sprite indexed on a given palette: 0 transparent, colour i at i + 1 (each pixel the nearest)."""
    out = Image.new("P", im.size, 0)
    out.putpalette([0, 0, 0] + [v for c in colours for v in c] + [0] * (765 - 3 * len(colours)))

    def near(c):
        return min(range(len(colours)), key=lambda k: sum((a - b) ** 2 for a, b in zip(c, colours[k])))
    out.putdata([near(p[:3]) + 1 if p[3] >= 128 else 0 for p in im.getdata()])
    return out


def build_dynamic_sprite() -> dict[str, bytes]:
    """sprites/dynamic_sprite: the data shard turning in 24 frames (8 a row), and Kit at
    16x16, 32x48 and 64x64 on one palette (kit.png's), for the metasprite scene."""
    shard = Image.open(ART / "props" / "shard.png").convert("RGBA")
    colours = sorted({p[:3] for p in shard.getdata() if p[3] >= 128})
    grid = Image.new("RGBA", (128, 48), (0, 0, 0, 0))
    for k, fr in enumerate(spin(shard, 24)):
        grid.paste(fr, ((k % 8) * 16, (k // 8) * 16))
    kit_colours = sorted({p[:3] for p in Image.open(ART / "characters" / "kit.png").convert("RGBA").getdata()
                          if p[3] >= 128}, key=sum)
    return {"sprite16_grid.png": png(on_palette(grid, colours)),
            "spritehero8.png": png(on_palette(Image.open(ART / "characters" / "kit_16.png").convert("RGBA"), kit_colours)),
            "spritehero16.png": png(on_palette(Image.open(ART / "characters" / "kit_tall.png").convert("RGBA"), kit_colours)),
            "spritehero32.png": png(on_palette(Image.open(ART / "characters" / "kit_64.png").convert("RGBA"), kit_colours))}


def moonrise() -> Image.Image:
    """transitions/fading: backgrounds/title.png with its amber frame painted out (the top
    four rows take row 4, the bottom four row 219), indexed on its colours."""
    im = Image.open(ART / "backgrounds" / "title.png").convert("RGB")
    for y in range(4):
        for x in range(256):
            im.putpixel((x, y), im.getpixel((x, 4)))
            im.putpixel((x, 223 - y), im.getpixel((x, 219)))
    colours = sorted(set(im.getdata()), key=sum)
    assert len(colours) <= 16, len(colours)
    return mode7_indexed(im, colours)


def scope_target() -> Image.Image:
    """input/superscope's BG2 (2bpp): the security range's calibration target, a hologram of
    rings round the screen centre (128, 112) over the faint grid of the range's wall.
    Colour 0 is transparent (the backdrop), 1 the grid, 2 the rings, 3 the cross."""
    im = Image.new("P", (256, 224), 0)
    im.putpalette([8, 16, 40, 24, 56, 72, 224, 80, 144, 232, 248, 255] + [0] * 756)
    cx, cy = 128, 112
    for y in range(224):
        for x in range(256):
            v = 0
            if x % 16 == 0 or y % 16 == 0:
                v = 1                                   # the wall's panels
            r = math.hypot(x - cx, y - cy)
            for ring in (12, 32, 56, 84):
                if abs(r - ring) < 1.0 or (ring == 84 and abs(r - ring) < 1.6 and (x + y) % 2):
                    v = 2
            if (abs(x - cx) <= 0 and abs(y - cy) < 96) or (abs(y - cy) <= 0 and abs(x - cx) < 96):
                v = 3                                   # the cross
            if r < 3:
                v = 3
            im.putpixel((x, y), v)
    return im


def scope_marker() -> Image.Image:
    """The impact marker, 16x16: a cyan ring with a magenta core, colour 0 transparent."""
    cols = ["#000000", "#ffffff", "#c0fcff", "#48d0e0", "#e05090"]
    im = Image.new("P", (16, 16), 0)
    im.putpalette([v for c in cols for v in hexrgb_(c)] + [0] * (768 - 3 * len(cols)))
    for y in range(16):
        for x in range(16):
            r = math.hypot(x - 7.5, y - 7.5)
            v = 0
            if 5.2 < r < 7.2:
                v = 3 if (x + y) % 3 else 2
            elif r < 2.2:
                v = 1
            elif r < 3.6:
                v = 4
            im.putpixel((x, y), v)
    return im


def hexrgb_(h: str) -> tuple:
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


# input/mouse: the terminal's cursor, an arrow (its hot spot the top-left pixel)
CURSOR = ["m...............", "Xm..............", "XWX.............", "XWcX............",
          "XWccX...........", "XWcccX..........", "XWccccX.........", "XWcccccX........",
          "XWccccccX.......", "XWcccXXXXX......", "XWcXccX.........", "XWX.XccX........",
          "XX..XccX........", "X....XccX.......", ".....XccX.......", "......XX........"]


def mouse_cursor() -> Image.Image:
    cols = {".": 0, "X": 1, "c": 2, "W": 3, "m": 4}
    im = Image.new("P", (16, 16), 0)
    im.putpalette([0, 0, 0, 16, 24, 40, 72, 208, 224, 232, 248, 255, 224, 80, 144] + [0] * 753)
    im.putdata([cols[ch] for row in CURSOR for ch in row])
    return im


def indexed_scene(name: str) -> Image.Image:
    """A 256x224 background of the art, indexed on its own colours by brightness (at most 16)."""
    im = Image.open(ART / "backgrounds" / name).convert("RGB")
    colours = sorted(set(im.getdata()), key=sum)
    assert len(colours) <= 16, (name, len(colours))
    return mode7_indexed(im, colours)


# sprites/sprite_swarm: a neon moth from above, wings open then closed (8x8 each).
# 1 the wing, 2 its glow, 3 the body; the four OBJ palettes the example loads
# colour them.
MOTH = [["11....11", "121..121", "12133121", ".113311.", ".113311.", "12133121", "121..121", ".1....1."],
        ["...11...", "..1221..", "..1331..", "..1331..", "..1331..", "..1331..", "..1221..", "...11..."]]


def moth_sheet() -> Image.Image:
    im = Image.new("P", (16, 8), 0)
    im.putpalette([0, 0, 0, 224, 80, 144, 248, 176, 216, 24, 16, 40] + [0] * 756)
    for f, rows in enumerate(MOTH):
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch != ".":
                    im.putpixel((f * 8 + x, y), int(ch))
    return im


# chips/sa1_starfield: a moth far away, 4x3 pixels in an 8x8 tile (the sprites of a
# 128-strong cloud must stay small: 32 sprites a line); 1 the wings, 2 the body
MOTE = ["........", "........", "..1..1..", "..1221..", "...22...", "........", "........", "........"]


def mote() -> Image.Image:
    im = Image.new("P", (8, 8), 0)
    im.putpalette([0, 0, 0, 72, 208, 224, 232, 248, 255] + [0] * 759)
    im.putdata([0 if ch == "." else int(ch) for row in MOTE for ch in row])
    return im


# sprites/world_sprites: a square of the Rue basse seen from above, 512x512 (the world is
# 512x448; the 64x64 map's last 64 rows are never shown), and its residents.
PLAZA = ["#06060c", "#0c0e18", "#161c2a", "#1a2232", "#222a3c", "#2a3044", "#3a4258", "#0a1220",
         "#2a8090", "#a03070", "#e05090", "#5a2050", "#48d0e0", "#1a4858", "#3a3a48", "#5a5a6c"]
RESIDENTS = ["kit", "frog", "owl", "pangolin", "raccoon", "rat"]


def plaza() -> Image.Image:
    """Running-bond slabs (32x16, two tones), a kerb with a cyan neon strip round the world's
    edge, a pink neon ring inlaid at the centre (256, 224), manhole covers and puddles that
    hold the neon. Index 0 is the backdrop, never drawn."""
    W, H, WH = 512, 512, 448
    im = Image.new("P", (W, H), 1)
    im.putpalette([v for c in PLAZA for v in hexrgb_(c)] + [0] * (768 - 3 * len(PLAZA)))
    px = im.load()
    for y in range(H):
        for x in range(W):
            row = y // 16
            sx = (x + (16 if row % 2 else 0)) % 32
            if y % 16 == 15 or sx == 31:
                v = 1                                       # the grout
            else:
                slab = ((x + (16 if row % 2 else 0)) // 32 + row * 3) % 5
                v = 3 if slab in (1, 3) else 2
                if y % 16 == 0 or sx == 0:
                    v = 4                                   # the slab's lit edge
            px[x, y] = v
    # the kerb round the world, its neon strip on the inside
    for y in range(H):
        for x in range(W):
            d = min(x, y, W - 1 - x, WH - 1 - y) if y < WH else 0
            if y >= WH or d < 12:
                px[x, y] = 6 if (d < 12 and d % 12 == 0) else 5
                if y < WH and d < 12 and (x // 8 + y // 8) % 2 and d in (4, 5):
                    px[x, y] = 6                            # the kerb's stones
            elif d < 14:
                px[x, y] = 12 if d == 12 else 13            # the cyan strip and its glow
    cx, cy = 256, 224
    for y in range(cy - 90, cy + 91):
        for x in range(cx - 90, cx + 91):
            r = math.hypot(x - cx, y - cy)
            if 78 <= r < 81:
                px[x, y] = 10                               # the ring
            elif 76 <= r < 78 or 81 <= r < 84:
                px[x, y] = 11                               # its glow on the stone
            elif r < 6:
                px[x, y] = 10 if r < 2.5 else 11
    for gx, gy in ((88, 72), (416, 96), (120, 360), (392, 352), (256, 56)):
        for y in range(gy - 8, gy + 8):                     # a manhole cover, 16x16
            for x in range(gx - 8, gx + 8):
                r = math.hypot(x - gx + 0.5, y - gy + 0.5)
                if r < 7.5:
                    px[x, y] = 15 if r >= 6.5 else (14 if (x + y) % 3 else 1)
    for bx, by, rx, ry, glow in ((160, 120, 22, 9, 9), (360, 300, 28, 11, 8), (64, 260, 16, 7, 8),
                                 (300, 400, 20, 8, 9), (440, 200, 14, 10, 9)):
        for y in range(by - ry, by + ry + 1):               # a puddle and the neon in it
            for x in range(bx - rx, bx + rx + 1):
                e = ((x - bx) / rx) ** 2 + ((y - by) / ry) ** 2 + 0.12 * math.sin(x * 0.45)
                if e < 1.0:
                    px[x, y] = 7
                    if y in (by - 2, by + 1) and abs(x - bx) < rx * (0.6 if y < by else 0.4) and (x + y) % 4:
                        px[x, y] = glow                     # the neon of a sign, broken by ripples
                    elif y == by - 1 and abs(x - bx) < rx * 0.3:
                        px[x, y] = 12 if glow == 8 else 10
    return im


def residents() -> Image.Image:
    """The six residents from above, 32x32, four frames a row: frame 2k the resident k facing
    south (walking down), 2k + 1 facing north; resident k on OBJ palette 1 + k (colour index
    16 (1 + k) + c). The data shard, 16x16 on palette 0, is the first block of the fourth
    row (tile 192). 128 px wide: one row of the sheet is one row of OBJ tiles."""
    td = ART / "characters" / "topdown"
    im = Image.new("P", (128, 128), 0)
    pal = [(0, 0, 0)] * 112
    def put(src, pal_no, ox, oy, colours):
        for k, c in enumerate(colours):
            pal[pal_no * 16 + 1 + k] = c
        for y in range(src.height):
            for x in range(src.width):
                p = src.getpixel((x, y))
                if p[3] >= 128:
                    k = min(range(len(colours)), key=lambda j: sum((a - b) ** 2 for a, b in zip(p[:3], colours[j])))
                    im.putpixel((ox + x, oy + y), pal_no * 16 + 1 + k)
    for k, name in enumerate(RESIDENTS):
        frames = [Image.open(td / f"{name}_{d}.png").convert("RGBA") for d in ("south", "north")]
        colours = sorted({p[:3] for f in frames for p in f.getdata() if p[3] >= 128}, key=sum)
        if len(colours) > 15:                               # two looks of one resident on one palette
            q = Image.new("RGB", (64, 32))
            for i, f in enumerate(frames):
                q.paste(f.convert("RGB"), (32 * i, 0))
            colours = sorted({c for c in q.quantize(15, method=Image.Quantize.MEDIANCUT).convert("RGB").getdata()}, key=sum)
        for i, f in enumerate(frames):
            n = 2 * k + i
            put(f, 1 + k, (n % 4) * 32, (n // 4) * 32, colours)
    shard = Image.open(ART / "props" / "shard.png").convert("RGBA")
    put(shard, 0, 0, 96, sorted({p[:3] for p in shard.getdata() if p[3] >= 128}, key=sum))
    im.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    return im


# basics/aim_target: a security turret on a roof, in 16 headings, and the drones it aims at
TURRET = ["#101018", "#20242e", "#3a4050", "#5a6274", "#8890a4", "#c8d0e0", "#a02860", "#e05090",
          "#ffd0f0"]
ROOFTOP = ["#000000", "#0a0e16", "#141a26", "#3a1830"]


def turret(heading: int) -> Image.Image:
    """A 32x32 turret from above turned to `heading` (0-15, sixteenths of a turn, 0 = +x,
    4 = +y as atan2_8 counts): a round mount, a head and a barrel that turn, a pink lens
    and muzzle. Indices 1-9 of TURRET, 0 transparent."""
    a = heading * math.pi / 8
    ca, sa = math.cos(a), math.sin(a)
    im = Image.new("P", (32, 32), 0)
    for y in range(32):
        for x in range(32):
            dx, dy = x - 15.5, y - 15.5
            u, v = dx * ca + dy * sa, -dx * sa + dy * ca         # along / across the barrel
            r = math.hypot(dx, dy)
            c = 0
            if r < 11.5:
                c = 2 if r >= 10.5 else (3 if (dx + dy) < 0 else 2)  # the mount, lit from top-left
                if 7.5 <= r < 8.5:
                    c = 1
            if -6 <= u <= 5 and abs(v) <= 5.5 and not (u < -4.5 and abs(v) > 4):
                c = 4 if v < -2 else (5 if u < 2 else 4)         # the head
                if abs(v) > 4.5 or u < -5.5 or u > 4.5:
                    c = 3
                if 1 <= u <= 3.5 and -1.5 <= v <= 1.5:
                    c = 8 if abs(v) <= 0.5 else 7                # the lens
            if 5 < u <= 15 and abs(v) <= 1.6:
                c = 5 if v < 0 else 4                            # the barrel
                if u > 13.5:
                    c = 9 if abs(v) < 0.8 else 8                 # the muzzle
            im.putpixel((x, y), c)
    return im


def aim_sprites() -> Image.Image:
    """aim_target's OBJ sheet, 128 px wide: the turret's 16 headings (4 a row, OBJ palette 0),
    then the security drone (palette 1) and the same drone with its eye cyan (palette 2),
    which orbits. Tiles: turret heading h at ((h % 4) * 4 + (h / 4) * 64), drones 256, 260."""
    im = Image.new("P", (128, 160), 0)
    for h in range(16):
        im.paste(turret(h), ((h % 4) * 32, (h // 4) * 32))
    drone = Image.open(ART / "characters" / "drone.png").convert("RGBA")
    dcol = sorted({p[:3] for p in drone.getdata() if p[3] >= 128}, key=sum)
    assert len(dcol) <= 15, len(dcol)
    def cyan(c):                                                 # the orbiter: pinks to cyans
        r, g, b = c
        return (g // 2, max(r, g), max(r, b)) if r > g + 40 else c
    pal = [(0, 0, 0)] + [hexrgb_(c) for c in TURRET] + [(0, 0, 0)] * (6)
    pal += [(0, 0, 0)] + dcol + [(0, 0, 0)] * (15 - len(dcol))
    pal += [(0, 0, 0)] + [cyan(c) for c in dcol] + [(0, 0, 0)] * (15 - len(dcol))
    for bank, ox in ((1, 0), (2, 32)):
        for y in range(32):
            for x in range(32):
                p = drone.getpixel((x, y))
                if p[3] >= 128:
                    im.putpixel((ox + x, 128 + y), bank * 16 + 1 + dcol.index(p[:3]))
    im.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    return im


def rooftop() -> Image.Image:
    """aim_target's BG2 (Mode 0, 2bpp): a dark roof under the text, 256x224 — slabs, the
    turret's pad (a dim pink ring and its bolts round the centre, (128, 112)) and the
    hatch of a stairwell. Colour 0 is the backdrop; the others stay dark for the text."""
    im = Image.new("P", (256, 224), 1)
    im.putpalette([v for c in ROOFTOP for v in hexrgb_(c)] + [0] * 756)
    for y in range(224):
        for x in range(256):
            c = 1
            if x % 32 == 0 or y % 32 == 0:
                c = 2                                            # the slabs' seams
            r = math.hypot(x - 127.5, y - 111.5)
            if 24 <= r < 26 or 32 <= r < 33:
                c = 3                                            # the pad
            elif r < 24:
                c = 2 if (x // 4 + y // 4) % 2 else 1            # its tread plate
            if 26 <= r < 32 and abs(math.atan2(y - 111.5, x - 127.5) % (math.pi / 4) - math.pi / 8) < 0.06:
                c = 2                                            # the bolts
            if 200 <= x < 240 and 24 <= y < 56:
                c = 2 if (x in (200, 239) or y in (24, 55)) else (1 if (y - 24) % 6 else 2)  # a hatch
            im.putpixel((x, y), c)
    return im


# basics/collision_demo: a warehouse of the Rue basse, 16x16 cells (the example builds
# its map from its collision array), Kit walking from above and the security drones
WAREHOUSE = ["#06060c", "#141820", "#1a2028", "#222a34", "#2c3440", "#3a2410", "#6a4420", "#a06828",
             "#d09838", "#202838", "#384458", "#586a80", "#48d0e0", "#e05090", "#f0c040", "#0c0e14"]


def warehouse_cells() -> Image.Image:
    """Four 16x16 cells side by side (64x16, converted without reduction so that cell k is
    tiles 2k, 2k + 1, 8 + 2k, 9 + 2k): 0 the concrete floor, 1 a crate seen from above,
    2 a wall plate with its cyan light, 3 the floor with a drain grate."""
    im = Image.new("P", (64, 16), 0)
    im.putpalette([v for c in WAREHOUSE for v in hexrgb_(c)] + [0] * (768 - 3 * len(WAREHOUSE)))
    rnd = random.Random(2210)
    for k in range(4):
        for y in range(16):
            for x in range(16):
                if k in (0, 3):
                    c = 2 if rnd.random() < 0.8 else rnd.choice((1, 3))
                    if x == 15 or y == 15:
                        c = 1                                    # the slabs' joints
                    if k == 3 and 3 <= x <= 11 and 3 <= y <= 11:
                        c = 4 if x in (3, 11) or y in (3, 11) else (15 if x % 2 else 1)  # a drain grate
                elif k == 1:
                    edge = x in (0, 15) or y in (0, 15)
                    c = 5 if edge else (7 if (y // 3) % 2 else 6)  # boards
                    if (x - y) in (0, 1) or (x + y) in (14, 15):
                        c = 8 if not edge else 5                 # the straps, a cross
                    if x in (1, 14) and y in (1, 14):
                        c = 8
                    if 10 <= x <= 13 and 2 <= y <= 3:
                        c = 13                                   # a neon shipping tag
                else:
                    c = 10 if x % 4 else 9                       # corrugated metal
                    if y in (0, 15):
                        c = 11
                    if y in (7, 8):
                        c = 12 if y == 7 else 15                 # the light strip and its shade
                im.putpixel((16 * k + x, y), c)
    return im


def collision_sprites() -> Image.Image:
    """collision_demo's OBJ sheet, 128 px wide, 32x32 frames four a row: Kit's walk from
    above (frames 0-4 down, 5-9 up, 10-14 right, OBJ palette 0) then the drone's hover
    (15-18, palette 1). Palette 2 is the drone's with its pink lights green: the colour of
    a drone the player touches."""
    kit = Image.open(ART / "characters" / "topdown" / "kit_walk_sheet.png")
    walk = ART / "characters" / "walk"
    drone = strip_sheet([walk / f"drone_{i}.png" for i in range(4)], "MACHINE")
    im = Image.new("P", (128, 160), 0)
    kpal = kit.getpalette()[:48]
    for n in range(15):
        fr = kit.crop((0, 32 * n, 32, 32 * n + 32))
        for y in range(32):
            for x in range(32):
                v = fr.getpixel((x, y))
                if v:
                    im.putpixel(((n % 4) * 32 + x, (n // 4) * 32 + y), v)
    for n in range(4):
        fr = drone.crop((32 * n, 0, 32 * n + 32, 32))
        for y in range(32):
            for x in range(32):
                v = fr.getpixel((x, y))
                if v:
                    m = 15 + n
                    im.putpixel(((m % 4) * 32 + x, (m // 4) * 32 + y), 16 + v)
    dpal = drone.getpalette()[:48]
    greens = {"#601838": "#185830", "#a02860": "#28a050", "#e05090": "#60f090"}
    mach = ["#080810", "#101828", "#203048", "#384050", "#586878", "#8898a8", "#b0c0d0",
            "#601838", "#a02860", "#e05090", "#684018", "#b07028", "#f0b050", "#28a0b0", "#e8e8f0"]
    apal = [0, 0, 0] + [v for c in mach for v in rgb(greens.get(c, c))]
    im.putpalette(kpal + dpal + apal + [0] * (768 - 144))
    return im


# basics/game_skeleton: the play square (a piece of world_sprites' square with a kerb round
# the field), the title's logo, Kit walking from above and a data shard turning
def skeleton_field() -> Image.Image:
    """256x224 on PLAZA's colours: the paving of the square round its ring, a kerb with its
    cyan strip round the field Kit may walk (x 8-247, y 24-215: the HUD row is above)."""
    im = plaza().crop((128, 112, 384, 336))
    px = im.load()
    for y in range(224):
        for x in range(256):
            d = min(x - 0, 255 - x, y - 16, 223 - y)
            if y < 16 or d < 4:
                px[x, y] = 0 if y < 16 else (6 if d == 0 else 5)  # the HUD band; the kerb
            elif d < 6:
                px[x, y] = 12 if d == 4 else 13
    return im


def skeleton_sprites() -> Image.Image:
    """128 px wide: Kit's walk from above (32x32, four a row: 0-4 down, 5-9 up, 10-14
    right, OBJ palette 0), then the data shard turning in eight 16x16 frames on the fifth
    row (tile 256 + 2k, palette 1)."""
    kit = Image.open(ART / "characters" / "topdown" / "kit_walk_sheet.png")
    im = Image.new("P", (128, 160), 0)
    for n in range(15):
        fr = kit.crop((0, 32 * n, 32, 32 * n + 32))
        im.paste(fr, ((n % 4) * 32, (n // 4) * 32))
    shard = Image.open(ART / "props" / "shard.png").convert("RGBA")
    colours = sorted({p[:3] for p in shard.getdata() if p[3] >= 128}, key=sum)
    for k, fr in enumerate(spin(shard, 8)):
        for y in range(16):
            for x in range(16):
                p = fr.getpixel((x, y))
                if p[3] >= 128:
                    c = min(range(len(colours)), key=lambda j: sum((a - b) ** 2 for a, b in zip(p[:3], colours[j])))
                    im.putpixel((16 * k + x, 128 + y), 17 + c)
    pal = kit.getpalette()[:48] + [0, 0, 0] + [v for c in colours for v in c]
    im.putpalette(pal + [0] * (768 - len(pal)))
    return im


# basics/panel_hud: the noodle bar from above, the panel sheet (2bpp) and two residents
PANEL = ["#000000", "#0c1428", "#48d0e0", "#e05090"]
HEART = ["........", ".33..33.", "33333333", "33333333", ".333333.", "..3333..", "...33...", "........"]
HEART_EMPTY = ["........", ".22..22.", "2..22..2", "2......2", ".2....2.", "..2..2..", "...22...", "........"]


def panel_sheet() -> Image.Image:
    """BG3's sheet for the panel module, twelve 8x8 tiles in a row (96x8, converted without
    reduction, 2bpp): 0 transparent (what panelInit() stamps), 1-9 the 9-slice of a neon
    box in raster order (a cyan line one pixel in from the edge, rounded corners, a dark
    fill), 10 a heart, 11 an empty heart. Colour 1 the fill, 2 the line, 3 the heart."""
    im = Image.new("P", (96, 8), 0)
    im.putpalette([v for c in PANEL for v in hexrgb_(c)] + [0] * 756)
    for k in range(9):
        top, bottom, left, right = k < 3, k > 5, k % 3 == 0, k % 3 == 2
        for y in range(8):
            for x in range(8):
                c = 1
                if (top and y == 0) or (bottom and y == 7) or (left and x == 0) or (right and x == 7):
                    c = 0                                        # outside the line: the scene shows
                elif (top and y == 1) or (bottom and y == 6) or (left and x == 1) or (right and x == 6):
                    c = 2                                        # the line
                if (top or bottom) and (left or right):          # a rounded corner
                    cx, cy = (x if left else 7 - x), (y if top else 7 - y)
                    if cx + cy < 3:
                        c = 0
                    elif cx + cy == 3:
                        c = 2
                im.putpixel((8 * (1 + k) + x, y), c)
    for t, rows in ((10, HEART), (11, HEART_EMPTY)):
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                im.putpixel((8 * t + x, y), 1 if ch == "." else int(ch))
    return im


def panel_residents() -> Image.Image:
    """Kit (OBJ palette 0) and the raccoon behind the counter (palette 1), from above,
    facing south, 32x32 side by side."""
    td = ART / "characters" / "topdown"
    im = Image.new("P", (64, 32), 0)
    pal = [(0, 0, 0)] * 32
    for bank, name in ((0, "kit"), (1, "raccoon")):
        fr = Image.open(td / f"{name}_south.png").convert("RGBA")
        cols = sorted({p[:3] for p in fr.getdata() if p[3] >= 128}, key=sum)
        assert len(cols) <= 15, (name, len(cols))
        for k, c in enumerate(cols):
            pal[16 * bank + 1 + k] = c
        for y in range(32):
            for x in range(32):
                p = fr.getpixel((x, y))
                if p[3] >= 128:
                    im.putpixel((32 * bank + x, y), 16 * bank + 1 + cols.index(p[:3]))
    im.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    return im


# memory/save_game: the save terminal of the Rue basse (BG2 of Mode 0, 2bpp) and Kit's icon
TERMINAL = ["#000000", "#0a1a22", "#1a3038", "#48d0e0"]
TERMINAL_CARDS = ((13, 16), (19, 22))   # the slot cards, in text rows (8 px), inclusive


def terminal() -> Image.Image:
    """256x224, four colours: the terminal's bezel (2) round its glass (1, every other
    line a shade lighter: index 2, the scanlines), a cyan rule under the title row and
    the outlines of the two slot cards (3). The text module writes on BG1, over it."""
    im = Image.new("P", (256, 224), 0)
    im.putpalette([v for c in TERMINAL for v in hexrgb_(c)] + [0] * 756)
    for y in range(224):
        for x in range(256):
            bezel = x < 8 or x >= 248 or y < 4 or y >= 212
            c = 2 if bezel else (2 if y % 4 == 0 else 1)
            if not bezel and y == 22:
                c = 3                                        # the rule under the title
            for top, bottom in TERMINAL_CARDS:
                y0, y1 = top * 8 - 4, bottom * 8 + 11
                if 16 <= x <= 239 and y0 <= y <= y1:
                    if x in (16, 239) or y in (y0, y1):
                        c = 3                                # the card's outline
                    elif 20 <= x <= 55 and y0 + 4 <= y <= y0 + 39:
                        c = 1                                # the icon's well
            im.putpixel((x, y), c)
    return im


def save_icon() -> Image.Image:
    """Kit's head, 32x32 (logo/kit_icon.png), indexed on its 8 colours (OBJ palette 0)."""
    icon = Image.open(ART / "logo" / "kit_icon.png").convert("RGBA")
    cols = sorted({p[:3] for p in icon.getdata() if p[3] >= 128}, key=sum)
    return on_palette(icon, cols)


# input/controller: a test screen in a city terminal, two generic pads drawn, a test bay
PAD_ORIGINS = ((16, 32), (136, 32))     # each pad's top-left: 104x48, the shoulders on its top 4 lines
# The lamps main.c lights over a pad, (x, y) from its origin, then the lamp tile:
# 0 a face button (disc), 1 a D-pad arm (square), 2 SELECT / START (pill), 3 half a shoulder
PAD_LAMPS = {
    "B": (74, 31, 0), "Y": (65, 22, 0), "SELECT": (42, 30, 2), "START": (54, 30, 2),
    "UP": (22, 14, 1), "DOWN": (22, 30, 1), "LEFT": (14, 22, 1), "RIGHT": (30, 22, 1),
    "A": (83, 22, 0), "X": (74, 13, 0), "L": (10, 0, 3), "L2": (18, 0, 3), "R": (78, 0, 3), "R2": (86, 0, 3),
}
TEST_BAY = (16, 152, 239, 207)          # the bay the two avatars walk in, outline included
LAMP_GLOW = (["#b0f8ff", "#48d0e0"], ["#ffd0f0", "#e05090"])    # pad 1 cyan, pad 2 pink: core, halo


def lamp_shape(kind: int, x: int, y: int) -> bool:
    """Whether pixel (x, y) of an 8x8 cell belongs to the lamp or button of that kind."""
    if kind == 0:
        return math.hypot(x - 3.5, y - 3.5) < 3.8
    if kind == 1:
        return True
    if kind == 2:
        return 2 <= y <= 5 and not ((x in (0, 7)) and y in (2, 5))
    return y <= 3


def pad_screen() -> Image.Image:
    """256x224 on TERMINAL's four colours: the bezel and scanlines of save_game's terminal,
    the rule under the title, two pads (body 2, outline and buttons 3, the buttons' wells 0,
    the cells main.c lights in PAD_LAMPS) and the test bay's floor grid."""
    im = Image.new("P", (256, 224), 0)
    im.putpalette([v for c in TERMINAL for v in hexrgb_(c)] + [0] * 756)
    px = im.load()
    for y in range(224):
        for x in range(256):
            bezel = x < 8 or x >= 248 or y < 4 or y >= 212
            px[x, y] = 2 if bezel or y % 4 == 0 else 1
            if not bezel and y == 22:
                px[x, y] = 3
    for ox, oy in PAD_ORIGINS:
        for y in range(4, 48):                              # the body, a rounded slab
            for x in range(104):
                cx = min(x - 12, 0) if x < 12 else max(x - 91, 0)
                cy = min(y - 16, 0) if y < 16 else max(y - 35, 0)
                d = math.hypot(cx, cy)
                if d < 12:
                    px[ox + x, oy + y] = 3 if d >= 11 or y in (4, 47) or x in (0, 103) else 2
        for name, (lx, ly, kind) in PAD_LAMPS.items():
            if kind == 3:
                continue
            for y in range(8):
                for x in range(8):
                    if lamp_shape(kind, x, y):
                        edge = not all(lamp_shape(kind, x + dx, y + dy) if 0 <= x + dx < 8 and 0 <= y + dy < 8
                                       else kind == 1 for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                        px[ox + lx + x, oy + ly + y] = 3 if edge or kind == 3 else 0
        for y in range(8):                                  # the D-pad's hub joins the four arms
            for x in range(8):
                px[ox + 22 + x, oy + 22 + y] = 0
        for sx in (10, 78):                                 # the shoulders, an outlined bar each
            for y in range(4):
                for x in range(16):
                    px[ox + sx + x, oy + y] = 3 if y == 0 or x in (0, 15) else 2
    x0, y0, x1, y1 = TEST_BAY
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if x in (x0, x1) or y in (y0, y1):
                px[x, y] = 3
            elif (x - x0) % 16 == 0 or (y - y0) % 16 == 0:
                px[x, y] = 2                                # the floor's grid
            else:
                px[x, y] = 0
    return im


def pad_sprites() -> Image.Image:
    """128x96, OBJ_SIZE8_L32: Kit facing south, north, east, west (tiles 0, 4, 8, 12, OBJ
    palette 0), the raccoon the same (tiles 64-76, palette 1), then the four 8x8 lamps of
    lamp_shape at tiles 128-131, on palette 2 (pad 1's glow) - main.c lights pad 2's on 3."""
    td = ART / "characters" / "topdown"
    im = Image.new("P", (128, 96), 0)
    pal = [(0, 0, 0)] * 64
    for bank, name in ((0, "kit"), (1, "raccoon")):
        frames = [Image.open(td / f"{name}_{d}.png").convert("RGBA") for d in ("south", "north", "east", "west")]
        cols = sorted({p[:3] for f in frames for p in f.getdata() if p[3] >= 128}, key=sum)
        if len(cols) > 15:                                  # four facings on one palette
            q = Image.new("RGB", (128, 32))
            for i, f in enumerate(frames):
                q.paste(f.convert("RGB"), (32 * i, 0), f)
            cols = sorted({c for c in q.quantize(15, method=Image.Quantize.MEDIANCUT).convert("RGB").getdata()}, key=sum)
        for k, c in enumerate(cols):
            pal[16 * bank + 1 + k] = c
        for i, f in enumerate(frames):
            for y in range(32):
                for x in range(32):
                    p = f.getpixel((x, y))
                    if p[3] >= 128:
                        k = min(range(len(cols)), key=lambda j: sum((a - b) ** 2 for a, b in zip(p[:3], cols[j])))
                        im.putpixel((32 * i + x, 32 * bank + y), 16 * bank + 1 + k)
    for bank, (core, halo) in enumerate(LAMP_GLOW):
        pal[16 * (2 + bank) + 1] = hexrgb_(core)
        pal[16 * (2 + bank) + 2] = hexrgb_(halo)
    for kind in range(4):
        for y in range(8):
            for x in range(8):
                if lamp_shape(kind, x, y):
                    edge = not all(lamp_shape(kind, x + dx, y + dy) if 0 <= x + dx < 8 and 0 <= y + dy < 8
                                   else kind in (1, 3) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                    im.putpixel((8 * kind + x, 64 + y), 16 * 2 + (2 if edge else 1))
    im.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    return im


# text/pixel_text: the menu screen of a noodle bar's terminal, its text composed by main.c
BAR_PANEL = (40, 74, 215, 149)          # the message panel's outline: main.c's layer is x 48-207, y 80-143


def bar_terminal() -> Image.Image:
    """256x224 on TERMINAL's four colours: the bezel and scanlines of save_game's terminal,
    a bowl of noodles with its steam and chopsticks over the message panel, and the
    panel's double outline (3 outside, 2 inside) round the layer main.c composes on BG1."""
    im = Image.new("P", (256, 224), 0)
    im.putpalette([v for c in TERMINAL for v in hexrgb_(c)] + [0] * 756)
    px = im.load()
    for y in range(224):
        for x in range(256):
            bezel = x < 8 or x >= 248 or y < 4 or y >= 212
            px[x, y] = 2 if bezel or y % 4 == 0 else 1
    x0, y0, x1, y1 = BAR_PANEL
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if x in (x0, x1) or y in (y0, y1):
                px[x, y] = 3
            elif x in (x0 + 2, x1 - 2) or y in (y0 + 2, y1 - 2):
                px[x, y] = 2
            else:
                px[x, y] = 1                                # the glass, without scanlines
    for y in range(50, 63):                                 # the bowl: a half ellipse under its rim
        for x in range(106, 151):
            d = ((x - 128) / 22) ** 2 + ((y - 50) / 12) ** 2
            if d <= 1:
                px[x, y] = 3 if d > 0.8 or y == 50 else 2
    for x in range(121, 136):                               # its foot
        px[x, 63] = 3
    for sx in (120, 128, 136):                              # the steam, three wavy strands
        for y in range(30, 47):
            px[sx + round(2 * math.sin((y + sx) / 3)), y] = 2
    for i in range(17):                                     # the chopsticks, across the rim
        px[140 + i, 45 - i] = 3
        px[143 + i, 47 - i] = 3
    return im


# color/palette_cycle
CYCLE_BANK0 = ["#06060c","#101828","#182838","#203048","#281838","#402858","#604080","#8060a8"]
CYCLE_SIGN = ["#ffd0f0","#f8a0d0","#e05090","#a02860","#a02860","#a02860","#a02860","#e05090"]
CYCLE_BANK1 = ["#000000","#101828","#203048","#405068","#6878a0","#a0b0d0","#283040","#182030"]
CYCLE_FLOW = ["#b0f8ff","#80d8e0","#48a0a0","#307078","#205058","#307078","#48a0a0","#80d8e0"]
def neon_sign() -> Image.Image:
    """color/palette_cycle: a noodle bar's sign and a coolant pipe on a brick wall. Palette 0:
    the wall and the frame (0-7), the letters (8-14, one index each); palette 1: the pipe's
    metal (1-7) and its coolant (8-15, along x). The example rotates 8-15 of both."""
    font=font8()
    im=Image.new('P',(256,224),0)
    pal=[hexrgb_(c) for c in CYCLE_BANK0 + CYCLE_SIGN + CYCLE_BANK1 + CYCLE_FLOW]
    im.putpalette([v for c in pal for v in c]+[0]*(768-3*len(pal)))
    PIPE_TOP,PIPE_BOT=160,184
    for y in range(224):
        for x in range(256):
            if PIPE_TOP<=y<PIPE_BOT:
                dy=y-PIPE_TOP
                if dy in (0,23): v=16+1
                elif dy in (1,22): v=16+3
                elif dy in (2,21): v=16+5 if dy==2 else 16+2
                elif dy in (3,20): v=16+4 if dy==3 else 16+6
                else: v=24+((x//4 - (dy%2))%8) if 6<=dy<=17 else 16+7
                if x%64 in (0,1) : v=16+1 if dy in(0,23) else 16+3   # couplings
                if x%64 in (2,3,4): v=16+5 if dy<12 else 16+4
                im.putpixel((x,y),v); continue
            # brick wall
            row=y//8; off=(row%2)*8
            if y%8==7 or (x+off)%16==15: v=1
            else: v=2 if ((x+off)//16+row)%3 else 3
            if y>=PIPE_BOT+16: v=1 if y%2 else 4   # the wet floor
            im.putpixel((x,y),v)
    # the sign: a frame and seven letters, each on its own index 8+k
    word="NOODLES"; S=3; lw=8*S; gap=4
    w=len(word)*lw+(len(word)-1)*gap; x0=(256-w)//2; y0=48
    for y in range(y0-10,y0+lw+10):
        for x in range(x0-10,x0+w+10):
            e=min(x-(x0-10),(x0+w+9)-x,y-(y0-10),(y0+lw+9)-y)
            im.putpixel((x,y),7 if e==0 else (6 if e==1 else (5 if e<3 else 0)))
    for k,ch in enumerate(word):
        g=font[ch]
        for gy in range(8):
            for gx in range(8):
                if g[gy]>>(7-gx)&1:
                    for sy in range(S):
                        for sx in range(S):
                            im.putpixel((x0+k*(lw+gap)+gx*S+sx,y0+gy*S+sy),8+k)
    return im


# The object engine's slope codes (lib/include/snes/map.h)
T_SOLID, T_SLOPEU1, T_SLOPED1, T_SLOPELU2, T_SLOPELD2, T_SLOPEUU2, T_SLOPEUD2 = 0xFF00, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25
# a slope tile's surface: the first filled row at each x (0..7)
SLOPE_TOP = {
    T_SLOPEU1: lambda x: 7 - x,             # rises one row per pixel, to the right
    T_SLOPED1: lambda x: x,
    T_SLOPELU2: lambda x: 7 - x // 2,       # the lower half of a 2-wide ramp, rising
    T_SLOPEUU2: lambda x: 3 - x // 2,       # its upper half
    T_SLOPEUD2: lambda x: x // 2,           # the upper half of a 2-wide ramp, descending
    T_SLOPELD2: lambda x: 4 + x // 2,
}
STRIP = (12, 11)                            # the neon strip's two rows: #48a0a0 then #307078

# (kind, n): flat for n columns; up45 / down45 by n rows (one column a row);
# up22 / down22 by n rows (two columns a row); vent: a 2x3 block on the flat
SLOPES_LEVEL = [
    ("flat", 22), ("vent", 0), ("flat", 12), ("up22", 2), ("flat", 10), ("up45", 3), ("flat", 14),
    ("vent", 0), ("flat", 10), ("down45", 3), ("flat", 6), ("down22", 2), ("flat", 16), ("up45", 2),
    ("flat", 8), ("up22", 2), ("flat", 18), ("vent", 0), ("flat", 12), ("down22", 4), ("flat", 12),
    ("up45", 3), ("flat", 10), ("down45", 3), ("flat", 10), ("vent", 0), ("flat", 20),
]
BASE_ROW = 22


def build_mapandobjects() -> dict[str, bytes]:
    pal = palette()
    roof = load_piece(1, pal)                       # moved 2 px down: the strip's top on row 64
    wall = [[roof[64 + 16 + y][x] for x in range(64)] for y in range(32)]   # 8x4 tiles of windows and pipes
    strip = [[roof[64 + y][x] for x in range(64)] for y in range(8)]        # the strip and the wall's top
    deco = [[roof[16 + y][x] for x in range(128)] for y in range(48)]       # the equipment above the roof

    def wall_px(gx, gy):
        return wall[gy % 32][gx % 64]

    def vent_px(x, y):                              # a 16x24 vent: a teal box, its grille, ink outline
        if x in (0, 15) or y in (0, 23):
            return 1
        if y < 3:
            return 12 if x % 2 else 11              # the lit top: #48a0a0 / #307078
        return 11 if (y % 3 == 0) else 10           # the grille's slats: #307078 on #205058

    cols = 2
    for kind, n in SLOPES_LEVEL:
        cols += {"flat": n, "up45": n, "down45": n, "up22": 2 * n, "down22": 2 * n, "vent": 0}[kind]
    cols = max(cols, 40)
    cell = [[None] * cols for _ in range(ROWS)]      # (pixels, attribute) per cell
    top = [BASE_ROW] * cols                          # the surface row of each column
    shape = [None] * cols                            # the slope code where there is one
    vents = []
    c, row = 0, BASE_ROW
    plan = [("flat", 2)] + SLOPES_LEVEL
    for kind, n in plan:
        if kind == "flat":
            for _ in range(n):
                top[c] = row; c += 1
        elif kind == "vent":
            vents.append(c - 6)
        elif kind in ("up45", "down45"):
            # a rising tile sits on the row of the new height (from its bottom
            # to its top); a falling one on the row it leaves
            for _ in range(n):
                if kind == "up45":
                    row -= 1; top[c] = row; shape[c] = T_SLOPEU1
                else:
                    top[c] = row; shape[c] = T_SLOPED1; row += 1
                c += 1
        else:
            for _ in range(n):
                if kind == "up22":
                    row -= 1
                    top[c], shape[c] = row, T_SLOPELU2
                    top[c + 1], shape[c + 1] = row, T_SLOPEUU2
                else:
                    top[c], shape[c] = row, T_SLOPEUD2
                    top[c + 1], shape[c + 1] = row, T_SLOPELD2
                    row += 1
                c += 2
    for k in range(c, cols):
        top[k] = row
    width_px = cols * 8

    for cx in range(cols):
        for cy in range(ROWS):
            gx = cx * 8
            if shape[cx] is not None and cy == top[cx]:
                f = SLOPE_TOP[shape[cx]]
                px = []
                for yy in range(8):
                    for xx in range(8):
                        t = f(xx)
                        if yy < t:
                            px.append(0)
                        elif yy - t < 2:
                            px.append(STRIP[yy - t])
                        else:
                            px.append(wall_px(gx + xx, cy * 8 + yy))
                cell[cy][cx] = (bytes(px), shape[cx])
            elif cy == top[cx] and shape[cx] is None:
                cell[cy][cx] = (bytes(strip[yy][(gx + xx) % 64] for yy in range(8) for xx in range(8)), T_SOLID)
            elif cy > top[cx]:
                cell[cy][cx] = (bytes(wall_px(gx + xx, cy * 8 + yy) for yy in range(8) for xx in range(8)), T_SOLID)
            else:
                cell[cy][cx] = (bytes(64), 0)
    # equipment on some long flats (never solid): the 48 rows above the strip of roof_1
    for x0 in (40, 120, 200):
        if x0 + 16 < cols and all(shape[k] is None and top[k] == top[x0] for k in range(x0, x0 + 16)):
            for k in range(16):
                for r in range(6):
                    cy = top[x0] - 6 + r
                    px = bytes(deco[r * 8 + yy][k * 8 + xx] for yy in range(8) for xx in range(8))
                    if any(px):
                        cell[cy][x0 + k] = (px, 0)
    for v in vents:
        for k in range(2):
            for r in range(3):
                cy = top[v] - 3 + r
                cell[cy][v + k] = (bytes(vent_px(k * 8 + xx, r * 8 + yy) for yy in range(8) for xx in range(8)), T_SOLID)

    tiles: list[bytes] = [bytes(64)]
    attrs = [0]
    index = {(bytes(64), 0): 0}
    entries = []
    for cy in range(ROWS):
        for cx in range(cols):
            key = cell[cy][cx]
            if key not in index:
                index[key] = len(tiles)
                tiles.append(key[0])
                attrs.append(key[1])
            entries.append(index[key])
    if len(tiles) > 1024:
        sys.exit(f"mapandobjects: {len(tiles)} tiles")

    m16 = struct.pack("<3H", width_px, ROWS * 8, len(entries) * 2) + struct.pack(f"<{len(entries)}H", *entries)
    t16 = struct.pack(f"<{len(tiles)}H", *range(len(tiles)))
    b16 = struct.pack(f"<{len(tiles)}H", *attrs)

    def flat_run(k0, k1):
        """(x min, x max) of the flat stretch of column k0..k1 at one height, in px."""
        return k0 * 8, k1 * 8
    def surface_y(k):
        return top[k] * 8
    # objects: x, y (top of a 16x16 box), type (0 Kit, 1 insect, 2 automaton), patrol x min, x max
    flats = []
    k = 0
    while k < cols:
        if shape[k] is None:
            j = k
            while j + 1 < cols and shape[j + 1] is None and top[j + 1] == top[k]:
                j += 1
            if j - k >= 9:
                flats.append((k, j))
            k = j + 1
        else:
            k += 1
    objs = [(32, surface_y(4) - 16, 0, 0, 0)]
    for n, (k0, k1) in enumerate(flats[1:5]):
        obstacles = [v for v in vents if k0 <= v <= k1]
        lo, hi = k0 + 1, k1 - 2
        if obstacles:                                  # patrol on the side of the vent with more room
            v = obstacles[0]
            lo, hi = (k0 + 1, v - 3) if v - k0 > k1 - v else (v + 3, k1 - 2)
        x = (lo + hi) // 2 * 8
        objs.append((x, surface_y(k0) - 16, 1 if n % 2 == 0 else 2, lo * 8, hi * 8))
    o16 = b"".join(struct.pack("<5H", *o) for o in objs) + struct.pack("<H", 0xFFFF)

    walk = ART / "characters" / "walk"
    return {"tiles.png": png(tile_sheet(tiles, pal)), "BG1.m16": m16, "level.t16": t16, "level.b16": b16,
            "level.o16": o16, "kit.png": (ART / "characters" / "kit_side_sheet.png").read_bytes(),
            "skyline.png": build_likemario()["skyline.png"],
            "insect.png": png(strip_sheet([walk / f"insect_{i}.png" for i in range(4)], "MACHINE")),
            "automaton.png": png(strip_sheet([walk / f"automaton_{i}.png" for i in range(4)], "MACHINE")),
            "_report": f"{width_px} px wide, {len(tiles)} tiles, {len(objs)} objects {objs}".encode()}


def png(im: Image.Image) -> bytes:
    import io
    buf = io.BytesIO()
    im.save(buf, format="PNG", transparency=0, optimize=False)
    return buf.getvalue()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true", help="compare with the committed files instead of writing them")
    args = ap.parse_args()
    city = build_city(ART)
    race = build_track(ART)
    ground, sky = png(city["ground"]), png(city["sky"])
    plane, plane_tiles = extbg_plane(ART)
    arcade = build_breakout(font8())
    likemario = build_likemario()
    continuous = build_continuous_scroll(likemario["skyline.png"])
    jobs = [(LIKEMARIO, build_likemario()), (MODE0, build_mode0()), (MAP_SCROLL, build_map_scroll()),
            (MAPANDOBJECTS, build_mapandobjects()), (LOGO, build_logo(ART)),
            (GRADIENT_COLORS, {"logo.png": screen_logo(ART)}), (HDMA_HELPERS, {"logo.png": screen_logo(ART)}),
            (MODE7 / "perspective" / "res", {"ground.png": ground, "sky.png": sky}),
            (MODE7 / "dsp1_ground" / "res", {"ground.png": ground, "sky.png": sky}),
            (MODE7 / "perspective_rotate" / "res", {"ground.png": ground}),
            (MODE7 / "rotate_scale" / "res", {"mode7bg.png": ground, "glider.png": png(glider(ART))}),
            (MODE7 / "extbg" / "res", {"plane.png": png(plane), "orb.png": png(orb())}),
            (CONTINUOUS, continuous),
            (SPRITE_SIZES, build_sprite_sizes()),
            (ASEPRITE, build_aseprite_pipeline()),
            (WINDOW, build_window()),
            (DYNAMIC, build_dynamic_sprite()),
            (SWARM, {"moth.png": png(moth_sheet()), "alley.png": png(mode7_indexed(
                Image.open(ART / "backgrounds" / "alley.png").convert("RGB"),
                sorted(set(Image.open(ART / "backgrounds" / "alley.png").convert("RGB").getdata()), key=sum)))}),
            (PALETTE_CYCLE, {"sign.png": png(neon_sign())}),
            (WORLD, {"plaza.png": png(plaza()), "residents.png": png(residents())}),
            (AIM, {"units.png": png(aim_sprites()), "roof.png": png(rooftop())}),
            (COLLISION, {"cells.png": png(warehouse_cells()), "cast.png": png(collision_sprites())}),
            (SKELETON, {"field.png": png(skeleton_field()), "logo.png": screen_logo(ART),
                        "cast.png": png(skeleton_sprites())}),
            (SCENE_STACK, {"field.png": png(skeleton_field()), "logo.png": screen_logo(ART)}),
            (PANEL_HUD, {"noodlebar.png": png(indexed_scene("noodlebar.png")), "boxes.png": png(panel_sheet()),
                         "residents.png": png(panel_residents())}),
            (SAVE_GAME, {"terminal.png": png(terminal()), "icon.png": png(save_icon())}),
            (CONTROLLER, {"screen.png": png(pad_screen()), "cast.png": png(pad_sprites())}),
            (PIXEL_TEXT, {"terminal.png": png(bar_terminal())}),
            (SA1_FIELD, {"mote.png": png(mote()), "skyline.png": png(indexed_scene("city_far.png"))}),
            (SHADOW_TINT, {"street.png": png(indexed_scene("city_near.png"))}),
            (MOUSE, {"cursor.png": png(mouse_cursor())}),
            (SUPERSCOPE, {"aim_adjust_target.png": png(scope_target()), "marker.png": png(scope_marker())}),
            (FADING, {"moonrise.png": png(moonrise())}),
            (MULTI_HDMA, {"city.png": png(tram_view())}),
            (MIXED, {"logo.png": screen_logo(ART), "rain.png": png(rain())}),
            (BREAKOUT, {"tiles.png": png(arcade["tiles"]), "back.png": png(arcade["back"]),
                        "bg1map.bin": arcade["bg1map"], "bg2map.bin": arcade["bg2map"],
                        "backpal.bin": arcade["backpal"]}),
            (RACING, {"track.png": png(race["track"]), "track_class.bin": race["classes"],
                      "lanes_xy.bin": race["lanes"][0], "lanes_heading.bin": race["lanes"][1],
                      "bikes.png": png(bikes(ART))})]
    print("likemario:", jobs[0][1].pop("_report").decode())
    print("map_scroll:", jobs[2][1].pop("_report").decode())
    print("continuous_scroll:", continuous.pop("_report").decode())
    print("mapandobjects:", jobs[3][1].pop("_report").decode())
    print(f"mode7 city: {city['_tiles']} tiles, {city['_colours']} colours")
    print(f"mode7 extbg: {plane_tiles} tiles")
    print(f"mode7 race: {race['_tiles']} tiles, {race['_colours']} colours")
    bad = 0
    for folder, files in jobs:
        for name, data in files.items():
            path = folder / name
            if args.check:
                if not path.is_file() or path.read_bytes() != data:
                    print(f"  DIFFERS {path.relative_to(ROOT)}")
                    bad += 1
            else:
                path.write_bytes(data)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
