# Maps

**"let an engine drive the world."** Where
[`scrolling/`](../scrolling/) moves a background past the camera by hand,
these examples hand the world to the `map` engine and a Tiled pipeline: a
scrolling tile world bigger than VRAM, authored in a real editor, read for
collision, and walked with slopes. This is Stage 3, "can I build a world?"

## The ladder

| Example | Developer question |
|---------|--------------------|
| [map_scroll](map_scroll/) | How do I author a map in Tiled and scroll it past the camera when it is bigger than VRAM (the `map` module)? |

Climb from a world authored in Tiled and scrolled past the camera
(`map_scroll`) to a character that stands on its slopes and enemies that
patrol it (`demos/mapandobjects`).

## The idea in one screen

A **hardware tilemap** is a 32×32 (2 KB) up to 64×64 (8 KB) grid of tile
entries the PPU renders every frame; the hardware scroll registers pan across
it for free. When the world is larger than 64×64, the `map` module **streams**
fresh rows/columns into VRAM during VBlank as the camera moves
(`mapLoad`/`mapUpdateCamera`), and `opensnes-level` turns a Tiled `.tmj` into
the `.m16`/`.b16` data it consumes. **Collision** reads tile *properties*
(solid, platform, slope angle) from the map rather than testing sprite boxes;
`collideTileEx` / `objCollidMapWithSlopes` add diagonal surfaces.

Everything here runs inside the ~4 KB VBlank DMA budget by streaming a few
rows or columns per frame (`map_scroll`).

> The capstone that fuses maps + entities + dialog is [`demos/rpg`](../demos/rpg/).
