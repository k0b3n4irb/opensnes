# opensnes-tileset — pictures to tilesets and tilemaps {#tools_opensnes_tileset}

The background tool of the 1.x family (@ref tools_conventions). A picture
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
| `town.pal` | the palette (`--colors` entries, 256 by default); `--no-palette` skips it |
| `town.inc`, `town_data.as` | the glue, in the lib's `asset.h` naming: `town.inc` declares `town_tiles`, `town_map`, `town_pal` (each with `_end`) and a ready `BgAsset town` (`DECLARE_BG_ASSET`, when the map is 32x32, 64x32, 32x64 or 64x64) so `bgLoad(0, &town, slot, tiles_vram, map_vram)` is the whole load; `town_data.as` is the `.incbin` fragment the build gathers into `assets_gen.asm` |

**Palette banks.** At 4 bpp a map entry names one of eight 16-colour banks,
so every opaque pixel of a tile must sit in one bank of the palette. A tile
that straddles two is refused with its position. `--rearrange` regroups the
picture's colours into banks and renumbers the tiles accordingly (the
palette then holds the eight banks, 128 colours at 4 bpp); `--palette FILE`
imposes an authored `.pal` so indices never drift when the picture changes.
`--palette-entry N` offsets the bank numbers for a layer that shares CGRAM.

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
