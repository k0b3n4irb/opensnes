# Graphics & Backgrounds Tutorial {#tutorial_graphics}

This tutorial covers SNES graphics fundamentals including video modes, backgrounds, and tilemaps.

## SNES Video Modes

The SNES has 8 video modes (0-7). OpenSNES primarily uses:

| Mode | BG1 | BG2 | BG3 | BG4 | Best For |
|------|-----|-----|-----|-----|----------|
| **0** | 4 colors | 4 colors | 4 colors | 4 colors | Text, simple UI |
| **1** | 16 colors | 16 colors | 4 colors | - | Most games |
| **7** | 256 colors (rotatable) | - | - | - | Racing, rotation effects |

## Setting Up Graphics

### Initialize Display

```c
#include <snes.h>

int main(void) {
    // Initialize console (sets up display, enables NMI)
    consoleInit();

    // Set video mode 1 (most common for games)
    setMode(BGMODE_MODE1);

    // Enable BG1 on main screen
    REG_TM = TM_BG1;

    // Turn on display
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
```

## Loading Tiles to VRAM

Tiles are 8x8 pixel graphics stored in VRAM. For 4bpp (16 color) tiles:

```c
// Tile data (32 bytes per 8x8 tile for 4bpp)
const u8 my_tiles[] = {
    // Plane 0-1 (16 bytes)
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    // Plane 2-3 (16 bytes)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void load_tiles(void) {
    u16 i;

    // Set VRAM address (tiles at $1000)
    REG_VMAIN = 0x80;  // Increment on high byte
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x10;  // $1000

    // Copy tile data
    for (i = 0; i < sizeof(my_tiles); i += 2) {
        REG_VMDATAL = my_tiles[i];
        REG_VMDATAH = my_tiles[i + 1];
    }
}
```

## Setting Up Tilemaps

Tilemaps define which tiles appear where on screen. Each entry is 2 bytes:

```
Bits: vhopppcc cccccccc
  v = vertical flip
  h = horizontal flip
  o = priority
  ppp = palette (0-7)
  cccccccccc = tile number (0-1023)
```

```c
void setup_tilemap(void) {
    u16 x, y;

    // Set VRAM address for tilemap ($0400)
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;

    // Fill 32x32 tilemap
    for (y = 0; y < 32; y++) {
        for (x = 0; x < 32; x++) {
            REG_VMDATAL = 0x01;  // Tile number 1
            REG_VMDATAH = 0x00;  // No flip, palette 0
        }
    }
}
```

## Setting Palettes

SNES uses 15-bit color (RGB555):

```c
void setup_palette(void) {
    // Set CGRAM address (palette 0)
    REG_CGADD = 0;

    // Color 0: Black (transparent for BG)
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x00;

    // Color 1: White
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    // Color 2: Red
    REG_CGDATA = 0x1F;
    REG_CGDATA = 0x00;

    // Color 3: Green
    REG_CGDATA = 0xE0;
    REG_CGDATA = 0x03;

    // Color 4: Blue
    REG_CGDATA = 0x00;
    REG_CGDATA = 0x7C;
}
```

## Background Scrolling

```c
s16 scroll_x = 0;
s16 scroll_y = 0;

void update_scroll(void) {
    // Write scroll registers (write twice for 16-bit value)
    REG_BG1HOFS = (u8)scroll_x;
    REG_BG1HOFS = (u8)(scroll_x >> 8);
    REG_BG1VOFS = (u8)scroll_y;
    REG_BG1VOFS = (u8)(scroll_y >> 8);
}
```

## Parallax Scrolling

For depth effect, scroll backgrounds at different speeds:

```c
// In main loop:
scroll_x++;

// BG1 (foreground) - full speed
REG_BG1HOFS = (u8)scroll_x;
REG_BG1HOFS = (u8)(scroll_x >> 8);

// BG2 (background) - half speed
REG_BG2HOFS = (u8)(scroll_x >> 1);
REG_BG2HOFS = (u8)((scroll_x >> 1) >> 8);
```

## Using gfx4snes

Convert PNG images to SNES format using gfx4snes (based on PVSnesLib):

```bash
# Convert 8x8 tiles with palette output
gfx4snes -s 8 -p -i tiles.png
# Output: tiles.pic (tile data), tiles.pal (palette)

# Convert 16x16 sprites with palette
gfx4snes -s 16 -p -i sprites.png

# Convert with metasprite definition (for animation)
gfx4snes -s 16 -p -T -X 32 -Y 48 -i character.png
# Output: character.pic, character.pal, character_meta.inc
```

Include the generated data in your assembly file:
```asm
sprite_tiles:
.incbin "res/sprites.pic"
sprite_tiles_end:

sprite_pal:
.incbin "res/sprites.pal"
sprite_pal_end:
```

## Example: Complete Background Setup

See `examples/graphics/scrolling/` for a complete parallax scrolling example.

## Next Steps

- @ref tutorial_sprites "Sprites & Animation"
- @ref mode7.h "Mode 7 Reference"
