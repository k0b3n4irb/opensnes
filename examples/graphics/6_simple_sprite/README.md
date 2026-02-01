# Simple Sprite Example

Display a single 32x32 sprite on screen.

## Learning Objectives

After this lesson, you will understand:
- OAM (Object Attribute Memory) structure
- Sprite tile loading to VRAM
- Sprite palette configuration
- Basic sprite positioning and attributes

## Prerequisites

- Completed background examples
- Understanding of tile formats

---

## What This Example Does

Displays a static 32x32 pixel sprite:
- Loads sprite graphics to VRAM
- Sets up sprite palette
- Positions sprite at screen center
- Demonstrates basic OAM usage

```
+----------------------------------------+
|                                        |
|                                        |
|                                        |
|              [SPRITE]                  |
|               32x32                    |
|                                        |
|                                        |
+----------------------------------------+
```

**Controls:** None (static display)

---

## Code Type

**C with Library Functions**

| Component | Type |
|-----------|------|
| OAM init | Library (`oamInit`) |
| Graphics loading | Library (`oamInitGfxSet`) |
| Sprite setup | Library (`oamSet`, `oamSetSize`) |
| Sprite display | Library (`oamUpdate`, `oamSetVisible`) |
| Unused sprites | Library (`oamHide`) |

---

## OAM Overview

The SNES can display up to 128 sprites using OAM:

| Feature | Value |
|---------|-------|
| Max sprites | 128 |
| Sprite sizes | 8x8, 16x16, 32x32, 64x64 |
| Colors | 16 per sprite (from 8 palettes) |
| Sprite VRAM | $0000-$7FFF (32KB) |
| OAM size | 544 bytes |

---

## Basic Sprite Setup

### 1. Initialize OAM

```c
/* Initialize OAM with size configuration */
oamInit();

/* Set all sprites to small size (16x16) */
/* oamSetSize configures size bits in secondary table */
```

### 2. Load Graphics

```c
/* Load sprite tiles and palette */
oamInitGfxSet(sprite_tiles, sprite_tiles_size,
              sprite_palette, sprite_palette_size,
              0,     /* Palette slot (0-7) */
              0x0000, /* VRAM address */
              OBJ_SIZE16_L32);  /* 16x16 small, 32x32 large */
```

### 3. Set Sprite Attributes

```c
/* Set sprite 0 at position (100, 80) */
oamSet(0,           /* Sprite number (0-127) */
       100, 80,     /* X, Y position */
       0,           /* First tile number */
       0,           /* Palette (0-7) */
       3,           /* Priority (0-3) */
       0);          /* Attributes (flip, size) */
```

### 4. Show the Sprite

```c
oamSetVisible(0, OBJ_SHOW);  /* Make sprite 0 visible */
oamUpdate();                  /* Transfer OAM to hardware */
```

---

## OAM Entry Format

Each sprite uses 4 bytes in main OAM + 2 bits in secondary table:

### Main OAM (4 bytes per sprite)

```
Byte 0: X position (lower 8 bits)
Byte 1: Y position (8 bits)
Byte 2: Tile number (8 bits, can access tiles 0-511)
Byte 3: vhoopppc
        v = Vertical flip
        h = Horizontal flip
        oo = Priority (0-3)
        ppp = Palette (0-7)
        c = Tile high bit (for tiles 256-511)
```

### Secondary OAM (2 bits per sprite)

```
Bit 0: X position bit 8 (allows X = 256-511)
Bit 1: Size select (0 = small, 1 = large)
```

---

## Sprite Sizes

OBSEL register configures two sprite sizes:

| Setting | Small | Large |
|---------|-------|-------|
| OBJ_SIZE8_L16 | 8x8 | 16x16 |
| OBJ_SIZE8_L32 | 8x8 | 32x32 |
| OBJ_SIZE8_L64 | 8x8 | 64x64 |
| OBJ_SIZE16_L32 | 16x16 | 32x32 |
| OBJ_SIZE16_L64 | 16x16 | 64x64 |
| OBJ_SIZE32_L64 | 32x32 | 64x64 |

---

## Build and Run

```bash
cd examples/graphics/6_simple_sprite
make clean && make

# Run in emulator
/path/to/Mesen simple_sprite.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Sprite setup code |
| `data.asm` | Sprite graphics and palette |
| `Makefile` | Build configuration |
| `res/` | Source sprite graphics |

---

## Exercises

### Exercise 1: Move the Sprite

Add D-pad control:
```c
if (pad & KEY_RIGHT) sprite_x++;
if (pad & KEY_LEFT) sprite_x--;
oamSet(0, sprite_x, sprite_y, ...);
```

### Exercise 2: Multiple Sprites

Display several sprites:
```c
oamSet(0, 50, 80, 0, 0, 3, 0);
oamSet(1, 100, 80, 0, 0, 3, 0);
oamSet(2, 150, 80, 0, 0, 3, 0);
```

### Exercise 3: Sprite Flipping

Add horizontal/vertical flip:
```c
/* Attribute byte: bit 6 = H flip, bit 7 = V flip */
oamSet(0, x, y, tile, pal, pri, 0x40);  /* H flip */
oamSet(1, x, y, tile, pal, pri, 0x80);  /* V flip */
oamSet(2, x, y, tile, pal, pri, 0xC0);  /* Both */
```

### Exercise 4: Change Palette

Use different sprite palettes:
```c
/* Palette parameter (0-7) selects from colors 128-255 */
oamSet(0, x, y, tile, 0, 3, 0);  /* Palette 0 */
oamSet(1, x, y, tile, 1, 3, 0);  /* Palette 1 */
```

---

## Technical Notes

### Sprite Priority

Priority 0-3 determines sprite ordering relative to backgrounds:
- Priority 3: Always in front
- Priority 2: In front of BG1/2
- Priority 1: Behind BG1, in front of BG2
- Priority 0: Behind both BGs

### VRAM Addressing

Sprite tiles use a different addressing than backgrounds:
- Tile number 0-255: First name table
- Tile number 256-511: Second name table (set by OBSEL)

### Y Position

Y=0 is top of screen. Y values 224-255 position sprites off the bottom (effectively hiding them). Y=240 is commonly used to hide unused sprites.

---

## What's Next?

**Animation:** [Animation Example](../2_animation/) - Animated sprites

**Effects:** [Fading](../7_fading/) - Screen transitions

---

## License

Code: MIT
