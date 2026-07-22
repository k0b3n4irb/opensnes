# Collision Detection Tutorial {#tutorial_collision}

This tutorial covers collision detection on the SNES, from basic bounding box checks to tile-based world collision used in platformers.

## Why AABB?

The SNES has no hardware collision detection. All collision must be done in software by the CPU. Axis-aligned bounding box (AABB) testing is the standard approach because it requires only four comparisons per pair of objects -- fast enough for the 65816 at 3.58 MHz.

## The Rect Type

OpenSNES provides a `Rect` structure representing an axis-aligned bounding box:

```c
#include <snes.h>
#include <snes/collision.h>

typedef struct {
    s16 x;          /* Left edge X coordinate */
    s16 y;          /* Top edge Y coordinate */
    u16 width;      /* Width in pixels */
    u16 height;     /* Height in pixels */
} Rect;
```

Initialize it with `rectInit()` or by direct assignment:

```c
Rect player_box;
rectInit(&player_box, player_x, player_y, 16, 16);
```

## Sprite-to-Sprite Collision

### Basic Overlap Test

Use `collideRect()` to test whether two rectangles overlap:

```c
Rect player_box;
Rect enemy_box;

static void check_collisions(void) {
    player_box.x = player_x;
    player_box.y = player_y;
    player_box.width = 16;
    player_box.height = 16;

    enemy_box.x = enemy_x;
    enemy_box.y = enemy_y;
    enemy_box.width = 16;
    enemy_box.height = 16;

    if (collideRect(&player_box, &enemy_box)) {
        /* Collision detected */
        player_hit();
    }
}
```

### Testing Multiple Enemies

The `collision_demo` example checks the player against four enemies each frame using a bitmask to track which enemies are currently colliding:

```c
#define NUM_ENEMIES 4

static Rect player_box;
static Rect enemy_box[NUM_ENEMIES];
static u8 collision_flags;

static void check_collisions(void) {
    u8 i;
    collision_flags = 0;

    player_box.x = player_x;
    player_box.y = player_y;
    player_box.width = PLAYER_SIZE;
    player_box.height = PLAYER_SIZE;

    for (i = 0; i < NUM_ENEMIES; i++) {
        enemy_box[i].x = enemy_x[i];
        enemy_box[i].y = enemy_y[i];
        enemy_box[i].width = ENEMY_SIZE;
        enemy_box[i].height = ENEMY_SIZE;

        if (collideRect(&player_box, &enemy_box[i])) {
            collision_flags |= (1 << i);
        }
    }
}
```

### Point-vs-Rectangle

Use `collidePoint()` when checking a single coordinate (bullet, cursor) against a target:

```c
if (collidePoint(bullet_x, bullet_y, &target_box)) {
    target_hit();
}
```

## Sprite-to-Background Collision

### Reading Tilemap Data

For platformers and top-down games, the world is made of tiles. A separate collision map (one byte per tile, 0 = empty, nonzero = solid) lets you check whether a pixel position is blocked:

```c
#define MAP_WIDTH 16

static u8 collision_map[MAP_WIDTH * MAP_HEIGHT] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  /* Top wall */
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    /* ... */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  /* Bottom wall */
};
```

Use `collideTile()` to look up the tile at a pixel position:

```c
if (collideTile(player_x + 8, player_y + 16, collision_map, MAP_WIDTH)) {
    /* Standing on solid ground */
    on_ground = 1;
}
```

### Checking All Four Corners

A single point check misses edges. The `collision_demo` example checks all four corners of the player bounding box to prevent clipping through walls:

```c
static u8 check_wall_collision(s16 new_x, s16 new_y) {
    s16 map_x, map_y;

    map_x = new_x - MAP_OFFSET_X;
    map_y = new_y - MAP_OFFSET_Y;

    /* Top-left */
    if (collideTile(map_x, map_y, collision_map, MAP_WIDTH)) return 1;
    /* Top-right */
    if (collideTile(map_x + PLAYER_SIZE - 1, map_y, collision_map, MAP_WIDTH)) return 1;
    /* Bottom-left */
    if (collideTile(map_x, map_y + PLAYER_SIZE - 1, collision_map, MAP_WIDTH)) return 1;
    /* Bottom-right */
    if (collideTile(map_x + PLAYER_SIZE - 1, map_y + PLAYER_SIZE - 1, collision_map, MAP_WIDTH)) return 1;

    return 0;
}
```

