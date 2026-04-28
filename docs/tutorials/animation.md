# Animation Tutorial {#tutorial_animation}

This tutorial covers sprite and background animation techniques on the SNES, from simple frame cycling to the dynamic sprite engine.

## Frame-Based Animation

The simplest animation technique: cycle through tile numbers at a fixed interval. Each animation frame corresponds to a different tile (or group of tiles) in VRAM.

```c
#include <snes.h>

#define ANIM_FRAMES 3
#define ANIM_DELAY  6   /* VBlanks between frame changes */

u8 anim_frame = 0;
u8 anim_timer = 0;

void update_animation(void) {
    anim_timer++;
    if (anim_timer >= ANIM_DELAY) {
        anim_timer = 0;
        anim_frame++;
        if (anim_frame >= ANIM_FRAMES)
            anim_frame = 0;
    }

    /* Each 16x16 sprite occupies 2 tile columns in VRAM,
     * so frame 0 = tile 0, frame 1 = tile 2, frame 2 = tile 4 */
    oamSetTile(0, anim_frame * 2);
}
```

The key insight: **animation only advances when the timer expires**, not every frame. This decouples visual speed from the 60 Hz game loop.

## Frame Timing

Two approaches for controlling animation speed:

### Manual Counter (Recommended)

A dedicated `u8` counter gives full control and costs almost nothing:

```c
u8 anim_timer = 0;

/* In the main loop, after WaitForVBlank() */
anim_timer++;
if (anim_timer >= 8) {
    anim_timer = 0;
    /* Advance to next animation frame */
}
```

This is the pattern used in `examples/graphics/sprites/dynamic_sprite/`. The counter increments once per VBlank, and every 8th frame the animation advances.

### Using getFrameCount()

The system `frame_count` (incremented by the NMI handler) is available via `getFrameCount()`. Useful for simple periodic animations where you do not need to pause or reset the timer independently:

```c
/* Cycle through 4 frames, changing every 8 VBlanks */
u8 frame = (getFrameCount() / 8) % 4;
oamSetTile(0, frame * 2);
```

The manual counter is preferred for gameplay animations because you can pause it (stop incrementing when the character is idle) or reset it on state changes.

## Sprite Sheet Layout

### VRAM Tile Numbering

For 16x16 sprites in 4bpp mode, each sprite is a 2x2 group of 8x8 tiles (32 bytes per 8x8 tile, 128 bytes per 16x16 frame). Tile numbers follow the SNES OBJ character layout:

```
VRAM row 0:  tile 0   tile 1   tile 2   tile 3  ...  tile 15
VRAM row 1:  tile 16  tile 17  tile 18  tile 19 ...  tile 31
VRAM row 2:  tile 32  tile 33  ...
```

A single 16x16 sprite uses a 2x2 block. Its tile number is the **top-left** 8x8 tile:

| 16x16 Frame | Tile Number | 8x8 tiles used |
|-------------|-------------|-----------------|
| Frame 0 | 0 | 0, 1, 16, 17 |
| Frame 1 | 2 | 2, 3, 18, 19 |
| Frame 2 | 4 | 4, 5, 20, 21 |

### Sprite Sheet Organization

A typical character sprite sheet is organized as rows of animation directions. For the animated sprite example (`examples/graphics/sprites/animated_sprite/`):

```
Row 0 (tiles 0-15):   Walk Down frames    | Walk Up frames    | Walk Right frames
Row 1 (tiles 16-31):  (lower halves)      | (lower halves)    | (lower halves)
Row 2 (tiles 32-47):  Walk Right frame 3  | ...
```

Tile numbers for each direction:
- **Down**: 0, 2, 4 (three frames, each 2 tiles apart)
- **Up**: 6, 8, 10
- **Right**: 12, 14, 32 (frame 3 wraps to next VRAM row)
- **Left**: same tiles as Right, drawn with H-flip

### Converting Sprite Sheets with gfx4snes

