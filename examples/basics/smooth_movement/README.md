# Smooth Movement Example

Fixed-point math for sub-pixel positioning and smooth physics.

## Learning Objectives

After this lesson, you will understand:
- Fixed-point number representation (8.8 format)
- Acceleration and friction-based movement
- Sine/cosine for circular motion
- Sub-pixel smooth animation

## Prerequisites

- Completed collision demo
- Basic understanding of physics concepts

---

## What This Example Does

Demonstrates smooth movement techniques:
- Player sprite with acceleration and friction
- Orbiting trail effect using trigonometry
- Three orbital speed modes
- Sub-pixel precise positioning

```
+----------------------------------------+
|                                        |
|         Orbital Speed: NORMAL          |
|                                        |
|              *   *                     |
|           *         *                  |
|          *  [PLAYER]  *                |
|           *         *                  |
|              *   *                     |
|                                        |
|   D-Pad: Move    A: Cycle Speed        |
+----------------------------------------+
```

**Controls:**
- D-Pad: Move player with smooth acceleration
- A button: Cycle orbital speed (Slow/Normal/Fast/Faster)

---

## Code Type

**C with Library Math Functions**

| Component | Type |
|-----------|------|
| Fixed-point math | Library (`fixMul`, `fixClamp`) |
| Trigonometry | Library (`fixSin`, `fixCos`) |
| Sprite display | Library (`oamSet`, `oamUpdate`) |
| Input handling | Direct register access |

---

## Fixed-Point Math

### What is Fixed-Point?

Instead of floating-point (slow on SNES), we use fixed-point:
- Integer with implied decimal point
- 8.8 format: 8 bits integer, 8 bits fraction
- 256 = 1.0, 128 = 0.5, 64 = 0.25

### Fixed-Point Operations

```c
/* 8.8 fixed point */
typedef s16 fixed8;

#define INT_TO_FIX(x) ((x) << 8)
#define FIX_TO_INT(x) ((x) >> 8)

/* Multiply two fixed-point values */
fixed8 fixMul(fixed8 a, fixed8 b) {
    return (s32)a * b >> 8;
}
```

---

## Velocity-Based Movement

### Acceleration and Friction

```c
#define ACCEL      16   /* Acceleration per frame (0.0625) */
#define FRICTION   12   /* Friction per frame */
#define MAX_SPEED 512   /* Maximum velocity (2.0) */

static fixed8 vel_x = 0;
static fixed8 vel_y = 0;

void updateMovement(u16 pad) {
    /* Apply acceleration from input */
    if (pad & KEY_RIGHT) vel_x += ACCEL;
    if (pad & KEY_LEFT) vel_x -= ACCEL;
    if (pad & KEY_DOWN) vel_y += ACCEL;
    if (pad & KEY_UP) vel_y -= ACCEL;

    /* Apply friction (slow down when not pressing) */
    if (!(pad & KEY_LEFT) && !(pad & KEY_RIGHT)) {
        if (vel_x > 0) vel_x -= FRICTION;
        if (vel_x < 0) vel_x += FRICTION;
        if (vel_x < FRICTION && vel_x > -FRICTION) vel_x = 0;
    }

    /* Clamp to max speed */
    vel_x = fixClamp(vel_x, -MAX_SPEED, MAX_SPEED);
    vel_y = fixClamp(vel_y, -MAX_SPEED, MAX_SPEED);

    /* Apply velocity to position */
    pos_x += vel_x;
    pos_y += vel_y;
}
```

---

## Orbital Motion

### Sine/Cosine Movement

```c
static u16 orbit_angle = 0;
static fixed8 orbit_radius = INT_TO_FIX(32);

void updateOrbit(void) {
    orbit_angle += orbit_speed;  /* Rotate */

    /* Calculate orbital position */
    fixed8 orbit_x = fixMul(orbit_radius, fixCos(orbit_angle));
    fixed8 orbit_y = fixMul(orbit_radius, fixSin(orbit_angle));

    /* Position relative to player */
    s16 trail_x = FIX_TO_INT(pos_x + orbit_x);
    s16 trail_y = FIX_TO_INT(pos_y + orbit_y);
}
```

### Trail Effect

```c
#define TRAIL_COUNT 8

static u16 trail_angles[TRAIL_COUNT];

void updateTrail(void) {
    for (u8 i = 0; i < TRAIL_COUNT; i++) {
        /* Each trail sprite at different angle */
        u16 angle = orbit_angle + i * (65536 / TRAIL_COUNT);

        s16 x = FIX_TO_INT(pos_x) + (fixCos(angle) >> 3);
        s16 y = FIX_TO_INT(pos_y) + (fixSin(angle) >> 3);

        oamSet(i + 1, x, y, 0, 0, 2, 0);
    }
}
```

---

## Build and Run

```bash
cd examples/basics/smooth_movement
make clean && make

# Run in emulator
/path/to/Mesen smooth_movement.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Movement physics and orbit calculations |
| `data.asm` | Sprite graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Bouncing

Add wall bouncing:
```c
if (FIX_TO_INT(pos_x) < 0 || FIX_TO_INT(pos_x) > 240) {
    vel_x = -vel_x;  /* Reverse X velocity */
}
```

### Exercise 2: Gravity

Add downward acceleration:
```c
#define GRAVITY 8

vel_y += GRAVITY;  /* Always accelerate down */
```

### Exercise 3: Variable Friction

Different friction on different surfaces:
```c
u8 surface = getTileFriction(pos_x, pos_y);
switch (surface) {
    case TILE_ICE: friction = 2; break;
    case TILE_GRASS: friction = 16; break;
    case TILE_MUD: friction = 24; break;
}
```

### Exercise 4: Orbit Wobble

Make the orbit radius pulse:
```c
static u16 wobble = 0;
wobble += 4;
orbit_radius = INT_TO_FIX(32) + (fixSin(wobble) >> 4);
```

---

## Technical Notes

### Fixed-Point Precision

8.8 format:
- Range: -128.0 to 127.996
- Precision: 1/256 = 0.00390625
- Good for screen coordinates

16.16 format for more precision:
```c
typedef s32 fixed16;
#define INT_TO_FIX16(x) ((x) << 16)
```

### Sine Table

`fixSin()` uses a lookup table:
- 256 or 512 entries
- Full circle = 65536 angle units
- Returns 8.8 fixed-point (-256 to 256)

### Screen Boundary Clamping

```c
/* Clamp position to screen */
pos_x = fixClamp(pos_x, 0, INT_TO_FIX(240));
pos_y = fixClamp(pos_y, 0, INT_TO_FIX(208));
```

### Movement Feel

Tune these values for different feel:
- High ACCEL, low FRICTION = responsive, slidey
- Low ACCEL, high FRICTION = slow, precise
- Balance for your game type

---

## What's Next?

**Game:** [Breakout](../../game/breakout/) - Complete game

**Graphics:** Return to [graphics examples](../../graphics/)

---

## License

Code: MIT
