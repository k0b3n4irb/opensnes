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
