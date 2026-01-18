# Lesson 1: Hello World

Your first SNES ROM! Display "HELLO WORLD!" on screen.

## Learning Objectives

After this lesson, you will understand:
- How VRAM stores tile graphics (2bpp format)
- How tilemaps create the display
- Basic PPU register setup
- The ROM build process with OpenSNES

## Prerequisites

- OpenSNES SDK installed
- Basic C knowledge
- A SNES emulator (we recommend Mesen2)

---

## The Challenge

Display this on screen:

```
+------------------------------------+
|                                    |
|                                    |
|          HELLO WORLD!              |
|                                    |
|                                    |
+------------------------------------+
```

Seems simple? On the SNES, there's no `printf()`. We need to:
1. Create font graphics (tiles) in 2bpp format
2. Load them to VRAM
3. Create a tilemap that spells our message
4. Configure the PPU to display it

---

## Code Type

**Mixed C + Direct Register Access**

| Component | Type |
|-----------|------|
| Hardware init | Library (`consoleInit()`) |
| Video mode | Library (`setMode()`) |
| Screen enable | Library (`setScreenOn()`) |
| VBlank sync | Library (`WaitForVBlank()`) |
| Font tiles | C const array (embedded 2bpp) |
| VRAM writes | Direct register access |
| Palette | Direct register access |

---

## Step-by-Step Explanation

### Step 1: Hardware Initialization

```c
#include <snes.h>

int main(void) {
    consoleInit();              /* Initialize SNES hardware */
    setMode(BGMODE_MODE0);      /* Mode 0: 4 BG layers, all 2bpp */
```

`consoleInit()` sets up the SNES hardware to a known state:
- Disables interrupts during setup
- Clears all PPU registers
- Sets up default timing

`setMode()` configures Mode 0, which gives us 4 background layers at 2 bits per pixel (4 colors each).

### Step 2: Define the Font (2bpp Format)

```c
static const u8 font_tiles[] = {
    /* Tile 0: Space (blank) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* Tile 1: H */
    0x66, 0x00, 0x66, 0x00, 0x66, 0x00, 0x7E, 0x00,
    0x66, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00,
    /* ... more tiles ... */
};
```

Each 8x8 tile is 16 bytes in 2bpp format. Every pair of bytes represents one row:
- First byte: Bitplane 0 (LSB of each pixel)
- Second byte: Bitplane 1 (MSB of each pixel)

For a simple font, we only use bitplane 0, so colors alternate between 0 and 1.

### Step 3: Configure Background Layer

```c
REG_BG1SC = 0x04;   /* Tilemap at VRAM $0400, 32x32 tiles */
REG_BG12NBA = 0x00; /* BG1 tiles at VRAM $0000 */
```

| Register | Value | Meaning |
|----------|-------|---------|
| BG1SC | $04 | Tilemap at VRAM $0400, 32x32 tiles |
| BG12NBA | $00 | BG1 tiles at VRAM $0000 |

### Step 4: Upload Font Tiles to VRAM

```c
REG_VMAIN = 0x80;   /* Auto-increment after high byte write */
REG_VMADDL = 0x00;  /* VRAM address = $0000 */
REG_VMADDH = 0x00;

for (i = 0; i < 144; i += 2) {
    REG_VMDATAL = font_tiles[i];     /* Bitplane 0 */
    REG_VMDATAH = font_tiles[i + 1]; /* Bitplane 1 */
}
```

`REG_VMAIN = 0x80` enables auto-increment mode after writing to the high byte, allowing sequential VRAM writes.

### Step 5: Set Up the Palette

```c
REG_CGADD = 0;      /* Start at color 0 */
REG_CGDATA = 0x00;  /* Color 0: Dark blue (low byte) */
REG_CGDATA = 0x28;  /* Color 0: Dark blue (high byte) */
REG_CGDATA = 0xFF;  /* Color 1: White (low byte) */
REG_CGDATA = 0x7F;  /* Color 1: White (high byte) */
```

SNES colors are 15-bit BGR format: `0BBBBBGG GGGRRRRR`

### Step 6: Write the Message to the Tilemap

The message is encoded as tile indices:

```c
static const u8 message[] = {
    1, 2, 3, 3, 4,  /* HELLO */
    0,              /* space */
    5, 4, 6, 3, 7,  /* WORLD */
    8,              /* ! */
    0xFF            /* end marker */
};
```

Written to VRAM at tilemap position (row 14, column 10):

```c
addr = 0x05CA;  /* 0x0400 + (14 * 32 + 10) */
REG_VMADDL = addr & 0xFF;
REG_VMADDH = addr >> 8;

i = 0;
while (1) {
    tile = message[i];
    if (tile == 0xFF) break;
    REG_VMDATAL = tile;   /* Tile number */
    REG_VMDATAH = 0;      /* Attributes (palette 0, no flip) */
    i++;
}
```

### Step 7: Enable Display and Main Loop

```c
REG_TM = TM_BG1;  /* Enable BG1 on main screen */
setScreenOn();    /* Turn on display */

while (1) {
    WaitForVBlank();  /* Sync to 60Hz refresh */
}
```

---

## Key Concepts

### VRAM Layout

```
$0000-$01FF: Font tiles (9 tiles x 16 bytes = 144 bytes)
$0400-$07FF: BG1 tilemap (32x32 = 1024 words)
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

This produces `hello_world.sfc` (128KB ROM).

---

## Exercises

### Exercise 1: Change the Message
Edit `main.c` to display your name instead of "HELLO WORLD!".

**Hint:** You'll need to add font tiles for letters not already included (H, E, L, O, W, R, D, !).

### Exercise 2: Change Colors
Modify the palette writes to use different colors.

**Hint:** SNES colors are 15-bit BGR (5 bits each):
```c
/* Color format: 0bBBBBBGGGGGRRRRR */
u16 red = 0x001F;    /* R=31, G=0, B=0 */
u16 green = 0x03E0;  /* R=0, G=31, B=0 */
u16 blue = 0x7C00;   /* R=0, G=0, B=31 */
```

### Exercise 3: Add a Second Line
Display two lines of text by writing to two different tilemap addresses.

**Hint:** Row N is at offset `(N * 32)` from the tilemap base.

---

## Files in This Example

| File | Purpose |
|------|---------|
| `main.c` | All the code (self-contained) |
| `Makefile` | Build configuration |

Generated files (after build):
| File | Purpose |
|------|---------|
| `combined.asm` | Generated assembly from C code |
| `hello_world.sfc` | Output ROM file |
| `hello_world.sym` | Symbol table for debugging |

---

## What's Next?

In the next lesson, we'll:
- Use an external font image file
- Learn the `font2snes` tool
- Display more characters

**Next:** [2. Custom Font](../2_custom_font/)

---

## License

CC0 (Public Domain)
