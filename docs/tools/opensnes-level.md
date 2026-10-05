# opensnes-level — Tiled levels to SNES map data {#tools_opensnes_level}

The level tool of the 1.x family (@ref tools_conventions). A map drawn in
[Tiled](https://www.mapeditor.org/) against a tileset that `opensnes-tileset`
converted becomes what the `map` and `object` modules load. The converter is
`tmx2snes`'s (@ref tools_tmx2snes), linked as a library: the bytes are its own.

## Convert

```sh
opensnes-level convert res/town.tmj --tileset res/tiles.map --save   # the map, its tile tables, its entities
opensnes-level convert res/town.tmj --tileset res/tiles.map --entities --collision
```

**In:** a Tiled map exported as JSON (`.tmj`), 8x8 tiles, one tileset, and
the `.map` that `opensnes-tileset convert` wrote for that tileset's picture
(`--tileset`): it is the table that says which VRAM tile each tileset tile
became, and the level is written in those numbers.

**Out**, beside the input or in `--out`:

| File | What it is |
|---|---|
| `<layer>.m16` | one per tile layer, named after the layer (`BG1.m16`): a 6-byte header (width and height in pixels, size) then one word per cell — what `mapLoad()` streams |
| `town.b16` | one word per tileset tile: its `attribute` property (collision, in hex in Tiled) |
| `town.t16` | one word per tileset tile: its VRAM tile with the `palette` and `priority` properties folded in |
| `town.o16` | the `Entities` layer for `objLoadObjects()`: x, y, type, `minx`, `maxx` per object, `$FFFF`-terminated |
| `town_entities.inc` | with `--entities`: the Entities layer as C defines, grouped by type (`<TYPE>_COUNT`, `<TYPE>_FIELDS`, `<TYPE>_TABLE`, and `<TYPE>_TX` / `_TY` / `_<PROP>` for a lone object) |
| `<layer>.q16` | with `--quadrant`: a 64x64 map in the PPU's four 32x32 pages, palette and priority folded in, ready for VRAM (`bgSetScroll` games) |
| `<layer>.c16` | with `--collision`: one collision byte per cell, what `collideTile()` reads |
| `town.inc`, `town_data.as` | the glue: `town.inc` declares `town_BG1_map`, `town_tileattr`, `town_tiledef`, `town_objects` (each with `_end`; `_quad` and `_cells` when asked) so `mapLoad(town_BG1_map, town_tiledef, town_tileattr)` is the whole load; `town_data.as` is the `.incbin` fragment the build gathers into `assets_gen.asm`, one `ASSET_SECTION` per blob |

## The settings file

```toml
# res/town.tmj.toml
tool = "opensnes-level"

[convert]
tileset = "tiles.map"     # relative to this file; the .map opensnes-tileset writes beside tiles.png
entities = true
```

The build converts a level after the other assets of the project, so the
tileset's `.map` is fresh when the level reads it. Nothing else to declare:
`make/common.mk` runs the conversion and links `town_data.as`.

## Inspect

```sh
opensnes-level inspect res/*.tmj
```

Size in tiles, the tile layers, the bytes of map per layer, the tileset's
tile count and the number of entities, without writing anything (`--json`
for a script).

## Refused maps

A map is refused, with the reason, when a tile is rotated (the SNES has only
horizontal and vertical flips), when a tile id passes 1024, when the map uses
two tilesets (merge them in Tiled or split the layers), when it is larger than
16384 cells or 256 rows, when its tiles are not 8x8, when more than 64
entities are placed, or when `--quadrant` is asked of a map that is not 64x64.
`--tileset` is required for `convert`: without the tileset's table the `.t16`
cannot be written.

## See also

- @ref tools_tmx2snes — the 0.x tool, same converter, one explicit Makefile line; shipped for one more version.
- `examples/maps/map_scroll` and `examples/maps/tiled` convert their levels this way.
