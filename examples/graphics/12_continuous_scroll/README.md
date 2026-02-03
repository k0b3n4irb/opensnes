# Continuous Scroll Example

Game-like scrolling with player sprite and camera following.

## Learning Objectives

After this lesson, you will understand:
- Camera-relative scrolling systems
- Player movement with screen boundaries
- Threshold-based camera scrolling
- NMI callback for timing-critical updates

## Prerequisites

- Completed scrolling and sprite examples
- Understanding of VBlank timing

---

## What This Example Does

A complete scrolling game framework:
- Player-controlled sprite moves on screen
- Camera follows when player crosses thresholds
- Background scrolls smoothly
- Uses NMI interrupt for consistent timing

```
+----------------------------------------+
|                                        |
|    [SCROLLABLE WORLD]                  |
|                                        |
|              [PLAYER]                  |
|                 ↓                      |
|    When player reaches edge,           |
|    camera scrolls to follow            |
+----------------------------------------+
```

**Controls:**
- D-Pad: Move player sprite
- Camera scrolls when player nears edges

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Sprite handling | Library (`oamSet`, `oamUpdate`) |
| Background scroll | Library (`bgSetScroll`) |
| NMI callback | Library (`nmiSetBank`) |
| Graphics loading | Library (`oamInitGfxSet`, DMA) |
| Input handling | Library or direct registers |

---

## Game State Pattern

### Required Struct Pattern

Using a struct with `s16` coordinates is required for reliable sprite movement.
Separate `u16` variables cause horizontal movement issues due to a compiler quirk.

```c
typedef struct {
    s16 player_x, player_y;
    s16 bg1_scroll_x, bg1_scroll_y;
    s16 bg2_scroll_x, bg2_scroll_y;
    u8 need_scroll_update;
} GameState;

GameState game = {20, 100, 0, 32, 0, 32, 1};
```

---

## Camera System

### Screen Space vs World Space

```
World Coordinates: Absolute position in level
Screen Coordinates: Position on visible screen

screen_x = world_x - camera_x
screen_y = world_y - camera_y
```

### Threshold-Based Scrolling

```c
#define SCROLL_THRESHOLD_LEFT  80
#define SCROLL_THRESHOLD_RIGHT 140

/* Check if player crossed threshold */
if (game.player_x > SCROLL_THRESHOLD_RIGHT && game.bg1_scroll_x < MAX_SCROLL_X) {
    game.bg1_scroll_x++;
    game.player_x--;  /* Push player back */
}
if (game.player_x < SCROLL_THRESHOLD_LEFT && game.bg1_scroll_x > 0) {
    game.bg1_scroll_x--;
    game.player_x++;  /* Push player forward */
}
```

---

## Player Movement

### Position Update

```c
if (pad & KEY_RIGHT) { if (game.player_x < 230) game.player_x += 2; }
if (pad & KEY_LEFT)  { if (game.player_x > 8) game.player_x -= 2; }
if (pad & KEY_DOWN)  { if (game.player_y < 200) game.player_y += 2; }
if (pad & KEY_UP)    { if (game.player_y > 32) game.player_y -= 2; }
```

### Sprite Update

```c
/* Use oamSet() instead of oamSetXY() for reliable updates */
oamSet(0, game.player_x, game.player_y, 0, 0, 2, 0);
```

---

## NMI Callback

For smooth updates, use NMI (VBlank interrupt):

```c
void myVBlankHandler(void) {
    if (game.need_scroll_update) {
        /* Update scroll registers during VBlank */
        bgSetScroll(0, game.bg1_scroll_x, game.bg1_scroll_y);
        bgSetScroll(1, game.bg2_scroll_x, game.bg2_scroll_y);
        game.need_scroll_update = 0;
    }
}

/* In main() - check .sym file for correct bank number */
nmiSetBank(myVBlankHandler, 1);
```

---

## Build and Run

```bash
cd examples/graphics/12_continuous_scroll
make clean && make

# Run in emulator
/path/to/Mesen continuous_scroll.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Game loop, player logic, camera |
| `data.asm` | Level and sprite graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Smooth Camera

Add camera smoothing (lerp toward target):
```c
s16 target_x = game.player_x - 128;  /* Center player */
game.bg1_scroll_x += (target_x - game.bg1_scroll_x) >> 3;  /* Smooth follow */
```

### Exercise 2: Camera Bounds

Prevent camera from showing beyond level edges:
```c
if (game.bg1_scroll_x < 0) game.bg1_scroll_x = 0;
if (game.bg1_scroll_x > LEVEL_WIDTH - 256) game.bg1_scroll_x = LEVEL_WIDTH - 256;
```

### Exercise 3: Parallax Background

Add a slower-scrolling background layer (already implemented in this example):
```c
bgSetScroll(0, game.bg1_scroll_x, game.bg1_scroll_y);       /* Main layer */
bgSetScroll(1, game.bg2_scroll_x, game.bg2_scroll_y);       /* Parallax */
```

### Exercise 4: Multiple Sprites

Add enemies that also scroll with camera:
```c
for (u8 i = 0; i < enemy_count; i++) {
    s16 sx = enemies[i].x - game.bg1_scroll_x;
    s16 sy = enemies[i].y - game.bg1_scroll_y;
    oamSet(i + 1, sx, sy, enemy_tile, 0, 2, 0);
}
```

---

## Technical Notes

### Sprite Off-Screen Handling

When sprites scroll off-screen:
```c
if (screen_x < -16 || screen_x > 256) {
    oamSetVisible(sprite_id, OBJ_HIDE);
} else {
    oamSetVisible(sprite_id, OBJ_SHOW);
}
```

### Fixed-Point Camera

For sub-pixel smooth scrolling:
```c
static s32 camera_x_fp = 0;  /* 16.16 fixed point */
camera_x_fp += (target_x << 16 - camera_x_fp) >> 4;
bgSetScroll(0, camera_x_fp >> 16, 0);
```

### Level Streaming

For very large levels, load new tile data as player moves:
1. Detect when camera approaches edge of loaded area
2. DMA new tiles during VBlank
3. Update tilemap entries

---

## What's Next?

**Animation:** [Animation System](../13_animation_system/) - Sprite animation

**Metasprites:** [Metasprite](../14_metasprite/) - Multi-tile sprites

---

## License

Code: MIT
