# Window Masking Example

An interactive spotlight effect using SNES hardware window masking.

## Learning Objectives

After this lesson, you will understand:
- What hardware windows are on the SNES
- How to create and position windows
- Masking layers with windows
- Inverted vs normal window modes
- Common window effects (spotlight, split-screen, wipes)

## Prerequisites

- Understanding of backgrounds and layers
- Completed basic graphics examples

---

## What This Example Does

Creates a movable "spotlight" effect that reveals a portion of the background:
- A rectangular window moves across the screen
- Content inside the window is visible
- Content outside is masked (hidden)
- Can toggle inverted mode (show outside, hide inside)

```
+----------------------------------------+
|████████████████████████████████████████|
|█████████+----------+███████████████████|
|█████████| VISIBLE  |███████████████████|
|█████████| CONTENT  |███████████████████|
|█████████+----------+███████████████████|
|████████████████████████████████████████|
+----------------------------------------+
```

**Controls:**
- D-Pad Left/Right: Move window horizontally
- A button: Toggle window on/off
- B button: Toggle inverted mode
- L button: Decrease window width
- R button: Increase window width

---

## Code Type

**Pure C**

| Component | Type |
|-----------|------|
| Main loop | C (`main.c`) |
| Input handling | C (direct register reads) |
| Window control | Library (`window.c`) |
| Graphics loading | Library (`bgInitTileSet`, `dmaCopyVram`) |
| Graphics data | Assembly (`data.asm`) |

---

## Window Fundamentals

### What Are Windows?

The SNES has two hardware windows (Window 1 and Window 2) that create rectangular mask regions:
- Each window has left and right boundaries
- Windows span the full screen height
- Windows can mask any layer (BG1-4, sprites)
- Logic operations combine windows

### Window Registers

| Register | Address | Purpose |
|----------|---------|---------|
| WH0 | $2126 | Window 1 left position |
| WH1 | $2127 | Window 1 right position |
| WH2 | $2128 | Window 2 left position |
| WH3 | $2129 | Window 2 right position |
| W12SEL | $2123 | Window 1/2 enable for BG1/2 |
| W34SEL | $2124 | Window 1/2 enable for BG3/4 |
| WOBJSEL | $2125 | Window 1/2 enable for OBJ/Color |
| TMW | $212E | Main screen window mask |
| TSW | $212F | Subscreen window mask |

---

## Setting Up a Window

### Step 1: Set Position

```c
/* Window 1 from X=88 to X=168 (80 pixels wide) */
windowSetPos(WINDOW_1, 88, 168);
```

### Step 2: Enable for Layers

```c
/* Enable Window 1 for BG1 */
windowEnable(WINDOW_1, WINDOW_BG1);
```

### Step 3: Set Inversion

```c
/* Normal: mask outside, show inside */
windowSetInvert(WINDOW_1, WINDOW_BG1, 0);

/* Inverted: show outside, mask inside */
windowSetInvert(WINDOW_1, WINDOW_BG1, 1);
```

### Step 4: Enable Masking on Main Screen

```c
/* Apply window mask to main screen for BG1 */
windowSetMainMask(WINDOW_BG1);
```

---

## The Spotlight Effect

This example creates a centered, movable window:

```c
static u8 window_x = 128;         /* Center of screen */
static u8 window_half_width = 40; /* 80 pixels wide */

static void update_window(void) {
    u8 left, right;

    /* Calculate boundaries with clamping */
    if (window_x < window_half_width) {
        left = 0;
    } else {
        left = window_x - window_half_width;
    }

    if (window_x + window_half_width > 255) {
        right = 255;
    } else {
        right = window_x + window_half_width;
    }

    /* Apply window settings */
    windowSetPos(WINDOW_1, left, right);
    windowEnable(WINDOW_1, WINDOW_BG1);
    windowSetInvert(WINDOW_1, WINDOW_BG1, window_inverted);
    windowSetMainMask(WINDOW_BG1);
}
```

---

## Window Inversion

### Normal Mode (Invert = 0)

```
Outside window: MASKED (invisible)
Inside window:  VISIBLE
```

Use for: Spotlights, reveals, fade-in effects

### Inverted Mode (Invert = 1)

```
Outside window: VISIBLE
Inside window:  MASKED (invisible)
```

Use for: Cutouts, hiding UI elements, special effects

