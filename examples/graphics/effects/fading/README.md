# Fading Example

Screen brightness transitions for fade in/out effects.

## Learning Objectives

After this lesson, you will understand:
- How SNES screen brightness control works
- Creating smooth fade transitions
- Different fade speeds and styles
- Using fades for scene transitions

## Prerequisites

- Completed background examples
- Understanding of VBlank timing

---

## What This Example Does

Demonstrates multiple fade effects:
- Fade from black to full brightness
- Fade from full brightness to black
- Fast and slow fade speeds
- Button-triggered transitions

```
Frame 1:    ████████████████  (Black)
Frame 5:    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  (25%)
Frame 10:   ░░░░░░░░░░░░░░░░  (50%)
Frame 15:   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒  (75%)
Frame 20:   [Full Image]      (100%)
```

**Controls:**
- Any button: Advance to next fade effect

---

## Code Type

**C with Direct Register Access**

| Component | Type |
|-----------|------|
| Background setup | Library functions |
| Brightness control | Direct register (`REG_INIDISP`) |
| Fade timing | Frame counting with `WaitForVBlank()` |
| Input detection | Direct register access |

---

## INIDISP Register

The INIDISP register ($2100) controls screen brightness:

```
Bits 0-3: Brightness level (0-15)
Bit 4:    Force blank (1 = screen off)
Bits 5-7: Unused
```

| Value | Effect |
|-------|--------|
| $00 | Brightness 0 (black, screen on) |
| $0F | Brightness 15 (full) |
| $80 | Force blank (screen completely off) |
| $8F | Force blank at full brightness |

---

## Basic Fade Implementation

### Fade In (Black to Full)

```c
void fadeIn(u8 speed) {
    u8 brightness;

    for (brightness = 0; brightness <= 15; brightness++) {
        REG_INIDISP = brightness;

        /* Wait multiple frames for slower fade */
        for (u8 i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
}
```

### Fade Out (Full to Black)

```c
void fadeOut(u8 speed) {
    u8 brightness;

    for (brightness = 15; brightness > 0; brightness--) {
        REG_INIDISP = brightness;

        for (u8 i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
    REG_INIDISP = 0;  /* Ensure fully black */
}
```

---

## Fade Speeds

Different speeds for different purposes:

| Speed | Frames per step | Total frames | Duration |
|-------|-----------------|--------------|----------|
| 1 | 1 | 15 | 0.25 sec |
| 2 | 2 | 30 | 0.5 sec |
| 4 | 4 | 60 | 1.0 sec |
| 8 | 8 | 120 | 2.0 sec |

### Quick Flash

```c
void flash(void) {
    REG_INIDISP = 0x0F;  /* Full white */
    WaitForVBlank();
    WaitForVBlank();
    fadeIn(1);           /* Quick restore */
}
```

---

## Scene Transition Pattern

```c
void changeScene(void) {
    /* Fade out current scene */
    fadeOut(2);

    /* Force blank for safe VRAM updates */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Load new graphics (safe during blank) */
    loadNewBackground();
    loadNewSprites();

    /* Fade in new scene */
    fadeIn(2);
}
```

---

## Build and Run

```bash
cd examples/graphics/fading
make clean && make

# Run in emulator
/path/to/Mesen fading.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Fade logic and demo sequence |
| `data.asm` | Background graphics |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Smooth Fade

Create smoother fades using sub-frame timing (though limited by 16 brightness levels):
```c
/* Use longer waits between steps */
for (u8 wait = 0; wait < 8; wait++) {
    WaitForVBlank();
}
```

### Exercise 2: Directional Wipe

Combine with window masking for a wipe effect instead of fade.

### Exercise 3: Color Fade

Use color math to fade to a specific color instead of black:
```c
colorMathSetFixedColor(r, g, b);
/* Increase color math intensity as brightness drops */
```

### Exercise 4: Pulse Effect

Create a pulsing brightness effect:
```c
static u8 pulse_dir = 1;
static u8 brightness = 8;

brightness += pulse_dir;
if (brightness >= 15 || brightness <= 4) {
    pulse_dir = -pulse_dir;
}
REG_INIDISP = brightness;
```

---

## Technical Notes

### Force Blank vs Brightness 0

- **Brightness 0**: Screen shows black, but PPU still renders
- **Force Blank**: PPU stops rendering, VRAM access is safe

Use force blank when updating graphics, brightness 0 for visual fades.

### Fade During Gameplay

For in-game fades without stopping action:
1. Use force blank only briefly for VRAM updates
2. Fade brightness while game continues
3. Game logic runs normally during fade

### Alternative: Palette Fade

Instead of hardware brightness, modify palette colors:
```c
/* Fade palette toward black */
for (u8 i = 0; i < 256; i++) {
    palette[i] = (palette[i] * fade_level) >> 4;
}
dmaCopyCGram(palette, 0, 512);
```

This allows fading specific layers independently.

---

## What's Next?

**Mode 0:** [Mode 0 Example](../mode0/) - Four background layers

**Parallax:** [Parallax Example](../parallax/) - Multi-speed scrolling

---

## License

Code: MIT