```bash
# 16x16 sprites, 4bpp, with palette output
gfx4snes -s 16 -o 16 -u 16 -p -i sprites.png
```

Include the output in an assembly data file:

```asm
.section ".rodata1" superfree
sprite_tiles: .incbin "res/sprites.pic"
sprite_tiles_end:
sprite_pal:   .incbin "res/sprites.pal"
sprite_pal_end:
.ends
```

## Direction-Based Animation

Most game characters have different frames for each movement direction. The common pattern uses an **enum for states** and **H-flip for left/right mirroring**.

From `examples/graphics/sprites/animated_sprite/`:

```c
#define FRAMES_PER_ANIMATION 3
#define ANIM_DELAY 6

enum SpriteState {
    W_DOWN  = 0,
    W_UP    = 1,
    W_RIGHT = 2,
    W_LEFT  = 2   /* Reuses W_RIGHT tiles with H-flip */
};

typedef struct {
    s16 x, y;
    u16 gfx_frame;
    u16 anim_frame;
    u16 anim_delay;
    u8 state;
    u8 flipx;
} Monster;

Monster monster = {100, 100, 0, 0, 0, W_DOWN, 0};
```

### Input and State Changes

```c
u16 pad0 = padHeld(0);

if (pad0 & KEY_UP) {
    monster.y--;
    monster.state = W_UP;
    monster.flipx = 0;
}
if (pad0 & KEY_LEFT) {
    monster.x--;
    monster.state = W_LEFT;
    monster.flipx = 1;   /* Mirror the RIGHT frames */
}
if (pad0 & KEY_RIGHT) {
    monster.x++;
    monster.state = W_RIGHT;
    monster.flipx = 0;
}
if (pad0 & KEY_DOWN) {
    monster.y++;
    monster.state = W_DOWN;
    monster.flipx = 0;
}
```

### Calculating the Tile Number

Map the current state and animation frame to a tile number, then apply the flip flag:

```c
/* Compute tile from state + frame */
if (monster.state == W_DOWN) {
    monster.gfx_frame = monster.anim_frame * 2;        /* 0, 2, 4 */
} else if (monster.state == W_UP) {
    monster.gfx_frame = 6 + monster.anim_frame * 2;    /* 6, 8, 10 */
} else {
    /* W_RIGHT / W_LEFT (same tiles, flip differs) */
    if (monster.anim_frame < 2)
        monster.gfx_frame = 12 + monster.anim_frame * 2;
    else
        monster.gfx_frame = 32;   /* Wraps to next tile row */
}

u16 flags = monster.flipx ? OBJ_FLIPX : 0;
oamSet(0, monster.x, monster.y, monster.gfx_frame, 0, 3, flags);
```

### Animation Timing with Idle Check

Only advance the animation counter while the character is moving:

```c
if (pad0 != 0) {
    monster.anim_delay++;
    if (monster.anim_delay >= ANIM_DELAY) {
        monster.anim_delay = 0;
        monster.anim_frame++;
        if (monster.anim_frame >= FRAMES_PER_ANIMATION)
            monster.anim_frame = 0;
    }
}
```

When no buttons are held, `anim_delay` stops incrementing and the sprite freezes on its current frame.

## The Dynamic Sprite Engine

For sprites with many animation frames, pre-loading all frames into VRAM wastes space. The **dynamic sprite engine** streams tile data from ROM to VRAM on demand, uploading only the current frame each time it changes.

### How It Works

1. All sprite frames live in ROM (the `.pic` file)
2. The engine maintains a VRAM upload queue
3. When `oamrefresh = 1`, the current frame's tiles are queued for DMA
4. `oamVramQueueUpdate()` performs the actual VRAM transfer during VBlank
5. Up to 7 sprite uploads per frame to stay within VBlank budget

### Initialization

