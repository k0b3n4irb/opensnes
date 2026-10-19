# Scrolling maps {#tutorial_map}

This tutorial covers the `map` module — the scrolling tilemap engine that
streams a world larger than VRAM, and the Tiled content pipeline that feeds
it. Add `map` to `LIB_MODULES`.

## What the map module is

A SNES background tilemap is a fixed 32×32 (or 64×64) grid in VRAM. A game
world is bigger than that. The `map` module holds the *whole* map in ROM
and streams the strip of tiles the camera is about to reveal into VRAM one
column/row at a time, during VBlank — so scrolling costs a few tiles per
frame instead of a full-screen upload.

It works in **metatiles**: 8×8 blocks indexed through a small tile
definition, which is how a large map fits in ROM. You give it three things
at load time and then just move the camera each frame.

## The Tiled pipeline

You do not hand-author the binary formats. Draw the map in
[Tiled](https://www.mapeditor.org/) against a tileset picture, export JSON
(`.tmj`), and put two settings files in `res/`: one converts the tileset
picture with `opensnes-tileset`, the other the level with
@ref tools_opensnes_level, against the `.map` the first one writes
(`examples/maps/map_scroll`):

```toml
# res/roofs.png.toml
tool = "opensnes-tileset"

[convert]
colors = 16
```

```toml
# res/rooftops.tmj.toml
tool = "opensnes-level"

[convert]
tileset = "roofs.map"
```

The build converts the level after the pictures, so the tileset's `.map` is
fresh when the level reads it; the Makefile names neither. By hand it is
`opensnes-level convert res/rooftops.tmj --tileset res/roofs.map`.
It emits four files the modules consume:

| File | Content | Consumed by |
|------|---------|-------------|
| `<layer>.m16` | the streaming tilemap (tile indices + flip bits) | `mapLoad()` arg 1 |
| `<map>.t16` | per-tile palette + priority | `mapLoad()` arg 2 (tileset def) |
| `<map>.b16` | per-tile collision attribute | `mapLoad()` arg 3 |
| `<map>.o16` | the Entities object layer | the `object` module |

and `<map>.inc`, which declares them for C: `rooftops_BG1_map`,
`rooftops_tiledef`, `rooftops_tileattr`, `rooftops_objects` (each
with `_end`).

Three more keys of `[convert]` serve games that do **not** use the `map`
module and want the same content in a different shape:

| Key | Output | For |
|-----|--------|-----|
| `entities = true` | `<map>_entities.inc` — entities as C defines (struct + rows, custom properties) | a game that keeps its own entity structs |
| `quadrant = true` | `<layer>.q16` — a quadrant-ordered 64×64 tilemap | scrolling a fixed 64×64 area with `bgSetScroll` (the `background` module, not `map`) |
| `collision = true` | `<layer>.c16` — one collision byte per map cell | `collideTile()` (a flattened grid, indexed directly) |

`examples/games/rpg` scrolls a fixed 64×64 town with `background`, not
`map` (its `town.tmj` gives the same quadrant map under `--quadrant`);
`examples/maps/map_scroll` uses the default `map` pipeline. Pick the module that
matches the map size — `map` for worlds larger than 64×64, `background` for
a fixed screen or two.

> **cute_tiled needs no special export.** `opensnes-level` minifies the JSON
> itself, so a pretty-printed `.tmj` from a generator script parses fine.

## The minimum viable scroller

```c
#include <snes.h>
#include <snes/map.h>

#include "res/tileslevel1.inc"   // the tileset: tileslevel1_tiles[], tileslevel1_pal[]
#include "res/maplevel01.inc"    // the level: the .m16/.t16/.b16

int main(void) {
    consoleInit();

    // The tileset and the map's VRAM home; the map engine streams to $6800.
    bgInitTileSet(0, tileslevel1_tiles, tileslevel1_pal, 0,
                  tileslevel1_tiles_end - tileslevel1_tiles,
                  tileslevel1_pal_end - tileslevel1_pal,
                  BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_64x32);
    setMode(BG_MODE1, 0);

    // mapLoad flushes the first screen of tilemap to VRAM.
    // It must run BEFORE setScreenOn().
    mapLoad(maplevel01_BG1_map, maplevel01_tiledef, maplevel01_tileattr);

    setScreenOn();

    u16 cam_x = 0, cam_y = 0;
    while (1) {
        u16 keys = padHeld(0);
        if (keys & KEY_RIGHT) cam_x += 1;
        if (keys & KEY_LEFT)  cam_x -= 1;
        if (keys & KEY_DOWN)  cam_y += 1;
        if (keys & KEY_UP)    cam_y -= 1;

        mapUpdateCamera(cam_x, cam_y);   // where the camera wants to be
        mapUpdate();                     // stage the reveal strip
        WaitForVBlank();
        mapVblank();                     // DMA the staged strip to VRAM
    }
}
```

The three-call rhythm each frame is the whole contract:

- **`mapUpdateCamera(x, y)`** — set the camera's world position. The module
  gives the live scroll through `mapGetCameraX()` / `mapGetCameraY()`.
- **`mapUpdate()`** — compute which column/row just came into view and stage
  it. Cheap; runs in the active frame.
- **`mapVblank()`** — do the actual VRAM DMA of the staged strip. Must run
  in VBlank (call it right after `WaitForVBlank()`).

## Collision against the map

The map already knows which tiles are solid (the `.b16` attributes). Query
it by world pixel:

```c
if (mapGetMetaTilesProp(player_x, player_y) == 0) {
    // walkable — the tile's attribute is 0
}
u16 tile = mapGetMetaTile(player_x, player_y);   // the raw metatile id
```

`mapGetMetaTilesProp` is the map-engine equivalent of `collideTile()`; use
it when the map lives in the `map` module. (A game on the `background`
module with a `.c16` grid (`collision = true`) uses `collideTile()` instead — see the
[collision tutorial](collision.md).)

## One-way and second-layer options

```c
mapSetMapOptions(MAP_OPT_1WAY);   // horizontal-only — skips the vertical
                                  // buffer, cheaper for a side-scroller
mapSetMapOptions(MAP_OPT_BG2);    // stream a second map onto BG2
```

## Worked patterns (the shipped examples)

- **`examples/maps/map_scroll`** — the full Tiled pipeline (a `.tmj` drawn in
  Tiled, converted with `opensnes-level`, loaded with `mapLoad`) and the
  minimum scroller above, a large map followed by a sprite.
- **`examples/games/mapandobjects`** — per-tile collision *attributes* driving
  slope physics (the `.b16` values mean more than solid/empty).

## Gotchas

### 🔴 `mapLoad()` must run before `setScreenOn()`

`mapLoad` flushes the first screen of tilemap to VRAM internally. Call it
after `setScreenOn()` and that flush lands during active display — garbage
tiles on the first frame. Load during force blank, like every other VRAM
setup.

### 🟠 `mapVblank()` belongs in VBlank, `mapUpdate()` does not

`mapUpdate` stages (cheap, active frame); `mapVblank` writes VRAM (must be
in VBlank). Swapping them, or calling `mapVblank` outside VBlank, drops the
reveal strip and the leading edge of the scroll shows stale tiles.

### 🟡 `map` streams `.m16`; `background` wants `.q16`

These are different formats for different modules. A 64×64 fixed area
scrolled with `bgSetScroll` needs the quadrant layout (`quadrant = true`), not
the streaming `.m16`. Feeding one module the other's format is silent
garbage. Match the format to the module.

## See also

- [Scrolling & parallax](scrolling.md) — raw `bgSetScroll` when you don't
  need a streamed map.
- [Collision detection](collision.md) — `collideTile()` and the `.c16` grid.
- @ref map.h "Map API reference" · @ref tools_opensnes_level
