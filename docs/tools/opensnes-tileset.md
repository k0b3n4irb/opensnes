# opensnes-tileset — pictures to tilesets and tilemaps {#tools_opensnes_tileset}

The background tool of the `opensnes-*` family (@ref tools_conventions). A picture
drawn at the screen's size becomes the three things a BG layer loads: the
tileset (`.pic`), the tilemap (`.map`) and the palette (`.pal`). The
converter is `gfx4snes -m`'s, linked as a library: the bytes are its own.

## Convert

```sh
opensnes-tileset convert res/town.png --colors 16            # 4 bpp, Mode 1: tiles, map, 16-colour palette
opensnes-tileset convert res/title.png --bpp 8 --mode 7      # Mode 7: packed .pc7 / .mp7
opensnes-tileset convert res/level.png --pages --flip --save # a 64x64 scrolling map in 32x32 pages
```

**In:** an indexed PNG or BMP, cut into `--size` blocks (8 by default, the
BG tile; 16 for metatiles). Duplicate blocks are stored once; `--flip` also
folds mirrored blocks, the map carrying the flip bits.

**Out**, beside the input or in `--out`:

| File | What it is |
|---|---|
| `town.pic` | the unique tiles, `--bpp` deep (2, 4 or 8); `--lz` compresses; `--pack` writes packed pixels (`--mode 7` implies it, as `.pc7`) |
| `town.map` | one 16-bit entry per block: tile number (+ `--offset`), palette bank, `--priority` bit, flips; `--pages` lays it out in 32x32 pages; Modes 5 and 6 halve the width; Mode 7 writes one byte per tile (`.mp7`) |
| `town.cmap` | only with `--column-major`: the entries of `town.map` column after column (see below) |
| `town.pal` | the palette (`--colors` entries, 256 by default); `--no-palette` skips it |
| `town.inc`, `town_data.as` | the glue, in the lib's `asset.h` naming: `town.inc` declares `town_tiles`, `town_map`, `town_pal` (each with `_end`) and a ready `BgAsset town` (`DECLARE_BG_ASSET`, when the map is 32x32, 64x32, 32x64 or 64x64) so `bgLoad(0, &town, slot, tiles_vram, map_vram)` is the whole load; `town_data.as` is the `.incbin` fragment the build gathers into `assets_gen.asm` |

**A map larger than the screen, scrolled in both axes.** Such a game keeps
a window of the map in VRAM and feeds it a row and a column at a time. A row
of `town.map` is one contiguous block; a column is not — its entries are a
whole row apart. `--column-major` (`column-major = true` in the settings
file) also writes `town.cmap`, the same entries column after column, each
column from top to bottom, and declares `town_cols[]` / `town_cols_end[]`
in `town.inc`: the entry of column `c`, row `r` is at byte
`(c * height + r) * 2`, `height` being the map's height in entries. A
column is then one block, pushed to VRAM with a vertical increment
(`vramQueuePush(town_cols + (c * height + r) * 2, addr, n * 2,
VRAM_QUEUE_COLUMN)`), straight from ROM: no copy in RAM, nothing built at
boot. It costs the map a second time in ROM, in the asset banks. Refused
where there is no column order to write: Mode 7 (one byte per entry),
`--pages`, and a picture of exactly 64x32, 32x64 or 64x64 entries, which
is written in 32x32 screens as the PPU reads it. Asked by the first game
that scrolled a 80x144 map on the SDK: it was building that table in RAM
at boot, 23 KB and 38 frames.

**Palette banks.** At 4 bpp a map entry names one of eight 16-colour banks,
so every opaque pixel of a tile must sit in one bank of the palette. A tile
that straddles two is refused with its position. `--rearrange` regroups the
picture's colours into banks and renumbers the tiles accordingly (the
palette then holds the eight banks, 128 colours at 4 bpp); `--palette FILE`
imposes an authored `.pal` so indices never drift when the picture changes.
`--palette-entry N` offsets the bank numbers for a layer that shares CGRAM.

**Colour 0 of a bank is never drawn.** On a background a tile pixel of 0 is
transparent, whatever the palette holds at that index
([SNESdev wiki, Palettes](https://snes.nesdev.org/wiki/Palettes): "a tile
pixel of 0 is always transparent"): what shows there is the layer behind,
or the backdrop. A picture that comes from a machine without that rule (an
Amiga bitmap, say) and uses index 0 as its black will have holes; a cell
that must be opaque black needs another index set to black. Asked by the
first game built on the SDK, for a status bar that had to hide what scrolls
under it.

The hardware limits are refused, not masked: a tilemap addresses 1024 tiles
(the 10-bit field), a Mode 7 map 256 (one byte per tile).

## Inspect

```sh
opensnes-tileset inspect res/*.png
# town.png: 256x256 px, 1024 blocks of 8 (up to 1024 tiles before deduplication, 32768 bytes of VRAM at 4 bpp, map 2048 bytes), 14 colours used, highest index 13 (bank 0)
```

The upper bound before deduplication, the map's size, and whether the
picture already lives in one palette bank.

## Settings beside the asset

`--save` writes `town.png.toml` (`[convert]`: colors, mode, flip, pages…);
every later run reads it, the command line overrides for one run.

## From gfx4snes

| gfx4snes | opensnes-tileset |
|---|---|
| `gfx4snes -s 8 -o 16 -u 16 -p -m -i town.png` | `convert town.png --colors 16` |
| `-u 4` / `-u 256` | `--bpp 2` / `--bpp 8` |
| `-M 7` (and the `.pc7`/`.mp7` pair) | `--mode 7` |
| `-a`, `-c fixed.pal`, `-e N` | `--rearrange`, `--palette fixed.pal`, `--palette-entry N` |
| `-f N`, `-g`, `-y`, `-R`, `-F` | `--offset N`, `--priority`, `--pages`, `--no-reduce`, `--flip` |
| `-b`, `-z`, `-k`, `-d` | `--blank`, `--lz`, `--pack`, `--round` |

Sprites and metasprites are @ref tools_opensnes_sprite; Tiled maps with
objects and collision are `opensnes-level`'s, next in the family.
