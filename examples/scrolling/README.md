# Scrolling

**Part of Stage 3 — "can I build a world?"** A game world is bigger than one
screen. These examples move the world past the camera: layers at different
speeds for depth, a world streamed a column at a time, and per-scanline HDMA
scrolling for the parallax the SNES is famous for.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [mixed_scroll](mixed_scroll/) | How do I scroll layers at different rates for depth? |
| [continuous_scroll](continuous_scroll/) | How does a camera follow the player through a world wider than the screen? |
| [parallax_scroll](parallax_scroll/) | How do I get per-scanline parallax with HDMA? |

## The idea in one screen

Every background has hardware scroll registers (`BGxHOFS`/`BGxVOFS`); you
change *where the PPU reads*, never the tilemap, so scrolling is nearly free.
Scroll two layers at different rates and the far one looks distant
(`mixed_scroll`). A camera follows the player by scrolling the world instead
of moving him past a line of the screen (`continuous_scroll`, over 64x32 maps);
when the world is larger than the tilemap, a map engine streams a fresh column
of tiles into VRAM each time the camera crosses a tile boundary (`maps/`). For a gradient of speeds within *one* layer, rewrite
the scroll register **every scanline** with HDMA (`parallax_scroll`).

> Next stop in Stage 3: [`maps/`](../maps/) — let a map engine and a Tiled
> pipeline drive the world instead of hand-streaming it.
