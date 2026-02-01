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
#define SCROLL_THRESHOLD_L 64
#define SCROLL_THRESHOLD_R 192

/* Check if player crossed threshold */
if (player_screen_x > SCROLL_THRESHOLD_R) {
    camera_x += player_screen_x - SCROLL_THRESHOLD_R;
}
if (player_screen_x < SCROLL_THRESHOLD_L) {
    camera_x -= SCROLL_THRESHOLD_L - player_screen_x;
}
```

---

## Player Movement

### Position Update

```c
if (pad & KEY_RIGHT) player_x += 2;
if (pad & KEY_LEFT) player_x -= 2;
if (pad & KEY_DOWN) player_y += 2;
if (pad & KEY_UP) player_y -= 2;

/* Clamp to world bounds */
if (player_x < 0) player_x = 0;
if (player_x > WORLD_WIDTH - 16) player_x = WORLD_WIDTH - 16;
```

### Screen Position Calculation

```c
s16 screen_x = player_x - camera_x;
s16 screen_y = player_y - camera_y;

oamSetXY(0, screen_x, screen_y);
```

---

## NMI Callback

For smooth updates, use NMI (VBlank interrupt):

```c
void nmiCallback(void) {
    /* Update scroll registers */
    bgSetScroll(0, camera_x, camera_y);

    /* Update OAM */
    oamUpdate();
}

/* In main() */
nmiSetBank(0);  /* Set callback bank */
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
s16 target_x = player_x - 128;  /* Center player */
camera_x += (target_x - camera_x) >> 3;  /* Smooth follow */
```

### Exercise 2: Camera Bounds

Prevent camera from showing beyond level edges:
```c
if (camera_x < 0) camera_x = 0;
if (camera_x > LEVEL_WIDTH - 256) camera_x = LEVEL_WIDTH - 256;
```

### Exercise 3: Parallax Background

Add a slower-scrolling background layer:
```c
bgSetScroll(0, camera_x, camera_y);       /* Main layer */
bgSetScroll(1, camera_x >> 1, camera_y);  /* Parallax */
```

### Exercise 4: Multiple Sprites

Add enemies that also scroll with camera:
```c
for (u8 i = 0; i < enemy_count; i++) {
    s16 sx = enemies[i].x - camera_x;
    s16 sy = enemies[i].y - camera_y;
    oamSetXY(i + 1, sx, sy);
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
