# Metasprite Example

Composing large sprites from multiple OAM entries.

## Learning Objectives

After this lesson, you will understand:
- What metasprites are and why they're needed
- Arranging multiple sprites into larger characters
- OAM tile addressing for 16x16 sprites
- Direct OAM buffer manipulation

## Prerequisites

- Completed simple sprite example
- Understanding of OAM structure

---

## What This Example Does

Displays a 32x48 pixel character composed of 6 sprites:
- 2 columns x 3 rows of 16x16 sprites
- Demonstrates large character rendering
- Shows proper tile arrangement

```
+----------------------------------------+
|                                        |
|           +----+----+                  |
|           | 0  | 1  |  <- Row 0        |
|           +----+----+                  |
|           | 2  | 3  |  <- Row 1        |
|           +----+----+                  |
|           | 4  | 5  |  <- Row 2        |
|           +----+----+                  |
|            32x48 metasprite            |
+----------------------------------------+
```

**Controls:** None (static display)

---

## Code Type

**C with Direct OAM Access**

| Component | Type |
|-----------|------|
| Sprite init | Library (`oamInitDynamicSprite`, `oamClear`) |
| Tile loading | Library (`dmaCopyVram`) |
| Palette loading | Library (`dmaCopyCGram`) |
| OAM writing | Direct buffer manipulation |
| Display update | Library (`oamUpdate`) |

---

## Metasprite Concept

### Why Metasprites?

SNES sprites are limited to these sizes:
- 8x8, 16x16, 32x32, or 64x64

For custom sizes (like 32x48), combine multiple sprites.

### Metasprite Definition

```c
typedef struct {
    s8 x_offset;   /* Relative X from metasprite origin */
    s8 y_offset;   /* Relative Y from metasprite origin */
    u16 tile;      /* Tile number */
    u8 attr;       /* Flip, palette, priority */
} MetaspritePart;

MetaspritePart player_meta[] = {
    { 0,  0, 0, 0},   /* Top-left */
    {16,  0, 2, 0},   /* Top-right */
    { 0, 16, 4, 0},   /* Mid-left */
    {16, 16, 6, 0},   /* Mid-right */
    { 0, 32, 8, 0},   /* Bottom-left */
    {16, 32, 10, 0},  /* Bottom-right */
};
```

---

## Tile Arrangement

### 16x16 Sprite Tile Layout

For 16x16 sprites, tiles are arranged:
```
+-----+-----+
|  N  | N+1 |
+-----+-----+
| N+16| N+17|
+-----+-----+
```

Each 16x16 sprite uses 4 tiles (2x2).

### Loading Tiles

```c
/* Load all metasprite tiles to VRAM */
dmaCopyVram(character_tiles, 0x0000, character_tiles_size);

/* Load palette */
dmaCopyCGram(character_palette, 128, palette_size);
```

---

## Drawing Metasprites

### Basic Approach

```c
void drawMetasprite(u8 base_oam, s16 x, s16 y,
                    MetaspritePart *parts, u8 count) {
    for (u8 i = 0; i < count; i++) {
        s16 px = x + parts[i].x_offset;
        s16 py = y + parts[i].y_offset;

        oamSet(base_oam + i, px, py,
               parts[i].tile, 0, 3, parts[i].attr);
    }
}
```

### With Horizontal Flip

```c
void drawMetaspriteFlipped(u8 base_oam, s16 x, s16 y,
                           MetaspritePart *parts, u8 count) {
    for (u8 i = 0; i < count; i++) {
        /* Mirror X offset and flip tiles */
        s16 px = x + (width - parts[i].x_offset - 16);
        s16 py = y + parts[i].y_offset;

        oamSet(base_oam + i, px, py,
               parts[i].tile, 0, 3, parts[i].attr | 0x40);
    }
}
```

---

## Build and Run

```bash
cd examples/graphics/14_metasprite
make clean && make

# Run in emulator
/path/to/Mesen metasprite.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Metasprite setup and display |
| `data.asm` | Character tile data |
| `Makefile` | Build configuration |

---

## Exercises

### Exercise 1: Movable Metasprite

Add D-pad control:
```c
if (pad & KEY_RIGHT) meta_x++;
drawMetasprite(0, meta_x, meta_y, player_meta, 6);
```

### Exercise 2: Animated Metasprite

Change tile offsets for animation:
```c
u16 tile_offset = anim_frame * 12;  /* 6 parts * 2 tiles each */
for (u8 i = 0; i < 6; i++) {
    oamSet(i, ..., player_meta[i].tile + tile_offset, ...);
}
```

### Exercise 3: Multiple Metasprites

Draw several characters:
```c
drawMetasprite(0, player_x, player_y, player_meta, 6);
drawMetasprite(6, enemy_x, enemy_y, enemy_meta, 6);
```

### Exercise 4: Hitbox Visualization

Show the metasprite's bounding box:
```c
/* Draw outline using additional sprites */
```

---

## Technical Notes

### OAM Limits

With 128 sprites total:
- 6-sprite metasprite: 21 on screen
- 4-sprite metasprite: 32 on screen
- Plan sprite usage carefully

### Per-Scanline Limit

Maximum 32 sprites per scanline. Large metasprites can easily hit this limit:
- 32x48 character spans 6 scanlines
- Each scanline has 2 sprites
- 16 characters max per scanline

### Tile Number Calculation

For 16x16 sprites in secondary name table:
```c
/* Tile 0 in secondary = tile | 256 in attribute */
/* Or use OBSEL to set name table base */
```

### Z-Ordering

Parts of the same metasprite share priority. For complex ordering:
- Use different OAM slots strategically
- Consider sprite priority bits

---

## What's Next?

**Dynamic Sprites:** [Dynamic Sprite](../15_dynamic_sprite/) - Automatic VRAM

**HDMA Effects:** [HDMA Gradient](../16_hdma_gradient/) - Per-scanline effects

---

## License

Code: MIT