---

## Build and Run

```bash
cd examples/graphics/18_window
make clean && make

# Run in emulator
/path/to/Mesen window.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main loop, input handling, window state |
| `data.asm` | Background graphics data |
| `Makefile` | Build configuration |
| `res/pvsneslib.bmp` | Source background image |

---

## Window Layer Constants

```c
#define WINDOW_1    0
#define WINDOW_2    1

#define WINDOW_BG1  0x01
#define WINDOW_BG2  0x02
#define WINDOW_BG3  0x04
#define WINDOW_BG4  0x08
#define WINDOW_OBJ  0x10
#define WINDOW_MATH 0x20
#define WINDOW_ALL  0x3F
```

---

## Common Window Techniques

### Split Screen

```c
/* Left half for player 1, right half for player 2 */
windowSetPos(WINDOW_1, 0, 127);    /* Left half */
windowSetPos(WINDOW_2, 128, 255);  /* Right half */
/* Apply different BG scrolls in each region using HDMA */
```

### Horizontal Wipe Transition

```c
/* Wipe from left to right */
for (u8 x = 0; x < 255; x++) {
    windowSetPos(WINDOW_1, 0, x);
    windowEnable(WINDOW_1, WINDOW_ALL);
    windowSetMainMask(WINDOW_ALL);
    WaitForVBlank();
}
```

### UI Window

```c
/* Keep a portion always visible for HUD */
windowSetPos(WINDOW_1, 0, 255);    /* Full width */
windowEnable(WINDOW_1, WINDOW_BG3); /* BG3 for HUD */
windowSetInvert(WINDOW_1, WINDOW_BG3, 1); /* Always show HUD */
```

### HDMA-Animated Windows

Combine windows with HDMA to create:
- Circular spotlights (vary left/right per scanline)
- Wavy edges
- Perspective effects

---

## Exercises

### Exercise 1: Expanding Spotlight

Make the window slowly expand:
```c
static u8 expand_timer = 0;
expand_timer++;
if (expand_timer >= 4 && window_half_width < 100) {
    expand_timer = 0;
    window_half_width++;
    update_window();
}
```

### Exercise 2: Two Windows

Use both windows for a double spotlight:
```c
windowSetPos(WINDOW_1, 40, 80);
windowSetPos(WINDOW_2, 176, 216);
windowEnable(WINDOW_1, WINDOW_BG1);
windowEnable(WINDOW_2, WINDOW_BG1);
windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_OR);  /* Show either window */
```

### Exercise 3: Screen Wipe

Create a left-to-right reveal:
```c
void wipeReveal(void) {
    for (u8 x = 0; x < 255; x++) {
        windowSetPos(WINDOW_1, x, 255);
        update_window();
        WaitForVBlank();
    }
}
```

### Exercise 4: Hide Sprites

Mask sprites behind a window:
```c
windowEnable(WINDOW_1, WINDOW_OBJ);
windowSetMainMask(WINDOW_OBJ);
/* Sprites inside the window are hidden */
```

---

## Technical Notes

### Window Logic Operations

When using both windows on the same layer:

```c
#define WINDOW_LOGIC_OR   0x00  /* Show if inside either window */
#define WINDOW_LOGIC_AND  0x01  /* Show if inside both windows */
#define WINDOW_LOGIC_XOR  0x02  /* Show if inside one but not both */
#define WINDOW_LOGIC_XNOR 0x03  /* Show if inside both or neither */

windowSetLogic(WINDOW_BG1, WINDOW_LOGIC_OR);
```

### Vertical Windows with HDMA

Standard windows only define horizontal boundaries. For vertical masking:
1. Set up an HDMA channel targeting WH0/WH1
2. Create a table that changes boundaries per scanline
3. This creates shapes like circles or irregular regions

### Performance

Window operations are free - they're computed by the PPU hardware during rendering. No CPU cost during gameplay.

### Main vs Subscreen

- `windowSetMainMask()` - Affects main screen rendering
- `windowSetSubMask()` - Affects subscreen (for transparency effects)

Most games only use main screen masking.

---

## What's Next?

**Color Math:** [Transparency Example](../17_transparency/) - Shadow and tint effects

**HDMA:** [HDMA Gradient Example](../16_hdma_gradient/) - Per-scanline effects

---

## License

Code: MIT
