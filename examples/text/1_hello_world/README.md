# Lesson 1: Hello World

Your first SNES ROM! Display "HELLO WORLD" on screen.

## Learning Objectives

After this lesson, you will understand:
- ✅ How VRAM stores tile graphics
- ✅ How tilemaps create the display
- ✅ Basic PPU register setup
- ✅ The ROM build process

## Prerequisites

- OpenSNES SDK installed
- Basic C knowledge
- A SNES emulator (we recommend Mesen2)

---

## The Challenge

Display this on screen:

```
┌────────────────────────────────────┐
│                                    │
│                                    │
│          HELLO WORLD!              │
│                                    │
│                                    │
└────────────────────────────────────┘
```

Seems simple? On the SNES, there's no `printf()`. We need to:
1. Create font graphics (tiles)
2. Load them to VRAM
3. Create a tilemap that spells our message
4. Configure the PPU to display it

---

## Step-by-Step Explanation

### Step 1: Define the Font

We create a minimal font with just the letters we need:

```c
/* Each letter is 8 bytes (1bpp bitmap) */
static const u8 font_1bpp[] = {
    /* H */  0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00,
    /* E */  0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00,
    /* L */  0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00,
    /* O */  0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00,
    /* ... */
};
```

Each byte represents one row of pixels. For example, `0x66` = `01100110` in binary:

```
0x66 = . # # . . # # .
       ↑           ↑
   H's left leg   right leg
```

### Step 2: Load Tiles to VRAM

VRAM stores graphics. We write our font tiles there:

```c
/* Set VRAM address to 0 */
REG_VMAIN = 0x80;    /* Auto-increment after high byte write */
REG_VMADDL = 0x00;   /* Address low byte */
REG_VMADDH = 0x00;   /* Address high byte */

/* Write each tile */
for (i = 0; i < num_tiles; i++) {
    for (j = 0; j < 8; j++) {
        REG_VMDATAL = font_1bpp[i * 8 + j];  /* Bitplane 0 */
        REG_VMDATAH = 0x00;                   /* Bitplane 1 */
    }
}
```

**Why two writes per row?** SNES tiles use planar format. In 2bpp mode:
- Bitplane 0: Least significant bit of each pixel
- Bitplane 1: Most significant bit of each pixel

Since we want a simple 2-color font (background + text), we only use bitplane 0.

### Step 3: Set Up the Tilemap

The tilemap tells the PPU which tile to show at each position:

```c
/* Tilemap at VRAM $0800 */
u16 addr = 0x0800 + (y * 32) + x;  /* 32 tiles per row */

REG_VMAIN = 0x80;
REG_VMADDL = addr & 0xFF;
REG_VMADDH = addr >> 8;

/* Write tile numbers */
while (*str) {
    u8 tile = char_to_tile(*str);
    REG_VMDATAL = tile;   /* Tile number */
    REG_VMDATAH = 0x00;   /* Attributes */
    str++;
}
```

### Step 4: Configure the PPU

```c
/* Mode 0: Four 2bpp backgrounds */
REG_BGMODE = 0x00;

/* BG1 tilemap at $0800, tiles at $0000 */
REG_BG1SC = 0x08;     /* ($0800 / $400) << 2 = 0x08 */
REG_BG12NBA = 0x00;   /* Tiles at $0000 */

/* Enable BG1 on main screen */
REG_TM = 0x01;

/* Turn on display */
REG_INIDISP = 0x0F;   /* Full brightness */
```

---

## Key Concepts

### VRAM Layout

```
$0000-$07FF: Tile data (our font - 64 tiles max)
$0800-$0FFF: BG1 tilemap (32×32 entries)
```

### Tilemap Entry Format

Each tilemap entry is 2 bytes:
```
Low byte:  TTTTTTTT  (tile number 0-255)
High byte: VHOPPPCC
           V = Vertical flip
           H = Horizontal flip
           O = Priority
           PPP = Palette
           CC = Tile bits 9-8 (for >256 tiles)
```

### Mode 0

- 4 background layers (BG1-BG4)
- Each layer: 2bpp (4 colors)
- Each layer has its own 4-color palette
- Good for: Text, simple graphics, UI

---

## Build and Run

```bash
cd examples/text/1_hello_world
make clean && make

# Run in emulator
/path/to/Mesen hello_world.sfc
```

## Exercises

### Exercise 1: Change the Message
Edit `main.c` to display your name instead of "HELLO WORLD".

**Hint:** You'll need to add letters to the font if they're not included.

### Exercise 2: Change Colors
Modify `set_palette()` to use different colors.

**Hint:** SNES colors are 15-bit BGR (5 bits each):
```c
/* Color format: 0bBBBBBGGGGGRRRRR */
u16 red = 0x001F;    /* R=31, G=0, B=0 */
u16 green = 0x03E0;  /* R=0, G=31, B=0 */
u16 blue = 0x7C00;   /* R=0, G=0, B=31 */
```

### Exercise 3: Add a Second Line
Display two lines of text.

**Hint:** Call `print_at()` with a different Y coordinate.

---

## Files in This Example

| File | Purpose |
|------|---------|
| `main.c` | All the code (self-contained) |
| `crt0.asm` | Startup code (CPU init) |
| `hdr.asm` | ROM header (memory map) |
| `Makefile` | Build script |

---

## What's Next?

In the next lesson, we'll:
- Use an external font image
- Learn the `font2snes` tool
- Display the full ASCII character set

**Next:** [2. Custom Font](../2_custom_font/) →