You can also use `collideRectTile()` which does this internally:

```c
Rect playerBox = { map_x, map_y, PLAYER_SIZE, PLAYER_SIZE };
if (collideRectTile(&playerBox, collision_map, MAP_WIDTH)) {
    /* Player hit a solid tile */
}
```

### Custom Tile Sizes

For maps with 16x16 tiles, use `collideTileEx()`:

```c
if (collideTileEx(player_x, player_y + 16, collision_map, MAP_WIDTH, 16)) {
    on_ground = 1;
}
```

### Tile Properties from a Tilemap (Platformer Pattern)

The `likemario` example uses 16-bit tile properties looked up from the visual tilemap, rather than a separate collision map. Each tile index maps to a property value (`T_SOLID` or `T_EMPTY`):

```c
#define T_EMPTY  0x0000
#define T_SOLID  0xFF00

static u16 map_get_tile_prop(s16 px, s16 py) {
    u16 tx, ty;

    if (px < 0 || py < 0) return T_SOLID;

    tx = (u16)px >> 3;   /* Divide by 8 (tile width) */
    ty = (u16)py >> 3;

    if (tx >= map_width || ty >= map_height) return T_EMPTY;

    return tile_props[map_row_ptrs[ty][tx] & 0x03FF];
}
```

This avoids storing a duplicate collision map -- the visual tilemap doubles as the collision source.

## Where the sprite is vs where it collides

In a top-down game the character occupies **one tile**, but its sprite
is usually 16×16 — twice that tile. Where you draw those 16×16 pixels
relative to the tile decides whether collision *feels* right, and it is
the single most common source of "my collision is broken" reports that
turn out to have nothing wrong with the collision code.

The naive version draws the sprite at the tile's corner:

```c
oamSet(id, tile_x * 8 - cam_x, tile_y * 8 - cam_y, ...);   /* wrong */
```

The tile is 8×8 and the sprite is 16×16, so the visible body sits half a
tile away from the thing that actually collides. Walking up to a wall,
you stop with a visible gap on one side and clipping into it on the
other, *depending on the direction you approached from*. Players report
that as "collisions are random — sometimes too early, sometimes too
late", which sends you reading `collideTile()`, where there is no bug.

The convention is to **straddle** the tile: feet on it, body overhanging
upward.

```c
u16 sx = (u16)(tile_x * 8 - cam_x - 4);   /* centre the 16px body on an 8px tile */
u16 sy = (u16)(tile_y * 8 - cam_y - 8);   /* stand the feet on the tile */
oamSet(id, sx, sy, ...);
```

```
        ┌────────┐
        │        │      the sprite (16×16)
        │  body  │
        ├───┬────┤
        │   │▓▓▓▓│      the tile it occupies (8×8),
        └───┴────┘      and the only thing that collides
```

Now what you see is what collides, from every direction. `examples/games/rpg`
does this in `draw_char()`.

Two consequences worth knowing:

- **The sprite overlaps the tile above it.** That is intended — it is
  what makes a character look like it is standing *in* the scene rather
  than on top of it. It also means the tile above must be drawn before
  the sprite, i.e. the sprite needs a priority that puts it over the
  background layer.
- **Collision stays tile-exact.** You are not colliding a 16×16 box
  against tiles; you are asking "is the tile I want to step into
  solid?". One `collideTile()` call per step, not four per frame.

## Collision Response Patterns

Detecting a collision is only half the problem. What happens next matters more.

### Stopping Movement (Slide Along Walls)

When the player tries to move diagonally into a wall, test each axis independently so they can slide along the wall instead of getting stuck:

