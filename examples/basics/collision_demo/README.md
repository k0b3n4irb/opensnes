# Collision Detection Example

Multiple collision systems: AABB, point, and tile-based.

## Learning Objectives

After this lesson, you will understand:
- Axis-aligned bounding box (AABB) collision
- Point vs rectangle collision
- Tile-based world collision
- Visual collision feedback

## Prerequisites

- Completed sprite examples
- Understanding of coordinate systems

---

## What This Example Does

Demonstrates collision detection systems:
- Player sprite navigating a tile-based level
- Walls and platforms block movement
- Visual feedback when colliding
- Multiple collision check methods

```
+----------------------------------------+
|████████████████████████████████████████|
|█                                      █|
|█    ████        ████        ████      █|
|█                                      █|
|█         [PLAYER]                     █|
|█                                      █|
|█    ████████████████████              █|
|█                                      █|
|████████████████████████████████████████|
+----------------------------------------+
```

**Controls:**
- D-Pad: Move player sprite
- A button: Toggle collision display (if implemented)

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Collision checks | Library (`collideRect`, `collideTile`) |
| Sprite display | Library (`oamSet`, `oamUpdate`) |
| Level data | C array |
| Input handling | Direct register access |

---

## Collision Types

### 1. AABB Collision (Sprite vs Sprite)

Two rectangles overlap if all conditions are true:
```c
u8 collideRect(s16 x1, s16 y1, u8 w1, u8 h1,
               s16 x2, s16 y2, u8 w2, u8 h2) {
    return (x1 < x2 + w2) &&
           (x1 + w1 > x2) &&
           (y1 < y2 + h2) &&
           (y1 + h1 > y2);
}
```

### 2. Point vs Tile (World Collision)

Check if a point is inside a solid tile:
```c
u8 collideTile(s16 x, s16 y) {
    u8 tile_x = x >> 3;  /* Divide by tile size (8) */
    u8 tile_y = y >> 3;
    return collision_map[tile_y * MAP_WIDTH + tile_x];
}
```

### 3. Four-Corner Check

For moving objects, check all corners:
```c
u8 checkCollision(s16 x, s16 y, u8 w, u8 h) {
    return collideTile(x, y) ||           /* Top-left */
           collideTile(x + w, y) ||       /* Top-right */
           collideTile(x, y + h) ||       /* Bottom-left */
           collideTile(x + w, y + h);     /* Bottom-right */
}
```

---

## Collision Map

### Map Format

```c
#define MAP_WIDTH 16
#define MAP_HEIGHT 14

static const u8 collision_map[MAP_WIDTH * MAP_HEIGHT] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  /* Top wall */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,1,1,0,0,0,0,0,1,  /* Platforms */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* ... */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  /* Bottom wall */
};
/* 0 = passable, 1 = solid */
```

---

## Movement with Collision

### Basic Movement

```c
void movePlayer(void) {
    s16 new_x = player_x;
    s16 new_y = player_y;

    if (pad & KEY_RIGHT) new_x += 2;
    if (pad & KEY_LEFT) new_x -= 2;
    if (pad & KEY_DOWN) new_y += 2;
    if (pad & KEY_UP) new_y -= 2;

    /* Only move if no collision */
    if (!checkCollision(new_x, player_y, 16, 16)) {
        player_x = new_x;
    }
    if (!checkCollision(player_x, new_y, 16, 16)) {
        player_y = new_y;
    }
}
```

### Slide Along Walls

Check axes separately for smooth movement:
```c
/* Try X movement */
if (!checkCollision(new_x, player_y, 16, 16)) {
    player_x = new_x;
}

/* Try Y movement independently */
if (!checkCollision(player_x, new_y, 16, 16)) {
    player_y = new_y;
}
/* Player slides along walls instead of stopping completely */
```

---

## Visual Feedback

```c
void updateSprite(void) {
    u8 palette = 0;  /* Normal color */

    /* Check if currently colliding */
    if (checkCollision(player_x, player_y, 16, 16)) {
        palette = 1;  /* Red/highlighted color */
    }

    oamSet(0, player_x, player_y, 0, palette, 3, 0);
}
```

---

## Build and Run

```bash
cd examples/basics/collision_demo
make clean && make

# Run in emulator
/path/to/Mesen collision_demo.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Collision logic and game loop |
| `data.asm` | Level and sprite graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Collectibles

Add items that disappear on contact:
```c
if (collideRect(player_x, player_y, 16, 16,
                item_x, item_y, 8, 8)) {
    item_collected = 1;
    score++;
}
```

### Exercise 2: Pushable Objects

Implement a box that can be pushed:
```c
if (collideRect(player, box)) {
    box_x += player_velocity_x;
    /* Check if box hits wall */
    if (checkCollision(box_x, box_y, 16, 16)) {
        box_x -= player_velocity_x;
        /* Push failed - block player too */
    }
}
```

### Exercise 3: One-Way Platforms

Allow jumping through from below:
```c
if (velocity_y > 0 &&  /* Moving down */
    player_y + 16 <= platform_y + 4) {  /* Above platform */
    /* Allow passing through */
} else {
    /* Block */
}
```

### Exercise 4: Slope Collision

Implement diagonal surfaces:
```c
/* Calculate height at current X position */
s16 slope_height = getSlopeHeight(player_x);
if (player_y + 16 > slope_height) {
    player_y = slope_height - 16;
}
```

---

## Technical Notes

### Performance

Tile collision is O(1) - just array lookup. AABB checks are also O(1) but comparing many sprites is O(n).

### Hitbox vs Visual Size

Often hitboxes are smaller than sprites:
```c
#define HITBOX_X_OFFSET 2
#define HITBOX_WIDTH 12  /* vs 16 pixel sprite */
```

### Subpixel Collision

For fixed-point positions, round to pixels for collision:
```c
s16 pixel_x = fixed_x >> 8;
```

### Collision Layers

Different objects collide with different things:
```c
#define COL_PLAYER 0x01
#define COL_ENEMY  0x02
#define COL_BULLET 0x04

if (obj1.layer & obj2.mask) { /* Check collision */ }
```

---

## What's Next?

**Smooth Movement:** [Smooth Movement](../smooth_movement/) - Fixed-point physics

**Game:** [Breakout](../../game/breakout/) - Complete game

---

## License

Code: MIT
