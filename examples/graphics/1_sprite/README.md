# Lesson 1: Sprite Display

Display a sprite and control it with the D-pad.

## Learning Objectives

After this lesson, you will understand:
- ✅ How OAM (Object Attribute Memory) works
- ✅ Sprite tile format (4bpp)
- ✅ Loading graphics to VRAM
- ✅ Loading palettes to CGRAM
- ✅ Reading controller input
- ✅ Using the `gfx2snes` tool

## Prerequisites

- Completed [Text lessons](../../text/)
- Understanding of VRAM and tilemaps

---

## Sprites vs Backgrounds

| Feature | Backgrounds | Sprites |
|---------|-------------|---------|
| Position | Fixed grid (tiles) | Pixel-perfect anywhere |
| Movement | Scroll registers | Per-object X/Y |
| Quantity | 1-4 layers | Up to 128 objects |
| Use for | Scenery, maps | Characters, items, bullets |

Sprites are perfect for:
- Player characters
- Enemies
- Projectiles
- Collectibles
- Anything that moves independently

---

## How Sprites Work

### The OAM Buffer

OAM (Object Attribute Memory) stores sprite properties:

```
For each of 128 sprites:
┌────────────┬────────────┬────────────┬────────────┐
│  X pos     │  Y pos     │  Tile #    │ Attributes │
│  (8 bits)  │  (8 bits)  │  (8 bits)  │  vhppccct  │
└────────────┴────────────┴────────────┴────────────┘

Attributes:
  v = Vertical flip
  h = Horizontal flip
  pp = Priority (0-3, higher = in front)
  ccc = Palette (0-7)
  t = Tile high bit
```

Plus a "high table" with 2 extra bits per sprite:
- X position bit 8 (for X > 255)
- Size select (small or large)

### The Rendering Pipeline

```
1. Your Code          2. OAM Buffer         3. Hardware
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│ oam_set(     │ ──▶ │ RAM buffer   │ ──▶ │ PPU reads    │
│   id, x, y,  │     │ (544 bytes)  │     │ during VBlank│
│   tile, ...  │     │              │     │              │
│ )            │     │              │     │              │
└──────────────┘     └──────────────┘     └──────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │ oam_update() │ Copy to hardware
                     │ during VBlank│
                     └──────────────┘
```

---

## 16×16 Sprites

A 16×16 sprite is made of 4 tiles:

```
VRAM layout (128 pixels = 16 tiles wide):
┌────┬────┬────┬────┬────┬...
│ T0 │ T1 │ T2 │ T3 │    │
├────┼────┼────┼────┼────┤
│T16 │T17 │T18 │T19 │    │   ← 16 tiles per row
└────┴────┴────┴────┴────┘

Your 16×16 sprite uses:
┌────┬────┐
│ T0 │ T1 │   Top-left and top-right
├────┼────┤
│T16 │T17 │   Bottom-left and bottom-right
└────┴────┘
```

The PPU automatically fetches adjacent tiles for large sprites.

---

## The Code Explained

### 1. Initialize OAM

```c
static void oam_init(void) {
    u8 i;
    /* Hide all sprites by setting Y = 240 (off-screen) */
    for (i = 0; i < 128; i++) {
        oam_lo[i * 4 + 1] = 240;  /* Y position */
    }
}
```

### 2. Load Sprite Graphics

```c
static void load_sprite_tiles(void) {
    REG_VMAIN = 0x80;     /* Auto-increment mode */
    REG_VMADDL = 0x00;    /* VRAM address $0000 */
    REG_VMADDH = 0x00;

    for (i = 0; i < sprite_TILES_SIZE; i += 2) {
        REG_VMDATAL = sprite_tiles[i];
        REG_VMDATAH = sprite_tiles[i + 1];
    }
}
```

### 3. Load Palette

```c
static void load_sprite_palette(void) {
    REG_CGADD = 128;  /* Sprite palettes start at 128 */

    for (i = 0; i < sprite_PAL_COUNT; i++) {
        REG_CGDATA = sprite_pal[i] & 0xFF;
        REG_CGDATA = (sprite_pal[i] >> 8) & 0xFF;
    }
}
```