```c
new_x = player_x;
new_y = player_y;

if (pad & KEY_LEFT)  new_x -= PLAYER_SPEED;
if (pad & KEY_RIGHT) new_x += PLAYER_SPEED;
if (pad & KEY_UP)    new_y -= PLAYER_SPEED;
if (pad & KEY_DOWN)  new_y += PLAYER_SPEED;

if (!check_wall_collision(new_x, new_y)) {
    /* No collision -- accept full movement */
    player_x = new_x;
    player_y = new_y;
} else {
    /* Try each axis separately */
    if (!check_wall_collision(new_x, player_y)) {
        player_x = new_x;  /* Horizontal OK */
    }
    if (!check_wall_collision(player_x, new_y)) {
        player_y = new_y;  /* Vertical OK */
    }
}
```

### Pushing Out of Overlap

Use `collideRectEx()` to get the overlap amount and push objects apart. This is useful for objects that can drift into each other (e.g., physics objects):

```c
s16 dx, dy;
if (collideRectEx(&player, &wall, &dx, &dy)) {
    player_x -= dx;
    player_y -= dy;
}
```

### Bouncing (Breakout Pattern)

The `breakout` example reverses velocity on collision with walls and paddle. Negation uses a temporary variable to work around compiler constraints:

```c
/* Wall bounce */
if (pos_x > 171) {
    s16 neg_vx = -vel_x;
    vel_x = neg_vx;
    pos_x = 171;
}

/* Paddle bounce with angle control */
if (pos_y > 195 && pos_y < 203) {
    if (pos_x >= px && pos_x <= px + 27) {
        u8 zone = (pos_x - px) / 7;
        switch (zone) {
            case 0: vel_x = -2; vel_y = -1; break;
            case 1: vel_x = -1; vel_y = -2; break;
            case 2: vel_x =  1; vel_y = -2; break;
            default: vel_x = 2; vel_y = -1; break;
        }
    }
}
```

### Snapping to Tile Grid (Platformer Landing)

When a falling character hits the ground, snap their Y position to the tile boundary to prevent sinking into the floor:

```c
/* Ground check: test two points at the bottom edge */
prop = map_get_tile_prop(mario_x + 2, mario_y + 16);
if (prop == T_EMPTY)
    prop = map_get_tile_prop(mario_x + 13, mario_y + 16);

if (prop != T_EMPTY) {
    /* Snap to tile boundary */
    mario_y = ((mario_y + 16) & 0xFFF8) - 16;
    mario_yvel = 0;
    on_ground = 1;
}
```

The mask `0xFFF8` rounds down to the nearest 8-pixel tile boundary. Subtracting the sprite height (16) places the sprite so its feet rest exactly on top of the solid tile.

### Grid-Based Collision (Breakout Bricks)

The `breakout` example converts the ball's pixel position to a grid coordinate and checks a brick array directly:

```c
/* Convert pixel to brick grid (16x8 pixel bricks) */
bx = (pos_x - 14) >> 4;   /* Divide by 16 (brick width) */
by = (pos_y - 14) >> 3;   /* Divide by 8 (brick height) */

if (bx < 10 && by >= 1 && by <= 10) {
    u16 idx = bx + (by << 3) + (by << 1) - 10;
    if (blocks[idx] != 8) {
        /* Brick hit -- remove it and bounce */
        remove_brick();
    }
}
```

## Performance Tips

1. **Check collision only when objects are close.** Skip `collideRect()` for enemies off screen.
2. **Use bitmasks for groups.** Track collision state with bit flags (`collision_flags |= (1 << i)`) instead of boolean arrays.
3. **Separate axes.** Test X and Y independently -- this is cheaper than diagonal resolution and gives better sliding behavior.
4. **Power-of-2 tile sizes.** Use 8 or 16 pixel tiles so division becomes a bit shift (`>> 3` or `>> 4`).
5. **Avoid `collideRect()` in tight loops with many objects.** For N-vs-N collision, consider spatial partitioning (grid cells) to reduce the number of pairs tested.

## Complete Examples

- `examples/basics/collision_demo/` -- AABB sprite collision with tile-based wall collision and visual feedback
- `examples/games/breakout/` -- Ball vs walls, ball vs paddle (angle control), ball vs brick grid
- `examples/games/likemario/` -- Platformer tile collision with gravity, ground snapping, and wall sliding

## Next Steps

- @ref collision.h "Collision API Reference"
- @ref tutorial_sprites "Sprites & Animation"
- @ref tutorial_input "Input Handling"