```c
#include <snes.h>

extern u8 spr16_tiles[];
extern u8 spr16_pal[];

/* Initialize dynamic sprite engine via the struct-based API.
 *   vramLarge      = VRAM $0000 (large-tile pool)
 *   vramSmall      = VRAM $1000 (small-tile pool)
 *   slotLargeInit  = first OAM slot for the large pool
 *   slotSmallInit  = first OAM slot for the small pool
 *   sizeMode       = OBJ_SIZE8_L16 (8x8 small, 16x16 large)
 */
static const OamDynamicConfig dyn = {
    .vramLarge      = 0x0000,
    .vramSmall      = 0x1000,
    .slotLargeInit  = 0,
    .slotSmallInit  = 0,
    .sizeMode       = OBJ_SIZE8_L16,
};
oamDynamicInit(&dyn);

/* Load palette (sprite palettes start at CGRAM 128) */
dmaCopyCGram(spr16_pal, 128, 32);
```

### Setting Up a Dynamic Sprite

Use the `oambuffer[]` array (type `t_sprites`) instead of `oamSet()`:

```c
oambuffer[0].oamx = 100;
oambuffer[0].oamy = 80;
oambuffer[0].oamframeid = 0;                /* Frame index in sprite sheet */
oambuffer[0].oamattribute = OBJ_PRIO(3);    /* Priority 3 */
oambuffer[0].oamrefresh = 1;                /* Request VRAM upload */
OAM_SET_GFX(0, spr16_tiles);               /* Point to ROM tile data */
```

### The Game Loop

From `examples/graphics/sprites/dynamic_sprite/`:

```c
u8 frame_counter = 0;
u16 current_frame = 0;

while (1) {
    WaitForVBlank();

    /* Animate every 8th VBlank */
    frame_counter++;
    if (frame_counter >= 8) {
        frame_counter = 0;
        current_frame++;
        if (current_frame >= 24) current_frame = 0;

        oambuffer[0].oamframeid = current_frame;
        oambuffer[0].oamrefresh = 1;  /* Trigger VRAM upload */
    }

    /* Draw sprite (updates oambuffer + OAM, queues tile DMA).
     * The NMI handler auto-flushes the queue and hides last
     * frame's leftovers — no manual oamVramQueueUpdate /
     * oamInitDynamicSpriteEndFrame needed in the main loop. */
    oamDynamicDraw(0);
}
```

### Action-Based Animation (LikeMario)

The `examples/games/likemario/` example shows how to combine the dynamic sprite engine with action states. Each action maps to specific frame indices:

```c
#define FRAME_STAND  6
#define FRAME_JUMP   1
#define FRAME_WALK0  2
#define FRAME_WALK1  3

u8 mario_anim_idx = 0;
u8 anim_tick = 0;

void mario_animate(void) {
    anim_tick++;

    if (mario_action == ACT_WALK) {
        /* Toggle between walk frames every 4 VBlanks */
        if ((anim_tick & 3) == 3) {
            mario_anim_idx ^= 1;
            oambuffer[0].oamframeid = FRAME_WALK0 + mario_anim_idx;
            oambuffer[0].oamrefresh = 1;
        }
    } else if (mario_action == ACT_JUMP || mario_action == ACT_FALL) {
        if (oambuffer[0].oamframeid != FRAME_JUMP) {
            oambuffer[0].oamframeid = FRAME_JUMP;
            oambuffer[0].oamrefresh = 1;
        }
    } else {
        /* Standing — only update if frame changed */
        if (oambuffer[0].oamframeid != FRAME_STAND) {
            oambuffer[0].oamframeid = FRAME_STAND;
            oambuffer[0].oamrefresh = 1;
        }
    }
}
```

Key details:
- `oamrefresh` is only set to 1 when the frame actually changes, avoiding redundant VRAM uploads
- Direction is handled separately via `oamattribute` (setting or clearing the H-flip bit `0x40`)
- `oamDynamicDraw()` reads `oambuffer[].oamx` and `oambuffer[].oamy` for positioning

### One Draw Function, Engine-Resolved Size

