# tmx2snes — Tiled levels to SNES map data {#tools_tmx2snes}

`tmx2snes` is how a level you paint in [Tiled](https://www.mapeditor.org/)
becomes something the SNES map engine can load and scroll. It takes a Tiled map
exported as JSON, plus the tile-optimisation table from @ref tools_gfx4snes, and
emits the binary tilemap, collision, and object files the runtime reads. It is
the tool that makes @ref craft_tiles_to_levels a real workflow instead of a
theory.

## What goes in, what comes out

**In** — two positional arguments, in this order:

1. `map.tmj` — your Tiled map, **exported as JSON** (`.tmj`), not the XML `.tmx`.
2. `tileset.map` — the `.map` file `gfx4snes` produced for the same tileset (it
   lets tmx2snes reuse gfx4snes's tile de-duplication).

**Out** — the defaults, always emitted:

| File | Named after | What it is | Loaded with |
|------|-------------|-----------|-------------|
| `<layer>.m16` | the Tiled *layer* | the tilemap | `mapLoad` (arg 1) |
| `<base>.t16` | the map | per-tile palette + priority | `mapLoad` (arg 2) |
| `<base>.b16` | the map | per-tile attributes (collision behaviour) | `mapLoad` (arg 3) |
| `<base>.o16` | the map | object positions + types | `objLoadObjects` |

Opt-in outputs:

| Flag | Adds | For |
|------|------|-----|
| `-e` | `<map>.inc` — the Entities layer as C `#define`s | referencing objects from game code |
| `-Q` | `<layer>.q16` — a quadrant-ordered 64×64 tilemap | a `dmaCopyVram`-straight-to-VRAM map you scroll yourself |
| `-C` | `<layer>.c16` — one collision byte per cell | `collideTile` on a plain grid |

## How you actually use it

Because a Tiled project has choices the build cannot guess, tmx2snes is called
explicitly from the example's Makefile — typically from inside `res/`:

```sh
cd res && tmx2snes maplevel01.tmj tileslevel1.map
```

where the tileset `.map` came from, e.g.:

```sh
gfx4snes -s 8 -o 48 -u 16 -p -m -i tileset.png
```

Then in C you hand the three defaults to `mapLoad()` and scroll with the map
engine — see @ref tutorial_map for the loading and scrolling code.

## Authoring the level in Tiled

The data lives in Tiled tile **custom properties**, which tmx2snes reads and
bakes into the binaries — this is how a tile carries *meaning*, not just pixels
(the idea developed in @ref craft_tiles_to_levels):

| Tiled property | Type | Becomes |
|----------------|------|---------|
| `attribute` | hex string | the collision/behaviour byte (`.b16`) — solid, ladder, spike, slope… |
| `palette` | "0"–"7" | which of the 8 sub-palettes the tile uses |
| `priority` | "0" / "1" | whether the tile draws in front of sprites |

Objects (spawns, doors, triggers) go in an **objectgroup** layer named
`Entities`; each object's Tiled *type* groups it in the `.o16` / `.inc` output.

## Gotchas worth knowing up front

- **JSON, not XML.** Export the map as `.tmj` (Tiled: *File → Export As → JSON*).
  The XML `.tmx` is the editor's own format; the tool ingests the JSON export.
- **8×8 tiles only**, up to 16384 tiles, height ≤ 256.
- **Layer name = filename.** A layer called `BG1` yields `BG1.m16`. Name your
  layers deliberately.
- **`-h`, not `--help`.** The long form is not recognised (it prints usage
  anyway).
- **`-C` reflects the map only.** Runtime blockers (an NPC standing in a
  doorway) are a game-logic decision, not baked into the collision grid.

## See it in practice

- @ref examples_maps_tiled — the complete Tiled → SNES worked example.
- @ref examples_maps_map_scroll — the map engine scrolling a level wider than VRAM.
- @ref examples_games_rpg — a full game driving its world from Tiled.

Upstream of this: @ref tools_gfx4snes makes the tileset and the `.map` table
tmx2snes needs.
