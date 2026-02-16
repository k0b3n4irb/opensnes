# Animated Sprite — Direction States and Sprite Sheets

## What This Example Shows

How to animate a character sprite that walks in 4 directions with 3 animation frames
each. Demonstrates sprite sheet layout, direction state machines, and horizontal flip
to reuse art.

## Prerequisites

Read `sprites/simple_sprite` first (OAM basics, tile loading).

## Controls

| Button | Action |
|--------|--------|
| D-PAD Up | Walk up |
| D-PAD Down | Walk down |
| D-PAD Left | Walk left (flipped) |
| D-PAD Right | Walk right |

## How It Works

### 1. Sprite sheet layout

The sprite sheet is a 16-pixel-wide BMP strip containing 9 frames of 16x16 sprites:

```
Tile #:  0,2    4,6    8,10   12,14   16,32
         Down1  Down2  Down3  Up1     ...
```

Each 16x16 sprite occupies 2 tile columns in VRAM. The frame index maps to tile
numbers:
- Walking down: tiles 0, 2, 4
- Walking up: tiles 6, 8, 10
- Walking right: tiles 12, 14, 32
- Walking left: same as right, but with `OBJ_FLIPX`

### 2. State machine

```c
enum SpriteState {
    W_DOWN = 0,
    W_UP = 1,
    W_RIGHT = 2,
    W_LEFT = 2  /* Reuses W_RIGHT with flipx */
};
```

Left and right share the same animation row — the PPU's horizontal flip flag
(`OBJ_FLIPX`) mirrors the sprite at zero CPU cost.

### 3. Animation timing

```c
monster.anim_delay++;
if (monster.anim_delay >= ANIM_DELAY) {
    monster.anim_delay = 0;
    monster.anim_frame++;
    if (monster.anim_frame >= 3) monster.anim_frame = 0;
}
```

Animation only advances when a direction button is held. `ANIM_DELAY` (6 frames)
controls walking speed — at 60 fps, this means ~10 animation steps per second.

### 4. Tile calculation

The tile number is computed from the state and frame:

```c
if (state == W_DOWN)  tile = frame * 2;          /* 0, 2, 4 */
if (state == W_UP)    tile = 6 + frame * 2;      /* 6, 8, 10 */
if (state == W_RIGHT) tile = 12 + frame * 2;     /* 12, 14, (32) */
```

Frame 2 of the right-walk is at tile 32 (next row in VRAM) because the sheet wraps.

## SNES Concepts

- **OBJ_FLIPX**: Hardware-level horizontal mirror. Free — no extra tiles, no CPU work.
  Many SNES games use this to halve the sprite art needed for left/right movement.
- **Sprite sheets**: All animation frames are loaded into VRAM at once via
  `oamInitGfxSet()`. The active frame is selected by changing the tile number in
  `oamSet()`.
- **16x16 tiles**: In OAM, a 16x16 sprite uses 4 hardware 8x8 tiles arranged in a
  2x2 grid. The tile number refers to the top-left tile; the PPU fills in the rest.

## Next Steps

- `sprites/dynamic_sprite` — Stream frames instead of pre-loading all of them
- `games/breakout` — Multiple sprites interacting with game logic