`oamDynamicDraw(id)` looks up the sprite's pixel size from the size pair
set at init plus an optional per-sprite override (`oamDynamicSetSize`),
then dispatches to the matching internal routine. Callers no longer
pick a function by sprite size — the engine knows.

For metasprite groups, `oamMetaDrawDyn(id, x, y, meta, gfx, size_class)`
walks a `MetaspriteItem` array and dispatches each sub-sprite the same
way; pass `OBJ_LARGE` or `OBJ_SMALL` to select which half of the
configured size pair to use.

## Background Tile Animation

Backgrounds can be animated by cycling tilemap entries. This is useful for water, lava, torches, and other environmental effects.

### Approach 1: Swap Tilemap Entries

Write new tile numbers into the tilemap at fixed intervals. This changes which tiles appear without modifying tile graphics:

```c
#define WATER_TILE_A  20
#define WATER_TILE_B  21
#define WATER_ANIM_SPEED 16

u8 water_frame = 0;
u8 water_timer = 0;

void animate_water_tiles(void) {
    u16 tile;

    water_timer++;
    if (water_timer < WATER_ANIM_SPEED) return;
    water_timer = 0;

    water_frame ^= 1;
    tile = water_frame ? WATER_TILE_B : WATER_TILE_A;

    /* Write during VBlank or forced blank only */
    REG_VMAIN = 0x80;

    /* Update each water tile position in the tilemap */
    REG_VMADDL = (VRAM_MAP + water_col + water_row * 32) & 0xFF;
    REG_VMADDH = (VRAM_MAP + water_col + water_row * 32) >> 8;
    REG_VMDATAL = tile & 0xFF;
    REG_VMDATAH = tile >> 8;
}
```

### Approach 2: Overwrite Tile Graphics

Instead of changing the tilemap, DMA new pixel data into the same tile slot. Every tile referencing that slot updates simultaneously:

```c
extern u8 water_frame0[];  /* 32 bytes (one 4bpp 8x8 tile) */
extern u8 water_frame1[];

void animate_water_gfx(void) {
    u8 *src;

    water_timer++;
    if (water_timer < WATER_ANIM_SPEED) return;
    water_timer = 0;
    water_frame ^= 1;

    src = water_frame ? water_frame1 : water_frame0;

    /* DMA 32 bytes to the water tile's VRAM address (VBlank only) */
    REG_VMAIN = 0x80;
    REG_VMADDL = (WATER_TILE_VRAM) & 0xFF;
    REG_VMADDH = (WATER_TILE_VRAM) >> 8;
    dmaCopyVram(src, WATER_TILE_VRAM, 32);
}
```

This approach is more efficient when many tilemap positions use the same animated tile, since you update the graphics once rather than rewriting every tilemap entry.

### VBlank Budget

Background tile animation involves VRAM writes, which **must happen during VBlank or forced blank**. Keep animated tile DMA small (under 1 KB per frame) to stay within the VBlank budget alongside sprite updates and scroll register writes.

## Performance Considerations

1. **oamSet() is expensive** (framesize=158). For more than 2-3 sprites per frame, use `oamSetFast()` or write directly to `oamMemory[]`
2. **Only set `oamrefresh = 1` when the frame changes** -- redundant VRAM uploads waste VBlank time
3. **The dynamic engine uploads up to 7 sprites per frame**. If you need more animated sprites, spread their refresh across multiple frames
4. **BG tile animation competes with sprite DMA for VBlank time**. Budget carefully: ~4 KB total per VBlank

## Example References

- `examples/graphics/sprites/animated_sprite/` -- basic 4-direction sprite animation with H-flip
- `examples/graphics/sprites/dynamic_sprite/` -- dynamic sprite engine with VRAM streaming
- `examples/games/likemario/` -- action-state animation (walk, jump, stand) with camera and physics

## Next Steps

- @ref tutorial_sprites "Sprite Basics"
- @ref tutorial_graphics "Graphics & Backgrounds"
- @ref sprite.h "Sprite API Reference"
