# World Sprites

![Screenshot](world_sprites.png)

Seventeen coins drift over a 512 × 448 world, six square walkers pace up
and down near its centre, and the D-pad scrolls a 256 × 224 view of it.
Every frame the example hands its world-space arrays to `oamPlaceWorld()`,
once per batch: the call subtracts the camera, hides what is off screen,
writes the OAM entries and the ninth x bit, and reports in `visible[]`
which sprites it placed — the loop a scrolling game runs every frame, in
assembly. The walkers are sorted by depth and the batch's `order` says
which walker each slot shows, the way a game seen from above sorts its
actors.

## SNES Concepts

- World coordinates minus a camera give screen coordinates; a sprite partly
  off the left edge needs the ninth x bit (x in 256..511), which the PPU
  reads as negative.
- OAM culling: a sprite off screen is hidden (x = 257, y = 240), not left
  where it was.
- OAM order is drawing order: the lower slot is in front. Seen from above,
  the sprite lower on the screen is nearer, so the walkers are sorted by y
  every frame (an insertion sort of six bytes) and `order` permutes the
  slots — the arrays stay indexed by the walker, nothing is copied.
- Structure of arrays (one array per property) is what the 65816 indexes
  cheaply; an `OamWorldBatch` is filled with their addresses once.
- A batch has one size: the coins are the SMALL size of `OBJ_SIZE16_L32`
  (16 × 16), the walkers the LARGE one (32 × 32), set once with
  `oamSetSize()`: the call never touches the size bit.
- `visible[]` tells the game which sprites were placed — here it counts
  the coins in view; a game reads it to stream the animation frames of
  the sprites that are shown and skip the others, so that a sprite off
  screen costs no VRAM DMA.
- A batch costs the same whether its sprites are on screen or not (the
  call tests each one); a game that hides a whole group skips the call
  with a flag of its own rather than placing hidden sprites.

## Controls

| Button | Action |
|--------|--------|
| D-pad | Scroll the camera |

## How to Build

```bash
make
```

## Modules Used

`console`, `dma`, `background`, `sprite`, `input`, `tile`
