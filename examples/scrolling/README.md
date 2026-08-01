# Scrolling

**Part of Stage 3 — "can I build a world?"** A game world is bigger than one
screen. These examples move the world past the camera: layers at different
speeds for depth, a world streamed a column at a time, and per-scanline HDMA
scrolling for the parallax the SNES is famous for.

## The ladder

| Rung | Example | Developer question |
|------|---------|--------------------|
| 5.1 | [mixed_scroll](mixed_scroll/) | How do I scroll layers at different rates for depth? |
| 5.2 | [continuous_scroll](continuous_scroll/) | How do I scroll a world bigger than one screen, streaming tiles at the seam? |
| 5.3 | [parallax_scroll](parallax_scroll/) | How do I get per-scanline parallax with HDMA? |

## The idea in one screen

Every background has hardware scroll registers (`BGxHOFS`/`BGxVOFS`); you
change *where the PPU reads*, never the tilemap, so scrolling is nearly free.
Scroll two layers at different rates and the far one looks distant
(`mixed_scroll`). When the world is larger than the 32×32 tilemap, stream a
fresh column of tiles into VRAM each time the camera crosses a tile boundary
(`continuous_scroll`). For a gradient of speeds within *one* layer, rewrite
the scroll register **every scanline** with HDMA (`parallax_scroll`).

> Next stop in Stage 3: [`maps/`](../maps/) — let a map engine and a Tiled
> pipeline drive the world instead of hand-streaming it.
