# gfx4snes — images to tiles, palettes & maps {#tools_gfx4snes}

`gfx4snes` is the cornerstone of the asset pipeline. It takes an indexed image
and produces the three things a background (or a sprite sheet) is made of on the
SNES: **tile graphics**, a **palette**, and — for backgrounds — a **tilemap**.
Almost every visual example in the SDK starts here.

## What goes in, what comes out

**In:** a `.png` or `.bmp` that is **256-colour *indexed*** — not RGB. (Artist
art in RGB must pass through @ref tools_img2snes first; that is the single most
common gfx4snes mistake.)

**Out**, depending on flags:

| File | What it is | Loaded with |
|------|-----------|-------------|
| `.pic` | tile graphics (2/4/8bpp bitplanes) | `dmaCopyVram` / `bgInitTileSet` |
| `.pal` | palette, BGR555, 2 bytes/colour | `dmaCopyCGram` |
| `.map` | 16-bit tilemap entries (tile + flip + palette + priority) | `dmaCopyVram` / the map engine |
| `.inc` | C size constants for the above | `#include` |
| `.pc7` / `.mp7` | Mode 7 packed tiles / tilemap | Mode 7 setup |

## The two flags that decide everything: `-u` and `-s`

Colour depth is not a `--bpp` flag — it is implied by `-u` (colours *used* per
tile), and it drives the VRAM cost you budgeted in @ref craft_planning —

| `-u` | Depth | Bytes/tile | Use for |
|------|-------|-----------|---------|
| `-u 4` | 2bpp (4 colours) | 16 | Mode 0 layers, HUD/BG3, cheap backdrops |
| `-u 16` | 4bpp (16 colours) | 32 | the workhorse — Mode 1 BG1/BG2, most sprites |
| `-u 256` | 8bpp (256 colours) | 64 | Mode 3 / Mode 7 lavish single layers |

And `-s` sets the tile/cell size: `-s 8` for backgrounds, `-s 16` (or 32/64) for
sprites and metasprites.

## The flags you will actually use

| Flag | Meaning |
|------|---------|
| `-i FILE` | input image (**required**) |
| `-s N` | cell size 8/16/32/64 (8 = BG, 16 = sprite) |
| `-u N` | colours per tile → bit depth (4 / 16 / 256) |
| `-o N` | how many colours to write to `.pal` (e.g. `-o 16`) |
| `-p` | emit the palette `.pal` |
| `-m` | emit the tilemap `.map` (backgrounds; **omit for sprites**) |
| `-e N` | palette-bank offset (0–7) baked into tilemap entries |
| `-c FILE` | impose a fixed palette — build fails if the image needs a colour not in it |
| `-t png\|bmp` | force the input type |
| `-y` | tilemap as 32×32 pages (for scrolling) · `-z` LZ77 · `-F` dedupe flipped |

## Worked examples

```sh
# A 4bpp background for Mode 1 (tiles + palette + map):
gfx4snes -s 8 -o 16 -u 16 -p -m -i background.png

# A 2bpp background for Mode 0 (from a BMP):
gfx4snes -s 8 -o 4 -u 4 -e 0 -p -m -t bmp -i tiles.bmp

# A 16×16 sprite sheet (tiles + palette, no map):
gfx4snes -s 16 -p -i sprites.png

# With a fixed, authored palette so colours never drift (recommended):
gfx4snes -s 8 -o 16 -u 16 -p -m -c res/town_fixed.pal -i tileset.png
```

In the SDK you rarely type these: set `GFXSRC := background.png` in the example
Makefile and the zero-config rule runs `gfx4snes -s $(SPRITE_SIZE) -p` for you.
Add `-m`, `-c`, and friends when you outgrow the default.

## Gotchas worth knowing up front

- **Indexed input only.** RGB/RGBA art produces garbage or an error — run
  @ref tools_img2snes first.
- **The silent re-hue.** Without `-c`, gfx4snes derives the palette from the
  *whole* image, so adding one tile can quietly change the colours of every
  existing tile (a real regression the RPG example hit). Pin the palette with
  `-c` once your colours are settled.
- **bpp is `-u`, not a bpp flag** — the most common source of confusion.
- **Sprites skip `-m`** — a sprite sheet has no tilemap.

## See it in practice

- @ref examples_backgrounds_mode0 — 2bpp, four cheap layers.
- @ref examples_backgrounds_mode3 — 8bpp, and why one screen costs ~40 KB.
- @ref examples_games_rpg — fixed-palette backgrounds *and* `-s 16` sprites.
- @ref examples_input_move_sprite — the simplest sprite-tile case.

Next in the pipeline: feed the `.map` this produces into @ref tools_tmx2snes to
turn a Tiled level into ready-to-load map binaries.
