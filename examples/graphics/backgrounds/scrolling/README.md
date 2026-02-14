# Scrolling Example

Two-layer parallax scrolling with depth effect.

## Learning Objectives

After this lesson, you will understand:
- How background scrolling works on the SNES
- Creating parallax depth with different scroll speeds
- Manual vs automatic scrolling
- Using multiple background layers

## Prerequisites

- Completed Mode 1 example
- Understanding of tilemaps and VRAM layout

---

## What This Example Does

Displays two background layers that scroll at different speeds:
- Foreground (BG1) scrolls at full speed
- Background (BG2) scrolls at half speed
- Creates an illusion of depth and distance

```
+----------------------------------------+
|  ~~ CLOUDS ~~ (slow)     BG2           |
|                                        |
|  ████ MOUNTAINS ████ (medium)          |
|                                        |
|  ▓▓▓▓ GROUND ▓▓▓▓ (fast)    BG1       |
+----------------------------------------+
       ←←←← Scroll Direction ←←←←
```

**Controls:**
- D-Pad Left/Right: Scroll manually
- D-Pad Up/Down: Scroll vertically
- A button: Toggle auto-scroll mode

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| Background setup | Library (`bgSetMapPtr`, `bgInitTileSet`) |
| Scroll control | Library (`bgSetScroll`) |
| Graphics loading | Library (`dmaCopyVram`) |
| Input handling | Direct register access |
| VBlank sync | Library (`WaitForVBlank`) |

---

## Parallax Scrolling

### The Concept

Parallax creates depth by scrolling layers at different speeds:
- **Near objects** (foreground) move fast
- **Far objects** (background) move slow

This mimics how real-world motion parallax works.

### Implementation

```c
static s16 scroll_x = 0;

/* Update scroll based on input */
if (pad & KEY_RIGHT) scroll_x += 2;
if (pad & KEY_LEFT) scroll_x -= 2;

/* Apply different speeds to each layer */
bgSetScroll(0, scroll_x, 0);       /* BG1: full speed */
bgSetScroll(1, scroll_x >> 1, 0);  /* BG2: half speed */
```

### Auto-Scroll Mode

```c
static u8 auto_scroll = 0;

if (pressed & KEY_A) {
    auto_scroll = !auto_scroll;
}

if (auto_scroll) {
    scroll_x += 1;  /* Constant scroll */
}
```

---

## Background Setup

### VRAM Layout

```
$0000: BG1 tiles (foreground)
$2000: BG2 tiles (background)
$4000: BG1 tilemap
$4800: BG2 tilemap
```

### Layer Configuration

```c
/* Set up BG1 (foreground) */
bgSetMapPtr(0, 0x4000, SC_32x32);
bgInitTileSet(0, tiles_fg, palette, 0,
              tiles_fg_size, palette_size,
              BG_16COLORS, 0x0000);

/* Set up BG2 (background) */
bgSetMapPtr(1, 0x4800, SC_32x32);
bgInitTileSet(1, tiles_bg, palette, 1,
              tiles_bg_size, palette_size,
              BG_16COLORS, 0x2000);

/* Enable both layers */
REG_TM = TM_BG1 | TM_BG2;
```

---

## Scroll Registers

Each background has horizontal and vertical scroll registers:

| Register | Address | Purpose |
|----------|---------|---------|
| BG1HOFS | $210D | BG1 horizontal offset |
| BG1VOFS | $210E | BG1 vertical offset |
| BG2HOFS | $210F | BG2 horizontal offset |
| BG2VOFS | $2110 | BG2 vertical offset |

The library function `bgSetScroll(bg, x, y)` sets both values.

---

## Build and Run

```bash
cd examples/graphics/scrolling
make clean && make

# Run in emulator
/path/to/Mesen scrolling.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Scroll logic and input handling |
| `data.asm` | Background graphics data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Three-Layer Parallax

Add a third layer (BG3 in Mode 1 has limited colors):
```c
bgSetScroll(2, scroll_x >> 2, 0);  /* Even slower */
```

### Exercise 2: Vertical Parallax

Create a vertically scrolling scene:
```c
bgSetScroll(0, 0, scroll_y);
bgSetScroll(1, 0, scroll_y >> 1);
```

### Exercise 3: Smooth Speed

Use fixed-point for smoother half-speed scrolling:
```c
static s32 bg2_scroll_x = 0;  /* 16.16 fixed point */
bg2_scroll_x += scroll_x << 15;  /* Accumulate half */
bgSetScroll(1, bg2_scroll_x >> 16, 0);
```

### Exercise 4: Infinite Scroll

Create seamless looping by using repeating tilemaps:
```c
/* Tilemap wraps at 256 or 512 pixels depending on size */
/* Just keep incrementing scroll_x */
```

---

## Technical Notes

### Scroll Range

- 10-bit scroll values (0-1023)
- Tilemap wraps at boundaries
- For 32x32 tilemap: wraps at 256 pixels
- For 64x32 tilemap: wraps at 512 pixels

### VBlank Timing

Always update scroll registers during VBlank:
```c
WaitForVBlank();
bgSetScroll(0, scroll_x, scroll_y);
```

### Multiple Scroll Speeds

Common parallax ratios:
- Foreground: 1.0x (full speed)
- Midground: 0.5x (half speed)
- Far background: 0.25x (quarter speed)
- Sky/horizon: 0.0x (static)

---

## What's Next?

**Mode 1:** [Mode 1 Example](../mode1/) - Basic background setup

**Sprites:** [Simple Sprite](../simple_sprite/) - Adding sprites

---

## License

Code: MIT
