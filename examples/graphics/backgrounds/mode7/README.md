# Mode 7 Example

Hardware-accelerated rotation and scaling effects using SNES Mode 7.

## Learning Objectives

After this lesson, you will understand:
- How Mode 7 provides 2D rotation and scaling
- The Mode 7 transformation matrix
- Setting rotation angles and scale factors
- Creating perspective and 3D-like effects

## Prerequisites

- Completed basic background examples
- Understanding of tile-based graphics

---

## What This Example Does

Displays a tiled floor pattern with interactive rotation and zoom controls:
- The entire background rotates around a center point
- Scale can be adjusted to zoom in and out
- Demonstrates the famous Mode 7 effect used in F-Zero, Mario Kart, and Pilotwings

```
+----------------------------------------+
|                                        |
|      +-------------------------+       |
|     /                           \      |
|    /     ROTATING FLOOR          \     |
|   /                               \    |
|  +                                 +   |
|   \                               /    |
|    \                             /     |
|     \                           /      |
|      +-------------------------+       |
+----------------------------------------+
```

**Controls:**
- A button: Rotate clockwise
- B button: Rotate counter-clockwise
- D-Pad Up: Zoom in (increase scale)
- D-Pad Down: Zoom out (decrease scale)

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Mode 7 init | Library (`mode7Init()`) |
| Rotation | Library (`mode7SetAngle()`) |
| Scaling | Library (`mode7SetScale()`) |
| Graphics loading | Library (DMA functions) |
| Input handling | Direct register access |

---

## Mode 7 Fundamentals

### What is Mode 7?

Mode 7 is a special SNES video mode that applies a 2D affine transformation to a single background layer:
- Rotation around any point
- Scaling (zoom in/out)
- Shearing
- Any combination of the above

The transformation is defined by a 2x2 matrix plus translation.

### The Transformation Matrix

```
| A  B |   Rotation + Scale X
| C  D |   Rotation + Scale Y

For pure rotation by angle θ with scale S:
A = S * cos(θ)
B = S * sin(θ)
C = -S * sin(θ)
D = S * cos(θ)
```

### Mode 7 Registers

| Register | Address | Purpose |
|----------|---------|---------|
| M7SEL | $211A | Mode 7 settings (flip, wrap) |
| M7A | $211B | Matrix A (cosine term) |
| M7B | $211C | Matrix B (sine term) |
| M7C | $211D | Matrix C (-sine term) |
| M7D | $211E | Matrix D (cosine term) |
| M7X | $211F | Rotation center X |
| M7Y | $2120 | Rotation center Y |
| M7HOFS | $210D | Horizontal scroll |
| M7VOFS | $210E | Vertical scroll |

---

## Basic Mode 7 Setup

### Initialization

```c
/* Initialize Mode 7 with default settings */
mode7Init();

/* Set rotation center to screen center */
mode7SetPivot(128, 112);

/* Set initial scale (1.0 = 256 in fixed point) */
mode7SetScale(256, 256);

/* Set initial angle (0 degrees) */
mode7SetAngle(0);
```

### Rotation Control

```c
static u16 angle = 0;

/* Rotate based on input */
if (pad & KEY_A) {
    angle += 2;  /* Clockwise */
}
if (pad & KEY_B) {
    angle -= 2;  /* Counter-clockwise */
}

mode7SetAngle(angle);
```

### Scale Control

```c
static u16 scale = 256;  /* 1.0 in 8.8 fixed point */

if (pad & KEY_UP) {
    if (scale < 512) scale += 4;  /* Zoom in */
}
if (pad & KEY_DOWN) {
    if (scale > 64) scale -= 4;   /* Zoom out */
}

mode7SetScale(scale, scale);
```

---

## Mode 7 Limitations

### Single Layer Only

Mode 7 only affects BG1. Other layers are disabled. However:
- Sprites work normally
- You can use HDMA for per-scanline effects

### Tile Format

Mode 7 uses a unique 8bpp format:
- 256 colors per tile
- Tiles are 8x8 pixels
- Maximum 256 tiles
- Tilemap is 128x128 tiles (1024x1024 pixels)

### No Priority

All Mode 7 pixels have the same priority. Sprites can appear above or below the entire layer based on their priority setting.

---

## Build and Run

```bash
cd examples/graphics/mode7
make clean && make

# Run in emulator
/path/to/Mesen mode7.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Mode 7 setup and input handling |
| `data.asm` | Mode 7 graphics data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Auto-Rotation

Make the floor rotate automatically:
```c
angle += 1;  /* Rotate every frame */
mode7SetAngle(angle);
```

### Exercise 2: Perspective Effect

Use HDMA to change the scale per scanline for a perspective effect:
```c
/* Create a scale table that increases toward bottom of screen */
/* Smaller scale = further away (top), larger = closer (bottom) */
```

### Exercise 3: Racing Game View

Combine rotation with scroll to create a racing game perspective:
```c
/* Scroll forward while rotating for steering */
mode7SetScroll(scroll_x, scroll_y);
scroll_y -= speed;  /* Move forward */
```

### Exercise 4: Oscillating Zoom

Make the zoom pulse in and out:
```c
static u8 zoom_phase = 0;
zoom_phase += 2;
u16 scale = 256 + (fixSin(zoom_phase) >> 2);
mode7SetScale(scale, scale);
```

---

## Technical Notes

### Fixed Point Math

Mode 7 parameters use 8.8 fixed point:
- `256` = 1.0
- `512` = 2.0 (2x zoom)
- `128` = 0.5 (half size)

### Rotation Center

The pivot point (`mode7SetPivot()`) determines the center of rotation. Setting it to screen center (128, 112) rotates around the middle.

### Wrapping vs Clipping

M7SEL register controls edge behavior:
- Wrap: Tilemap repeats infinitely
- Clip: Pixels outside tilemap show backdrop color

---

## What's Next?

**Scrolling:** [Scrolling Example](../scrolling/) - Parallax backgrounds

**Advanced Mode 7:** Combine with HDMA for pseudo-3D effects

---

## License

Code: MIT