### 4. Set Sprite Properties

```c
static void oam_set(u8 id, u16 x, u8 y, u8 tile, u8 attr) {
    u16 idx = id * 4;
    oam_lo[idx + 0] = x & 0xFF;   /* X low byte */
    oam_lo[idx + 1] = y;          /* Y position */
    oam_lo[idx + 2] = tile;       /* Tile number */
    oam_lo[idx + 3] = attr;       /* Attributes */

    /* High table: X bit 8 and size */
    u8 hi_idx = id >> 2;
    u8 hi_shift = (id & 3) * 2;
    oam_hi[hi_idx] |= (0x02 << hi_shift);  /* Large size */
}
```

### 5. Read Controller

```c
static u16 read_joypad(void) {
    while (REG_HVBJOY & 0x01) {}  /* Wait for auto-read */
    return ((u16)REG_JOY1H << 8) | REG_JOY1L;
}

/* In main loop: */
if (joy & KEY_LEFT)  sprite_x--;
if (joy & KEY_RIGHT) sprite_x++;
```

### 6. Update OAM

```c
static void oam_update(void) {
    REG_OAMADDL = 0;
    REG_OAMADDH = 0;

    for (i = 0; i < 544; i++) {
        REG_OAMDATA = oam_buffer[i];
    }
}
```

---

## Using gfx2snes

Convert any PNG to SNES sprite format:

```bash
# 4bpp sprite (16 colors)
gfx2snes -b 4 -c sprite.png sprite.h

# 2bpp sprite (4 colors)
gfx2snes -b 2 -c sprite.png sprite.h
```

### Image Requirements

- Dimensions: multiples of 8 pixels
- For 16×16 sprites: use 16×16 PNG
- Maximum 16 colors for 4bpp
- Color 0 = transparent

---

## Build and Run

```bash
cd examples/graphics/1_sprite
make clean && make

# Test with emulator
/path/to/Mesen sprite.sfc
```

**Controls:**
- D-pad: Move sprite

---

## Exercises

### Exercise 1: Change Sprite Speed
Make the sprite move 2 pixels per frame instead of 1.

```c
if (joy & KEY_LEFT)  sprite_x -= 2;
```

### Exercise 2: Screen Boundaries
Add code to prevent the sprite from leaving the screen.

**Hint:**
```c
if (sprite_x < 0) sprite_x = 0;
if (sprite_x > 240) sprite_x = 240;  /* 256 - 16 */
```

### Exercise 3: Create Your Own Sprite
1. Draw a 16×16 character in an image editor
2. Save as PNG with ≤16 colors
3. Replace `assets/sprite.png`
4. Rebuild and test

### Exercise 4: Multiple Sprites
Display a second sprite and control both:
- Player 1: D-pad
- Player 2: ABXY buttons as directions

---

## Common Issues

### Sprite doesn't appear
- Check Y isn't 240 (hidden)
- Verify `REG_TM = 0x10` (sprites enabled)
- Ensure palette is loaded to 128+

### Sprite looks garbled
- Verify tile layout matches VRAM expectations
- Check that gfx2snes used `-b 4` for 4bpp

### Controller doesn't work
- Enable auto-joypad: `REG_NMITIMEN = 0x81`
- Wait for auto-read to complete before reading

---

## Key Registers

| Register | Address | Purpose |
|----------|---------|---------|
| OBJSEL | $2101 | Sprite size and base |
| OAMADDL/H | $2102-3 | OAM address |
| OAMDATA | $2104 | OAM data write |
| TM | $212C | Enable sprites (bit 4) |
| JOY1L/H | $4218-9 | Joypad 1 state |

---

## Files in This Example

| File | Purpose |
|------|---------|
| `main.c` | Game logic (self-contained) |
| `assets/sprite.png` | Sprite artwork |
| `sprite.h` | Generated tile data |
| `Makefile` | Build + asset conversion |

---

## What's Next?

Now you can display and move a sprite! Next lessons will cover:
- **Animation:** Multiple frames, timing
- **Backgrounds:** Tilemaps, scrolling
- **Collision:** Detecting sprite interactions

**Next:** 2. Animation *(coming soon)* →
