# Entity Demo

Minimal sprite test for basic OAM and tile setup.

## Learning Objectives

After this lesson, you will understand:
- Basic 4bpp sprite tile format
- Direct OAM manipulation
- Minimal sprite display setup
- Sprite palette configuration

## Prerequisites

- Basic SNES knowledge
- Understanding of tile concepts

---

## What This Example Does

Displays a single 8x8 sprite:
- Minimal code for sprite rendering
- Direct hardware register access
- Foundation for understanding OAM
- Static display (no interaction)

```
+----------------------------------------+
|                                        |
|                                        |
|                                        |
|              [8x8 SPRITE]              |
|                                        |
|                                        |
|                                        |
+----------------------------------------+
```

**Controls:** None (static test display)

---

## Code Type

**C with Direct Register Access**

| Component | Type |
|-----------|------|
| Console init | Library (`consoleInit`) |
| OAM setup | Library (`oamInit`, `oamSet`) |
| Sprite update | Library (`oamUpdate`) |
| Tile data | Embedded C array |
| Main loop | VBlank sync |

---

## 4bpp Sprite Tile

### Tile Data Format

4bpp tiles use 32 bytes per 8x8 tile:

```c
static const u8 sprite_tile[32] = {
    /* Bitplanes 0-1 (rows 0-7) */
    0x18, 0x00,  /* Row 0: BP0, BP1 */
    0x24, 0x00,  /* Row 1 */
    0x42, 0x00,  /* Row 2 */
    0xFF, 0x00,  /* Row 3 */
    0x42, 0x00,  /* Row 4 */
    0x42, 0x00,  /* Row 5 */
    0x42, 0x00,  /* Row 6 */
    0x00, 0x00,  /* Row 7 */

    /* Bitplanes 2-3 (rows 0-7) */
    0x00, 0x18,  /* Row 0: BP2, BP3 */
    0x00, 0x24,  /* Row 1 */
    0x00, 0x42,  /* Row 2 */
    0x00, 0xFF,  /* Row 3 */
    0x00, 0x42,  /* Row 4 */
    0x00, 0x42,  /* Row 5 */
    0x00, 0x42,  /* Row 6 */
    0x00, 0x00,  /* Row 7 */
};
```

---

## Minimal Setup

### Loading Tile to VRAM

```c
/* Set VRAM address for sprites ($0000) */
REG_VMAIN = 0x80;
REG_VMADDL = 0x00;
REG_VMADDH = 0x00;

/* Upload tile data */
for (u16 i = 0; i < 32; i += 2) {
    REG_VMDATAL = sprite_tile[i];
    REG_VMDATAH = sprite_tile[i + 1];
}
```

### Setting Palette

```c
/* Sprite palettes start at color 128 */
REG_CGADD = 128;

/* Write 16 colors (4bpp = 16 color palette) */
REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 0: Transparent */
REG_CGDATA = 0x1F; REG_CGDATA = 0x00;  /* Color 1: Red */
/* ... more colors ... */
```

### Configuring OAM

```c
/* Initialize OAM */
oamInit();

/* Set sprite 0 at position (100, 80) */
oamSet(0,           /* Sprite number */
       100, 80,     /* X, Y */
       0,           /* Tile number */
       0,           /* Palette (0-7) */
       3,           /* Priority */
       0);          /* Attributes */

/* Enable sprites on main screen */
REG_TM = 0x10;
```

---

## Main Loop

```c
while (1) {
    /* Wait for VBlank */
    WaitForVBlank();

    /* Update OAM */
    oamUpdate();
}
```

---

## Build and Run

```bash
cd examples/game/entity_demo
make clean && make

# Run in emulator
/path/to/Mesen entity_demo.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Complete sprite test code |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Change Color

Modify the palette to use different colors:
```c
REG_CGDATA = 0xE0; REG_CGDATA = 0x03;  /* Green */
```

### Exercise 2: Larger Sprite

Use 16x16 sprite:
```c
oamInitEx(OBJ_SIZE8_L16, 0);
/* Sprite uses 4 tiles in 2x2 arrangement */
```

### Exercise 3: Multiple Sprites

Display several sprites:
```c
for (u8 i = 0; i < 4; i++) {
    oamSet(i, 50 + i*20, 80, 0, 0, 3, 0);
}
```

### Exercise 4: Add Movement

Make the sprite move:
```c
static u8 x = 100;
x++;
if (x > 240) x = 0;
oamSet(0, x, 80, 0, 0, 3, 0);
```

---

## Technical Notes

### Why This Example?

This is the absolute minimum for sprite display:
- Understand each step clearly
- No library abstraction
- Foundation for complex sprites

### OAM Buffer

The library maintains an OAM buffer in RAM:
- Modify buffer with `oamSet()`
- Transfer to hardware with `oamUpdate()`
- Always update during VBlank

### Sprite Priority

Priority 3 = always in front of backgrounds
Priority 0 = behind backgrounds

### Tile Numbering

8x8 sprites: tile 0-511
16x16 sprites: uses 4 consecutive 8x8 tiles

---

## What's Next?

**Full Game:** [Breakout](../breakout/) - Complete game example

**Sprites:** [Simple Sprite](../../graphics/simple_sprite/) - Detailed sprite guide

---

## License

Code: MIT
