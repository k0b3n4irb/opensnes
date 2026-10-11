# World Sprites

![Screenshot](world_sprites.png)

Twenty-three coins drift over a 512 × 448 world and a 16 × 16 player sits at
the centre of a 256 × 224 view that the D-pad scrolls. Every frame the
example hands its world-space arrays to `oamPlaceWorld()`, once per batch:
the call subtracts the camera, hides what is off screen, writes the OAM
entries and the ninth x bit, and reports in `visible[]` which sprites it
placed — the loop a scrolling game runs every frame, in assembly.

## SNES Concepts

- World coordinates minus a camera give screen coordinates; a sprite partly
  off the left edge needs the ninth x bit (x in 256..511), which the PPU
  reads as negative.
- OAM culling: a sprite off screen is hidden (x = 257, y = 240), not left
  where it was.
- Structure of arrays (one array per property) is what the 65816 indexes
  cheaply; an `OamWorldBatch` is filled with their addresses once.
- A batch has one size. The player is the LARGE size of `OBJ_SIZE8_L16`,
  set once with `oamSetSize()`: the call never touches the size bit.
- `visible[]` lets the game skip its own work (animation, streaming) for
  the sprites that are not shown — here it counts the coins in view.

## Controls

| Button | Action |
|--------|--------|
| D-pad | Scroll the camera (the player stays centred) |

## How to Build

```bash
make
```

## Modules Used

`console`, `dma`, `background`, `sprite`, `input`, `tile`
