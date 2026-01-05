# Lesson 2: Animation

Animate a sprite with multiple frames and timing control.

## Learning Objectives

After this lesson, you will understand:
- How sprite animation works on SNES
- Frame-based timing systems
- Tile swapping for animation
- Animation state machines
- Horizontal sprite flipping

## Prerequisites

- Completed [1. Sprite](../1_sprite/)
- Understanding of OAM and tile loading

---

## Animation Fundamentals

### The Illusion of Motion

Animation is simply showing different images in sequence:

```
Frame 0     Frame 1     Frame 2     Frame 3     Frame 0...
┌────┐      ┌────┐      ┌────┐      ┌────┐      ┌────┐
│ o  │  →   │ o  │  →   │ o  │  →   │ o  │  →   │ o  │
│/|\ │      │/|\ │      │\|/ │      │/|\ │      │/|\ │
│/ \ │      │ | │      │ | │      │ |\ │      │/ \ │
└────┘      └────┘      └────┘      └────┘      └────┘
Standing    Walk L      Squat       Walk R      Standing
```

At 60fps, cycling through 4 frames every 8 VBlanks = 7.5 animation cycles/second.

### Sprite Sheet Organization

Store all frames in one image, convert once:

```
Source PNG (64×16):
┌────────────────────────────────────────────────────────────────┐
│ Frame 0  │ Frame 1  │ Frame 2  │ Frame 3  │
│ Stand    │ Walk 1   │ Squat    │ Walk 2   │
└────────────────────────────────────────────────────────────────┘
```

In VRAM (128 pixels wide, tiles numbered):
```
┌────┬────┬────┬────┬────┬────┬────┬────┬────┐...
│ T0 │ T1 │ T2 │ T3 │ T4 │ T5 │ T6 │ T7 │    │
├────┼────┼────┼────┼────┼────┼────┼────┼────┤
│T16 │T17 │T18 │T19 │T20 │T21 │T22 │T23 │    │
└────┴────┴────┴────┴────┴────┴────┴────┴────┘

Frame 0: tiles 0-1, 16-17  (tile index = 0)
Frame 1: tiles 2-3, 18-19  (tile index = 2)
Frame 2: tiles 4-5, 20-21  (tile index = 4)
Frame 3: tiles 6-7, 22-23  (tile index = 6)
```

---

## Frame Timing

### VBlank as Your Clock

The SNES runs at ~60fps (NTSC) or ~50fps (PAL). Each VBlank = 1 frame.

```c
#define ANIM_SPEED_WALK   8    /* Change frame every 8 VBlanks */
#define ANIM_SPEED_IDLE   15   /* Slower when standing still */

static u8 anim_timer;

void update_animation(void) {
    anim_timer--;

    if (anim_timer == 0) {
        /* Advance to next frame */
        anim_frame++;
        if (anim_frame >= NUM_FRAMES) {
            anim_frame = 0;
        }

        /* Reset timer */
        anim_timer = ANIM_SPEED_WALK;
    }
}
```

### Speed Examples

| Timer Value | FPS | Frames/Cycle | Animation Speed |
|-------------|-----|--------------|-----------------|
| 4 | 60 | 15/sec | Very fast |
| 8 | 60 | 7.5/sec | Walk speed |
| 15 | 60 | 4/sec | Idle bob |
| 30 | 60 | 2/sec | Very slow |

---

## Tile Swapping

### How It Works

You don't re-upload tile data. Instead, change the tile number in OAM:

```c
/* Get tile number for 16×16 sprite in horizontal strip */
u8 get_tile_for_frame(u8 frame) {
    return frame * 2;  /* 0, 2, 4, 6 */
}

/* In main loop: */
u8 tile = get_tile_for_frame(anim_frame);
oam_set(0, sprite_x, sprite_y, tile, attr);
```

### Why frame × 2?

A 16×16 sprite occupies 2 tiles horizontally:
```
Frame 0:  T0, T1      → tile index 0
Frame 1:  T2, T3      → tile index 2
Frame 2:  T4, T5      → tile index 4
Frame 3:  T6, T7      → tile index 6
```

---

## Horizontal Flipping

### Mirroring Sprites

Instead of storing separate left/right graphics, flip the sprite:

```c
u8 attr = 0x30;  /* Priority 3 */

if (facing_left) {
    attr |= 0x40;  /* Set H-flip bit */
}

oam_set(0, sprite_x, sprite_y, tile, attr);
```

### OAM Attribute Bits

