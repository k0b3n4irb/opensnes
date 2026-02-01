# Mode 3 Example

256-color background for high-detail single-layer graphics.

## Learning Objectives

After this lesson, you will understand:
- Mode 3 configuration
- 8bpp (256-color) tile format
- When to use high-color modes
- Trade-offs between colors and layers

## Prerequisites

- Completed Mode 1 example
- Understanding of palette organization

---

## What This Example Does

Displays a single 256-color background:
- Full 256-color palette utilization
- Detailed, colorful graphics
- Static display demonstrating Mode 3 capability

```
+----------------------------------------+
|                                        |
|     [HIGH COLOR BACKGROUND IMAGE]      |
|                                        |
|         256 colors available           |
|                                        |
|                                        |
+----------------------------------------+
```

**Controls:** None (static display)

---

## Code Type

**C with Assembly Helpers**

| Component | Type |
|-----------|------|
| Graphics loading | Assembly (`load_mode3_graphics`) |
| Register setup | Assembly (`setup_mode3_regs`) |
| Main loop | C |

---

## Mode 3 Overview

Mode 3 offers maximum color depth on one layer:

| Layer | Color Depth | Colors |
|-------|-------------|--------|
| BG1 | 8bpp | 256 |
| BG2 | 4bpp | 16 |
| Sprites | 4bpp | 16 per palette |

### Mode 3 Use Cases

- Title screens with detailed artwork
- Cutscenes and story sequences
- Games prioritizing visual quality over layers
- Photo-realistic backgrounds

---

## 8bpp Tile Format

Each 8x8 tile uses 64 bytes:

```
Bitplanes 0-1: 16 bytes (like 2bpp)
Bitplanes 2-3: 16 bytes (like 4bpp layer 2)
Bitplanes 4-5: 16 bytes
Bitplanes 6-7: 16 bytes

Total: 64 bytes per tile
```

This is 4x larger than 4bpp tiles, so VRAM fills quickly.

---

## VRAM Considerations

With 64 bytes per tile, VRAM limits are strict:

```
VRAM size: 64KB
8bpp tiles: 64 bytes each
Maximum tiles: 1024 tiles (if no tilemap)

Practical layout:
$0000-$0FFF: Tilemap (4KB)
$1000-$FFFF: Tiles (60KB = ~937 tiles)
```

---

## Mode 3 Setup

```c
/* Set Mode 3 */
REG_BGMODE = 0x03;  /* Mode 3 */

/* BG1: 8bpp layer */
REG_BG1SC = 0x00;     /* Tilemap at $0000 */
REG_BG12NBA = 0x01;   /* Tiles at $1000 */

/* Enable BG1 */
REG_TM = 0x01;
```

---

## Palette Organization

In Mode 3, BG1 uses all 256 colors:

```
Colors 0-255: BG1 palette (8bpp)
Colors 0-15: Also available for BG2 (4bpp)
Colors 128-255: Sprite palettes (8 x 16 colors)
```

Note: There's palette overlap - plan carefully!

---

## Build and Run

```bash
cd examples/graphics/10_mode3
make clean && make

# Run in emulator
/path/to/Mesen mode3.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Initialization and main loop |
| `helpers.asm` | Graphics loading routines |
| `data.asm` | 8bpp tile and palette data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Add BG2

Layer a 16-color foreground over the 256-color background:
```c
REG_TM = 0x03;  /* Enable BG1 + BG2 */
```

### Exercise 2: Palette Animation

Cycle palette colors for animation effects:
```c
/* Rotate colors 16-31 for water effect */
u16 temp = palette[16];
for (u8 i = 16; i < 31; i++) {
    palette[i] = palette[i+1];
}
palette[31] = temp;
dmaCopyCGram(palette, 0, 512);
```

### Exercise 3: Fade Effect

Fade the 256-color image:
```c
for (u8 b = 15; b > 0; b--) {
    REG_INIDISP = b;
    WaitForVBlank();
}
```

### Exercise 4: Scroll the Image

Add scrolling to explore a larger image:
```c
if (pad & KEY_RIGHT) scroll_x++;
bgSetScroll(0, scroll_x, scroll_y);
```

---

## Technical Notes

### Converting Images

Use `gfx4snes` or similar tools to convert images:
```bash
gfx4snes -m -p -t 8 image.png
```

The `-t 8` flag specifies 8bpp output.

### VRAM Usage

Calculate your needs carefully:
- 32x32 tilemap: 2KB
- 256 unique 8bpp tiles: 16KB
- Total: 18KB (fits easily)

For full-screen unique tiles:
- 32x28 tiles = 896 unique tiles
- 896 x 64 bytes = 57KB (tight fit!)

### Mode 3 vs Mode 7

Both offer 256 colors, but:
- Mode 3: Standard tile-based, no rotation
- Mode 7: Single tile plane with rotation/scaling

---

## What's Next?

**Mode 5:** [Mode 5 Example](../11_mode5/) - Hi-res interlaced

**Continuous Scroll:** [Continuous Scroll](../12_continuous_scroll/) - Game-like scrolling

---

## License

Code: MIT
