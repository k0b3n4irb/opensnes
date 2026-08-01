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
[Tiled](https://www.mapeditor.org/), export JSON (`.tmj`), and convert with
the SDK's `tmx2snes`:

```bash
tmx2snes town.tmj tileset.map
```

It emits four files the module consumes:

| File | Content | Consumed by |
|------|---------|-------------|
| `<layer>.m16` | the streaming tilemap (tile indices + flip bits) | `mapLoad()` arg 1 |
| `<layer>.t16` | per-tile palette + priority | `mapLoad()` arg 2 (tileset def) |
| `<layer>.b16` | per-tile collision attribute | `mapLoad()` arg 3 |
| `<layer>.o16` | the Entities object layer | the `object` module |

`tmx2snes` also has three flags for games that do **not** use the `map`
module and want the same content in a different shape:

| Flag | Output | For |
|------|--------|-----|
| `-e` | `<map>.inc` — entities as C defines (struct + rows, custom properties) | a game that keeps its own entity structs |
| `-Q` | `<layer>.q16` — a quadrant-ordered 64×64 tilemap | scrolling a fixed 64×64 area with `bgSetScroll` (the `background` module, not `map`) |
| `-C` | `<layer>.c16` — one collision byte per map cell | `collideTile()` (a flattened grid, indexed directly) |

`examples/games/rpg` uses `-Q`/`-e`/`-C` (it scrolls a fixed 64×64 town with
`background`, not `map`); `examples/maps/tiled` uses the default `map`
pipeline. Pick the module that matches the map size — `map` for worlds
larger than 64×64, `background` for a fixed screen or two.

> **cute_tiled needs no special export.** `tmx2snes` minifies the JSON
> itself, so a pretty-printed `.tmj` from a generator script parses fine.

## The minimum viable scroller

```c
#include <snes.h>
#include <snes/map.h>

extern u8 world_map[], world_tiles[], world_props[];   // the .m16/.t16/.b16

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    // mapLoad configures BG1, uploads the tileset, and flushes the first
    // screen of tilemap to VRAM. It must run BEFORE setScreenOn().
    mapLoad(world_map, world_tiles, world_props);

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
  exposes the live scroll in the globals `x_pos` / `y_pos`.
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
module with a `-C` grid uses `collideTile()` instead — see the
[collision tutorial](collision.md).)

## One-way and second-layer options

```c
mapSetMapOptions(MAP_OPT_1WAY);   // horizontal-only — skips the vertical
                                  // buffer, cheaper for a side-scroller
mapSetMapOptions(MAP_OPT_BG2);    // stream a second map onto BG2
```

## Worked patterns (the shipped examples)

- **`examples/maps/map_scroll`** — the minimum scroller above, a large map
  moved with the D-pad.
- **`examples/maps/tiled`** — the full Tiled pipeline: a `.tmj` drawn in
  Tiled, converted with `tmx2snes`, loaded with `mapLoad`.
- **`examples/maps/slope_collision`** — per-tile collision *attributes* driving
  slope physics (the `.b16` values mean more than solid/empty).
- **`examples/maps/dynamic_map`** — rewriting map tiles at runtime.

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
scrolled with `bgSetScroll` needs the quadrant layout (`tmx2snes -Q`), not
the streaming `.m16`. Feeding one module the other's format is silent
garbage. Match the format to the module.

## See also

- [Scrolling & parallax](scrolling.md) — raw `bgSetScroll` when you don't
  need a streamed map.
- [Collision detection](collision.md) — `collideTile()` and the `-C` grid.
- @ref map.h "Map API reference" · `tools/tmx2snes/README.md`
