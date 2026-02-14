# Animation System Example

Complete sprite animation framework with state machines.

## Learning Objectives

After this lesson, you will understand:
- Animation state machines
- Frame-based animation with timing
- Looping vs one-shot animations
- Animation callbacks and events

## Prerequisites

- Completed sprite examples
- Understanding of game loops and timing

---

## What This Example Does

Demonstrates a full animation system:
- Multiple animation states (idle, walk, jump)
- Frame timing with configurable delays
- State transitions based on input
- Animation callbacks for events

```
+----------------------------------------+
|                                        |
|   Animation: WALK                      |
|   Frame: 2/4                           |
|                                        |
|              [SPRITE]                  |
|              Walking...                |
|                                        |
|   D-Pad: Move  A: Jump  B: Toggle      |
+----------------------------------------+
```

**Controls:**
- D-Pad: Move sprite (triggers walk animation)
- A button: Jump (triggers jump animation)
- B button: Toggle walk/idle
- Start: Pause/resume animation

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Animation engine | Library (`animInit`, `animPlay`, etc.) |
| Sprite display | Library (`oamSet`, `oamUpdate`) |
| State management | C code |
| Input handling | Direct register access |
| Text display | Direct VRAM writes |

---

## Animation System Architecture

### Animation Data Structure

```c
typedef struct {
    u8 *frames;      /* Array of tile numbers */
    u8 frame_count;  /* Number of frames */
    u8 *delays;      /* Delay per frame (in game ticks) */
    u8 loop;         /* 1 = loop, 0 = play once */
} Animation;
```

### Example Animation Definition

```c
/* Walk animation: 4 frames, looping */
static const u8 walk_frames[] = {0, 2, 4, 2};
static const u8 walk_delays[] = {8, 8, 8, 8};

Animation walk_anim = {
    walk_frames, 4, walk_delays, 1
};
```

---

## Animation Playback

### Starting an Animation

```c
/* Play walk animation on sprite 0 */
animSetAnim(0, &walk_anim);
animPlay(0);
```

### Animation Update Loop

```c
while (1) {
    WaitForVBlank();

    /* Update all animations */
    animUpdate();

    /* Get current frame for display */
    u8 tile = animGetFrame(0);
    oamSet(0, x, y, tile, 0, 3, 0);
    oamUpdate();
}
```

---

## State Machine

### Animation States

```c
enum AnimState {
    STATE_IDLE,
    STATE_WALK,
    STATE_JUMP,
    STATE_FALL
};

static u8 current_state = STATE_IDLE;
```

### State Transitions

```c
void updateState(u16 pad) {
    switch (current_state) {
        case STATE_IDLE:
            if (pad & (KEY_LEFT | KEY_RIGHT)) {
                current_state = STATE_WALK;
                animSetAnim(0, &walk_anim);
            }
            if (pad & KEY_A) {
                current_state = STATE_JUMP;
                animSetAnim(0, &jump_anim);
            }
            break;

        case STATE_WALK:
            if (!(pad & (KEY_LEFT | KEY_RIGHT))) {
                current_state = STATE_IDLE;
                animSetAnim(0, &idle_anim);
            }
            break;

        case STATE_JUMP:
            if (animGetState(0) == ANIM_FINISHED) {
                current_state = STATE_FALL;
                animSetAnim(0, &fall_anim);
            }
            break;
    }
}
```

---

## Build and Run

```bash
cd examples/graphics/animation_system
make clean && make

# Run in emulator
/path/to/Mesen animation_system.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Game loop, state machine, animation control |
| `data.asm` | Sprite frames and animation data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Attack Animation

Add an attack animation triggered by Y button:
```c
if (pressed & KEY_Y) {
    current_state = STATE_ATTACK;
    animSetAnim(0, &attack_anim);
}
```

### Exercise 2: Animation Speed

Vary animation speed based on movement:
```c
if (running) {
    animSetSpeed(0, 4);  /* Faster */
} else {
    animSetSpeed(0, 8);  /* Normal */
}
```

### Exercise 3: Direction Flip

Flip sprite based on movement direction:
```c
u8 flip = facing_left ? 0x40 : 0x00;
oamSet(0, x, y, tile, 0, 3, flip);
```

### Exercise 4: Multiple Characters

Animate several sprites independently:
```c
for (u8 i = 0; i < NUM_CHARS; i++) {
    u8 tile = animGetFrame(i);
    oamSet(i, chars[i].x, chars[i].y, tile, 0, 3, 0);
}
```

---

## Technical Notes

### Frame Timing

Animation delays are in game frames (60fps):
- Delay 4: 15 FPS animation
- Delay 8: 7.5 FPS animation
- Delay 15: 4 FPS animation

### One-Shot Animations

For non-looping animations:
```c
if (animGetState(0) == ANIM_FINISHED) {
    /* Animation complete, transition to next state */
}
```

### Animation Callbacks

For events at specific frames:
```c
/* In animation update */
if (current_frame == attack_frame) {
    checkHitboxCollision();
}
```

### Memory Considerations

- Keep animation data in ROM
- Only runtime state in RAM
- Share frame data between similar animations

---

## What's Next?

**Metasprites:** [Metasprite](../metasprite/) - Large multi-tile sprites

**Dynamic Sprites:** [Dynamic Sprite](../dynamic_sprite/) - VRAM management

---

## License

Code: MIT
