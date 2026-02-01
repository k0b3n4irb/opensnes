# Color Math Transparency Example

Interactive shadow and tint effects using the SNES color math hardware.

## Learning Objectives

After this lesson, you will understand:
- How SNES color math works
- Adding vs subtracting fixed colors
- Creating shadow (darken) effects
- Creating tint (color overlay) effects
- The three color math registers: CGWSEL, CGADSUB, COLDATA

## Prerequisites

- Understanding of backgrounds and palettes
- Completed basic graphics examples

---

## What This Example Does

Displays a background with interactive color math effects:
- **Shadow mode**: Darkens the entire screen (adjustable intensity)
- **Tint mode**: Applies a colored overlay (red, green, or blue)

```
+----------------------------------------+
|                                        |
|    [Background Image]                  |
|                                        |
|    Press A: Toggle shadow              |
|    Press B: Cycle tints                |
|    Up/Down: Adjust intensity           |
|                                        |
+----------------------------------------+
```

**Controls:**
- A button: Toggle shadow effect on/off
- B button: Cycle through tints (none -> red -> green -> blue)
- D-Pad Up: Increase shadow intensity
- D-Pad Down: Decrease shadow intensity

---

## Code Type

**Pure C**

| Component | Type |
|-----------|------|
| Main loop | C (`main.c`) |
| Input handling | C (direct register reads) |
| Color math | Library (`colormath.c`) |
| Graphics loading | Library (`bgInitTileSet`, `dmaCopyVram`) |
| Graphics data | Assembly (`data.asm`) |

---

## Color Math Fundamentals

### The Three Registers

| Register | Address | Purpose |
|----------|---------|---------|
| CGWSEL | $2130 | Select color source (fixed or subscreen) |
| CGADSUB | $2131 | Operation (add/sub) and layer enables |
| COLDATA | $2132 | Fixed color value (5-bit per channel) |

### CGWSEL - Color Math Control A

```
Bit 1-0: Color math source
  00 = Always use subscreen
  01 = Subscreen where enabled, else fixed
  10 = Fixed color only
  11 = Disable color math
```

For fixed color effects, use `REG_CGWSEL = 0x02`.

### CGADSUB - Color Math Control B

```
Bit 7: Add/Subtract (0=add, 1=subtract)
Bit 6: Half result (divide by 2)
Bits 5-0: Layer enable (BK|OBJ|BG4|BG3|BG2|BG1)
```

Example:
```c
REG_CGADSUB = 0x80 | 0x01;  /* Subtract, BG1 enabled */
REG_CGADSUB = 0x00 | 0x01;  /* Add, BG1 enabled */
```

### COLDATA - Fixed Color

```
Bit 7: Blue channel
Bit 6: Green channel
Bit 5: Red channel
Bits 4-0: Intensity (0-31)
```

---

## Shadow Effect

Shadows are created by **subtracting** from the screen colors:

```c
void colorMathShadow(u8 layers, u8 intensity) {
    /* Enable color math on specified layers */
    colorMathEnable(layers);

    /* Subtract mode */
    colorMathSetOp(COLORMATH_SUB);

    /* Use fixed color */
    colorMathSetSource(COLORMATH_SRC_FIXED);

    /* Set all channels to same intensity (grayscale subtraction) */
    colorMathSetFixedColor(intensity, intensity, intensity);
}
```

Higher intensity = darker shadow (more color subtracted).

---

## Tint Effect

Tints are created by **adding** a single color channel:

```c
/* Red tint */
colorMathSetOp(COLORMATH_ADD);
colorMathSetFixedColor(12, 0, 0);  /* Add red only */

/* Green tint */
colorMathSetFixedColor(0, 12, 0);  /* Add green only */

/* Blue tint */
colorMathSetFixedColor(0, 0, 12);  /* Add blue only */
```

This makes the screen appear tinted toward that color.

---

## State Machine

The example uses a simple state machine:

```c
static u8 shadow_intensity = 8;
static u8 shadow_enabled = 0;
static u8 tint_mode = 0;  /* 0=none, 1=red, 2=green, 3=blue */

static void apply_effect(void) {
    if (shadow_enabled) {
        colorMathShadow(COLORMATH_ALL, shadow_intensity);
    } else if (tint_mode > 0) {
        /* Apply tint based on mode */
        colorMathEnable(COLORMATH_ALL);
        colorMathSetOp(COLORMATH_ADD);
        colorMathSetSource(COLORMATH_SRC_FIXED);

        switch (tint_mode) {
            case 1: colorMathSetFixedColor(12, 0, 0); break;
            case 2: colorMathSetFixedColor(0, 12, 0); break;
            case 3: colorMathSetFixedColor(0, 0, 12); break;
        }
    } else {
        colorMathDisable();
    }
}
```

