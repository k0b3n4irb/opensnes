# Dynamic Sprite Example

Automatic VRAM management for animated sprites.

## Learning Objectives

After this lesson, you will understand:
- Dynamic sprite system concepts
- Automatic VRAM queue management
- Per-sprite frame animation
- Efficient sprite tile updates

## Prerequisites

- Completed metasprite example
- Understanding of VRAM organization

---

## What This Example Does

Displays four animated 16x16 sprites:
- Each sprite animates independently
- VRAM tiles update automatically each frame
- Demonstrates dynamic sprite system

```
+----------------------------------------+
|                                        |
|    [SPRITE 1]        [SPRITE 2]        |
|    Frame 1           Frame 3           |
|                                        |
|    [SPRITE 3]        [SPRITE 4]        |
|    Frame 2           Frame 4           |
|                                        |
+----------------------------------------+
```

**Controls:** None (automatic animation)

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Dynamic sprite init | Library (`oamInitDynamicSprite`) |
| Sprite drawing | Library (`oamDynamic16Draw`) |
| VRAM updates | Library (`oamVramQueueUpdate`) |
| Frame management | Library (`oamInitDynamicSpriteEndFrame`) |

---

## Dynamic Sprite System

### The Problem

Traditional sprite systems:
- Load all animation frames to VRAM upfront
- Limited by VRAM size
- Wastes space on unused frames

### The Solution

Dynamic sprites:
- Only current frame tiles in VRAM
- System queues tile updates
- Uploads during VBlank
- More efficient VRAM usage

---

## System Architecture

```
ROM                    RAM                 VRAM
+------------+        +--------+          +--------+
| All Frames | -----> | Queue  | -------> | Active |
| (Full Set) |  DMA   | Buffer |  VBlank  | Tiles  |
+------------+        +--------+          +--------+
```

---

## Using Dynamic Sprites

### Initialization

```c
/* Initialize dynamic sprite system */
oamInitDynamicSprite(OBJ_SIZE16_L32);

/* Clear OAM */
oamClear(0, 0);
```

### Drawing Sprites

```c
/* Each frame, specify which graphics to use */
oamDynamic16Draw(0,           /* OAM slot */
                 x, y,        /* Position */
                 sprite_gfx,  /* Graphics pointer */
                 frame * 128, /* Offset in graphics */
                 0, 0,        /* Palette, flip */
                 3);          /* Priority */
```

### Frame Update

```c
while (1) {
    /* Update animation frames */
    for (u8 i = 0; i < 4; i++) {
        sprite_frame[i]++;
        if (sprite_frame[i] >= MAX_FRAMES) {
            sprite_frame[i] = 0;
        }
    }

    /* Draw all sprites */
    for (u8 i = 0; i < 4; i++) {
        oamDynamic16Draw(i, sprites[i].x, sprites[i].y,
                         sprite_gfx, sprite_frame[i] * 128,
                         0, 0, 3);
    }

    WaitForVBlank();

    /* Process VRAM update queue */
    oamVramQueueUpdate();

    /* End frame housekeeping */
    oamInitDynamicSpriteEndFrame();
}
```

---

## Build and Run

```bash
cd examples/graphics/15_dynamic_sprite
make clean && make

# Run in emulator
/path/to/Mesen dynamic_sprite.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Dynamic sprite demo |
| `data.asm` | Animation frame graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Different Animation Speeds

Give each sprite different timing:
```c
sprite_timer[i]++;
if (sprite_timer[i] >= sprite_speed[i]) {
    sprite_timer[i] = 0;
    sprite_frame[i]++;
}
```

### Exercise 2: Many Sprites

Test limits with more sprites:
```c
#define NUM_SPRITES 16
/* Monitor VRAM queue to avoid overflow */
```

### Exercise 3: Mixed Static/Dynamic

Combine with traditional static sprites for UI elements.

### Exercise 4: On-Demand Loading

Only update sprites that are visible:
```c
if (sprite_on_screen(i)) {
    oamDynamic16Draw(i, ...);
}
```

---

## Technical Notes

### VRAM Budget

Dynamic sprites need VRAM slots:
- Each 16x16 sprite: 128 bytes
- System manages allocation automatically
- Monitor usage to avoid overflow

### Queue Size

The update queue has limits:
- Too many updates = dropped frames
- Prioritize visible/important sprites
- Consider frame skipping for far objects

### When to Use Dynamic Sprites

**Good for:**
- Many unique animations
- Large sprite sheets
- Characters with many states

**Not needed for:**
- Few animation frames
- Static sprites
- Simple games with small sprite sets

### Performance

Dynamic sprites trade:
- **CPU time** (queue management) for
- **VRAM space** (only active frames)

Profile to ensure VBlank budget isn't exceeded.

---

## What's Next?

**HDMA:** [HDMA Gradient](../16_hdma_gradient/) - Per-scanline effects

**Color Math:** [Transparency](../17_transparency/) - Blending effects

---

## License

Code: MIT
