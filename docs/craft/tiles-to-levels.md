# From tiles to levels {#craft_tiles_to_levels}

@ref craft_planning told you unique tiles are what cost VRAM, and reuse is
nearly free. This guide is that idea taken all the way: a SNES level is not a
picture you draw, it is a *rearrangement of a small alphabet of tiles*. Get the
alphabet right and a huge world costs almost nothing; get it wrong and one
screen blows the VRAM budget. Here is how to think in tiles, attach meaning to
them, and stream a level far bigger than the hardware can hold at once.

## The tile is the atom; the metatile is the word

The PPU draws everything from 8×8 tiles. But 8×8 is too small a unit to *design*
in — a brick, a bush, a question block is really a **16×16 metatile**: a 2×2
group of four tiles you place as one. You paint the whole level in metatiles,
and the four underlying tiles come along for free.

This is not just convenience — it is the reuse that makes the VRAM budget work.
A 40-screen level built from 200 unique metatiles still only needs those 200
metatiles' worth of tiles in VRAM; the size lives in the *map* (cheap, 2 bytes
per cell), not the tileset. @ref examples_maps_dynamic_map treats a whole
background as a grid of 16×16 blocks for exactly this reason.

> **Go deeper.** nesdoug's [Metatiles](https://nesdoug.com/2018/09/05/11-metatiles/)
> is the clearest explanation of the technique anywhere. It is written for the
> NES, but the idea — and the memory argument — transfers to the SNES
> unchanged.

## A tile carries meaning, not just pixels

The map is not only what the level *looks* like — it is what the level *is*. Each
tile can carry gameplay properties that travel from the editor into the running
game:

| Property | What it controls | Example values |
|----------|------------------|----------------|
| **attribute** | collision / behaviour | `T_SOLID`, `T_LADDER`, `T_SPIKE`, `T_PLATFORM`, the slope types |
| **palette** | which of the 8 sub-palettes | 0–7 (bits 10–12 of the tilemap entry) |
| **priority** | draws in front of or behind sprites | 0 or 1 (bit 13) |

So a floor tile is not "the grey one" — it is a tile tagged `T_SOLID`, and the
physics code asks the map "what is at this pixel?" rather than hard-coding
geometry. Slopes, ladders, one-way platforms and spikes are all just tile
attributes; @ref examples_maps_slope_collision walks Mario up diagonal terrain
purely from the slope attributes on its tiles, with no per-level collision code.
Design your tile *semantics* alongside your tile art — decide early which tiles
are solid, which hurt, which you can climb.

## The level is bigger than VRAM — so stream it

A 32×32 tilemap is only **2 KB**, and VRAM holds just a screen or two of map at
once. A 224-tile-wide level cannot live in VRAM. The answer is streaming: keep
the *whole* level in ROM (and a working copy in extended WRAM), and copy only
the visible ~32-tile window to VRAM, feeding one new column (or row) as the
camera crosses each tile boundary.

That is a direct application of the @ref craft_frame_budget rules: a single
column is a tiny DMA, easily inside the ~4 KB VBlank budget, so scrolling stays
smooth no matter how wide the world is. @ref examples_maps_map_scroll is the
stepping stone — a Mario walking a level wider than the screen — and
@ref examples_scrolling_continuous_scroll is the same idea run continuously. You
do not manage the window by hand: the map engine's `mapUpdate` / `mapVblank`
detect the boundary crossing and queue the column for you.

The design consequence: **build wide, not dense.** Level width is nearly free
because it streams; what costs you is unique tiles (VRAM) and simultaneous
on-screen sprites (@ref craft_frame_budget). A long level of reused metatiles is
the SNES's comfort zone.

## The workflow: design in Tiled, convert for the SNES

You do not hand-author map bytes. [Tiled](https://www.mapeditor.org/) is a free
level editor, and the SDK bridges it to the hardware. The pipeline
(@ref examples_maps_tiled is the complete worked example):

```
  Tiled  ──►  gfx4snes  ──►  tmx2snes  ──►  mapLoad()
 paint the    tileset PNG    the .tmj map    in your game
 level +      → tiles +      + tileset.map   loop
 tile props   palette+.map   → map binaries
```

1. **Paint** the level in Tiled from your tileset image, and set each tile's
   `attribute` / `palette` / `priority` custom properties (these become the
   collision and rendering data above).
2. **Convert the tileset** with `gfx4snes` — it produces the tiles, the palette,
   and a `.map` optimisation table.
3. **Convert the level** with `tmx2snes`, handing it the `.tmj` export and that
   `.map` table; it emits the tilemap, tile-definition, and attribute binaries.
4. **Load** them with `mapLoad()` and scroll with the map engine.

The point of the tooling is that **the Tiled file stays the single source of
truth.** Change the level in the editor, re-run two commands, and the collision,
palettes and visuals all move together — no separate hand-maintained collision
map to drift out of sync.

## Two ways to hold a level

Not every game wants the streaming map engine. Choose your representation from
what the game needs:

| Your game | Representation | Why |
|-----------|----------------|-----|
| Scrolling platformer / action | **Map engine** (`mapLoad`, `mapUpdate`, `mapVblank`) | Streaming, built-in collision attributes, camera handling |
| Fixed-screen puzzle / board | **Your own array** | A 64×64 area fits without streaming; you keep your own entity state |

`tmx2snes` serves both: its default output feeds the map engine, and its `-Q`
flag emits a quadrant-ordered tilemap you can DMA straight to VRAM and drive
with `bgSetScroll` yourself — so even a game that does not use the engine still
authors its levels in Tiled instead of by hand.

## Designing the level, not just building it

Tooling gets pixels on screen; it does not make a level *good*. That is its own
craft — pacing, teaching a mechanic before testing it, visual variety from a
limited tileset. The best free primer is
[SMW Central's beginners' guide](https://www.smwcentral.net/?p=beginners); it is
about Super Mario World, but the level-design principles are universal.

Two habits from @ref craft_planning carry over here:

- **Grey-box first.** Lay out the whole level with a handful of placeholder
  metatiles — solid, spike, ladder, background — and prove it plays well before
  you draw finished art. Art is expensive and locks in palette/VRAM decisions.
- **Reuse is the point, not a compromise.** A tight tileset that reads clearly
  and recombines into many arrangements beats a sprawling one. The constraint is
  what gives SNES levels their coherent look.

> **Sources for the numbers above:**
> [fullsnes](https://problemkaputt.de/fullsnes.htm) and the
> [SNESdev Wiki](https://snes.nesdev.org/wiki) (tilemap layout, entry format);
> the SDK's own map engine (`lib/include/snes/map.h`) for the streaming model
> and tile-attribute set.