---

## Input Handling

Edge detection for button presses:

```c
u16 pad_prev = 0;

while (1) {
    WaitForVBlank();

    /* Read current joypad state */
    while (REG_HVBJOY & 0x01) {}
    u16 pad = REG_JOY1L | (REG_JOY1H << 8);

    /* Detect new presses (wasn't pressed before, is pressed now) */
    u16 pressed = pad & ~pad_prev;

    /* Toggle shadow on A press */
    if (pressed & 0x0080) {  /* A button */
        shadow_enabled = !shadow_enabled;
        apply_effect();
    }

    pad_prev = pad;
}
```

---

## Build and Run

```bash
cd examples/graphics/17_transparency
make clean && make

# Run in emulator
/path/to/Mesen transparency.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Main loop, input handling, effect state |
| `data.asm` | Background graphics data |
| `Makefile` | Build configuration |
| `res/pvsneslib.bmp` | Source background image |

---

## Color Math Layer Constants

```c
#define COLORMATH_BG1      0x01
#define COLORMATH_BG2      0x02
#define COLORMATH_BG3      0x04
#define COLORMATH_BG4      0x08
#define COLORMATH_OBJ      0x10
#define COLORMATH_BACKDROP 0x20
#define COLORMATH_ALL      0x3F
```

Use these to control which layers are affected:

```c
/* Shadow only on BG1 and BG2 */
colorMathShadow(COLORMATH_BG1 | COLORMATH_BG2, 10);

/* Tint sprites only */
colorMathEnable(COLORMATH_OBJ);
```

---

## Common Effects

### 50% Transparency

```c
colorMathEnable(COLORMATH_BG1);
colorMathSetOp(COLORMATH_ADD);
colorMathSetHalf(1);  /* Divide result by 2 */
colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
/* Subscreen must have content to blend with */
```

### Fade to Black

```c
/* Increase shadow intensity from 0 to 31 over time */
for (u8 i = 0; i < 32; i++) {
    colorMathShadow(COLORMATH_ALL, i);
    WaitForVBlank();
}
```

### Flash Effect

```c
/* Quick white flash */
colorMathEnable(COLORMATH_ALL);
colorMathSetOp(COLORMATH_ADD);
colorMathSetFixedColor(31, 31, 31);  /* Full white */
WaitForVBlank();
WaitForVBlank();
colorMathDisable();
```

---

## Exercises

### Exercise 1: Fade Transition

Create a fade-to-black transition:
```c
/* Call each frame */
void fadeOut(void) {
    static u8 fade_level = 0;
    if (fade_level < 31) {
        fade_level++;
        colorMathShadow(COLORMATH_ALL, fade_level);
    }
}
```

### Exercise 2: Pulsing Tint

Make the tint pulse by varying intensity:
```c
static u8 pulse_dir = 1;
static u8 pulse_val = 0;

pulse_val += pulse_dir;
if (pulse_val >= 20 || pulse_val == 0) {
    pulse_dir = -pulse_dir;
}
colorMathSetFixedColor(pulse_val, 0, 0);  /* Pulsing red */
```

### Exercise 3: Day/Night Cycle

Combine shadow with a blue tint for night:
```c
/* Night time effect */
colorMathSetOp(COLORMATH_ADD);
colorMathSetFixedColor(0, 0, 8);  /* Slight blue */
/* Then overlay shadow */
/* Note: May need HDMA for complex multi-effect */
```

### Exercise 4: Damage Flash

Flash red when taking damage:
```c
void damageFlash(void) {
    colorMathEnable(COLORMATH_ALL);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetFixedColor(20, 0, 0);  /* Red flash */

    for (u8 i = 0; i < 6; i++) WaitForVBlank();

    colorMathDisable();
}
```

---

## Technical Notes

### Why Fixed Color?

The SNES has two color math sources:
1. **Subscreen** - For transparency between layers
2. **Fixed color** - For uniform tint/shadow effects

Fixed color is simpler when you want a consistent effect across the entire screen.

### Half Mode

Setting `colorMathSetHalf(1)` divides the result by 2. This is essential for true 50% transparency but optional for shadow/tint effects.

### Layer Priority

Color math respects layer priority:
- Higher priority layers can block color math
- Use layer masks to control which layers are affected

---

## What's Next?

**Window Masking:** [Window Example](../18_window/) - Spotlight effect

**HDMA:** [HDMA Gradient Example](../16_hdma_gradient/) - Per-scanline effects

---

## License

Code: MIT