```
Byte 3 of OAM entry:
  Bit 7: Vertical flip   (v)
  Bit 6: Horizontal flip (h)
  Bit 5-4: Priority      (pp)
  Bit 3-1: Palette       (ccc)
  Bit 0: Tile high bit   (t)

  vhppccct
  0100 0000 = 0x40 = H-flip only
  1000 0000 = 0x80 = V-flip only
  1100 0000 = 0xC0 = Both flips
```

---

## Animation State Machine

### Managing Multiple States

```c
typedef enum {
    STATE_IDLE,
    STATE_WALKING,
    STATE_JUMPING
} PlayerState;

static PlayerState current_state;
static u8 anim_frame;
static u8 anim_timer;

void update_animation(void) {
    anim_timer--;
    if (anim_timer == 0) {
        switch (current_state) {
            case STATE_IDLE:
                /* Single frame or breathing animation */
                anim_frame = 0;
                anim_timer = 30;
                break;

            case STATE_WALKING:
                /* Cycle through walk frames */
                anim_frame = (anim_frame + 1) % 4;
                anim_timer = 8;
                break;

            case STATE_JUMPING:
                /* Hold jump frame */
                anim_frame = 4;
                anim_timer = 1;
                break;
        }
    }
}
```

---

## The Code Explained

### Animation Variables

```c
static u8 anim_frame;      /* Current frame index (0-3) */
static u8 anim_timer;      /* Countdown to next frame */
static u8 is_moving;       /* Was there input this frame? */
static u8 facing_left;     /* Direction for H-flip */
```

### Main Loop Flow

```c
while (1) {
    wait_vblank();

    /* 1. Read input */
    joy = read_joypad();

    /* 2. Update position & track if moving */
    is_moving = 0;
    if (joy & KEY_LEFT) {
        sprite_x--;
        is_moving = 1;
        facing_left = 1;
    }
    /* ... other directions ... */

    /* 3. Update animation state */
    update_animation();

    /* 4. Calculate tile and attributes */
    tile = get_tile_for_frame(anim_frame);
    attr = facing_left ? 0x70 : 0x30;  /* H-flip if left */

    /* 5. Update OAM */
    oam_set(0, sprite_x, sprite_y, tile, attr);
    oam_update();
}
```

---

## Build and Run

```bash
cd examples/graphics/2_animation
make clean && make

# Test with emulator
/path/to/Mesen animation.sfc
```

**Controls:**
- D-pad: Move character (triggers walk animation)
- Stand still: Returns to idle pose

---

## Exercises

### Exercise 1: Speed Control

Make the animation speed up when running (holding a button):

```c
#define KEY_B 0x8000  /* B button */

if (joy & KEY_B) {
    anim_timer = ANIM_SPEED_WALK / 2;  /* Double speed */
}
```

### Exercise 2: Jump Animation

Add a jump when pressing A:
1. Set a `jumping` flag
2. Apply upward velocity
3. Display a jump frame
4. Apply gravity until landing

### Exercise 3: Create Your Own Animation

1. Edit `assets/spritesheet.png` with new frames
2. Keep the 64×16 layout (4 frames × 16×16)
3. Rebuild and test

### Exercise 4: Two-Direction Walk

Currently all frames are used for all directions. Try:
- Frames 0-1: Walk right
- Frames 2-3: Walk left
- Modify `get_tile_for_frame()` to use different frames

---

## Common Issues

### Animation doesn't change
- Check `anim_timer` is being decremented
- Verify `anim_frame` changes are reaching `oam_set()`
- Ensure `oam_update()` is called after changes

### Sprite flickers between frames
- Make sure you're only calling `oam_update()` during VBlank
- Don't update OAM mid-frame

### Wrong tiles displayed
- Verify tile indices match your sprite sheet layout
- Remember: 16×16 sprites use tile index × 2

### Flipping doesn't work
- Check attribute byte format: `0x40` = H-flip
- Ensure you're not overwriting the flip bit

---

## Key Concepts Summary

| Concept | Implementation |
|---------|----------------|
| Timing | Count VBlanks, act when timer hits 0 |
| Tile swap | Change tile number in OAM, not VRAM |
| Sprite sheet | All frames in one image → VRAM |
| H-flip | Attribute bit 6 mirrors sprite |
| State machine | Track state, choose frames accordingly |

---

## Files in This Example

| File | Purpose |
|------|---------|
| `main.c` | Animation logic (timing, state, flip) |
| `assets/spritesheet.png` | 4-frame animation strip |
| `spritesheet.h` | Generated tile data |
| `Makefile` | Build + asset conversion |

---

## What's Next?

Now you can animate sprites! Next lessons will cover:
- **Backgrounds:** Tilemaps, scrolling, parallax
- **Collision:** Detecting sprite overlaps
- **Multiple sprites:** Managing many animated objects

**Next:** 3. Background *(coming soon)* →
