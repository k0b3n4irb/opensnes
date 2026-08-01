# Maps

**Family 5, second half — "let an engine drive the world."** Where
[`scrolling/`](../scrolling/) moves a background past the camera by hand,
these examples hand the world to the `map` engine and a Tiled pipeline: a
scrolling tile world bigger than VRAM, authored in a real editor, read for
collision, and walked with slopes. This is Stage 3, "can I build a world?"

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 5.5 | [map_scroll](map_scroll/) | How do I scroll a map bigger than VRAM (the `map` module)? |
| 5.6 | [tiled](tiled/) | How do I author a map in Tiled and load it? |
| 5.7 | [dynamic_map](dynamic_map/) | How do I drive a sprite from a tilemap and swap 32×32 ↔ 64×64 map modes? |
| 5.8 | [slope_collision](slope_collision/) | How do I do tile collision with slopes — a full platformer? |

Climb from a scrolling world (5.5) to an authored one (5.6) to a sprite that
reads it (5.7) to a character that stands on its slopes (5.8).

## The idea in one screen

A **hardware tilemap** is a 32×32 (2 KB) up to 64×64 (8 KB) grid of tile
entries the PPU renders every frame; the hardware scroll registers pan across
it for free. When the world is larger than 64×64, the `map` module **streams**
fresh rows/columns into VRAM during VBlank as the camera moves
(`mapLoad`/`mapUpdateCamera`), and `tmx2snes` turns a Tiled `.tmx`/`.tmj` into
the `.m16`/`.b16` data it consumes. **Collision** reads tile *properties*
(solid, platform, slope angle) from the map rather than testing sprite boxes;
`collideTileEx` / `objCollidMapWithSlopes` add diagonal surfaces.

Everything here runs inside the ~4 KB VBlank DMA budget by streaming one page
per frame — the pattern `dynamic_map` demonstrates directly.

> The capstone that fuses maps + entities + dialog is [`games/rpg`](../games/rpg/).
