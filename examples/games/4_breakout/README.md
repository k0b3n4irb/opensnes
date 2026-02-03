# Breakout

A complete breakout game demonstrating core SNES game development concepts.

Based on the SNES SDK game by Ulrich Hecht, ported to PVSnesLib by alekmaul,
and ported to OpenSNES.

## Screenshots

```
+------------------------+
|  HISCORE     SCORE     |
|   50000      00000     |
|  LIVES  4   LEVEL  1   |
|                        |
|  [====] [====] [====]  |  <- Bricks (8 colors)
|  [====] [====] [====]  |
|  [====] [====] [====]  |
|                        |
|         ( )            |  <- Ball
|                        |
|      [========]        |  <- Paddle
+------------------------+
```

## Controls

| Button | Action |
|--------|--------|
| D-Pad Left/Right | Move paddle |
| A | Move paddle faster |
| Start | Pause / Unpause |

## SNES Concepts Demonstrated

### 1. Video Mode Configuration (Mode 1)

This game uses **Mode 1**, the most common SNES video mode:

```
Mode 1 Layout:
- BG1: 4bpp (16 colors) - Game playfield with border and bricks
- BG3: 2bpp (4 colors)  - Scrolling background pattern
- Sprites: Ball and paddle
```

Mode 1 is used by Super Mario World, Street Fighter II, and most SNES games.

### 2. VRAM Layout and Tilemap Overlap

The game uses an intentional VRAM overlap to save memory:

```
VRAM Address Map:
0x0000-0x07FF: BG1 tilemap (blockmap) - 2KB
0x0400-0x0BFF: BG3 tilemap (backmap)  - 2KB  <- OVERLAPS with BG1!
0x1000-0x1EFF: BG1/BG3 tile graphics (tiles1)
0x2000-0x224F: Sprite tile graphics (tiles2)
```

**Why overlap?** BG1 only uses rows 0-15 of its 32x32 tilemap. BG3 can
reuse rows 16-31 of BG1's space, saving 1KB of VRAM.

**Critical lesson learned:** When tilemaps overlap, ALL related DMA
transfers must complete in the SAME VBlank. If you split them:

```c
// BAD - causes visual corruption!
WaitForVBlank();
dmaCopyVram(blockmap, 0x0000, 0x800);  // Writes 0x0000-0x07FF
WaitForVBlank();                        // Frame rendered with corrupt BG3!
dmaCopyVram(backmap, 0x0400, 0x800);   // Writes 0x0400-0x0BFF

// GOOD - atomic update
WaitForVBlank();
dmaCopyVram(blockmap, 0x0000, 0x800);
dmaCopyVram(backmap, 0x0400, 0x800);   // Both in same VBlank
```

### 3. Sprite Organization

The game uses 10 sprites for ball, paddle, and shadows:

```
Sprite Layout:
- Sprite 0:    (skipped - see note below)
- Sprite 1:    Ball
- Sprites 2-5: Paddle (4 tiles: left cap, 2x middle, right cap)
- Sprite 6:    Ball shadow
- Sprites 7-10: Paddle shadow

Tile Numbers (in secondary name table, accessed via tile | 256):
- Tiles 15-17: Paddle pieces
- Tiles 18-19: Paddle shadow pieces
- Tile 20:     Ball
- Tile 21:     Ball shadow
```

**Why skip sprite 0?** There's a known issue where sprite 0 data gets
corrupted, possibly related to WRAM mirroring. Using sprites 1-10
avoids this.

### 4. Tilemap-Based Brick System

Bricks are NOT sprites - they're background tiles that get modified:

```c
// Each brick is 2 tiles wide (16 pixels)
// Tilemap entry format: tile_number + (palette << 10)
blockmap[offset] = 13 + (color << 10);  // Left half
blockmap[offset+1] = 14 + (color << 10);  // Right half

// 8 brick colors = 8 palettes (0-7)
// Palette bits are in bits 10-12 of tilemap entry
```

When a brick is destroyed:
1. Set tilemap entries to 0 (transparent)
2. Update the background layer shadow tiles
3. DMA the modified tilemap to VRAM

### 5. Palette Cycling for Level Progression

Each level changes the background color by copying different palette
data to CGRAM colors 8-15:

```c
// backpal.dat contains 7 color sets (7 x 16 bytes)
// Each set has 8 colors (8 x 2 bytes per color)
mycopy((u8 *)pal + 16, backpal + color * 16, 0x10);

// Then upload entire palette to CGRAM
dmaCopyCGram((u8 *)pal, 0, 256 * 2);
```

### 6. RAM Buffer Pattern

Game data is copied from ROM to RAM buffers for runtime modification:

```c
// ROM data (read-only)
extern u8 bg1map[];     // Original tilemap in ROM
extern u8 palette[];    // Original palette in ROM

// RAM buffers (modifiable)
extern u16 blockmap[];  // Working copy of tilemap
extern u16 pal[];       // Working copy of palette
```

This pattern is essential because:
- Tilemaps change as bricks are destroyed
- Palette changes between levels
- Score/lives display updates the tilemap

### 7. Collision Detection

Ball-brick collision uses grid-based detection:

```c
// Convert ball pixel position to brick grid position
bx = (pos_x - 14) >> 4;  // Divide by 16 (brick width)
by = (pos_y - 14) >> 3;  // Divide by 8 (brick height)

// Calculate brick index: 10 bricks per row
b = bx + by * 10 - 10;

// Check if brick exists (value < 8 means brick present)
if (blocks[b] != 8) {
    remove_brick();
}
```

Ball-paddle collision determines bounce angle based on hit position:

```c
k = (pos_x - px) / 7;  // 0-3 based on where ball hits paddle
switch (k) {
    case 0: vel_x = -2; vel_y = -1; break;  // Sharp left
    case 1: vel_x = -1; vel_y = -2; break;  // Slight left
    case 2: vel_x =  1; vel_y = -2; break;  // Slight right
    default: vel_x = 2; vel_y = -1; break;  // Sharp right
}
```

## Memory Layout

```
Bank 0 RAM ($00:0000-$1FFF):
  $0000-$02FF: Compiler registers, stack
  $0300-$051F: OAM buffer (544 bytes)
  $0800-$0FFF: blockmap (2KB tilemap buffer)
  $1000-$17FF: backmap (2KB tilemap buffer)
  $1800-$19FF: pal (512 bytes palette buffer)

Bank 0 ROM ($00:8000-$FFFF):
  Code, tile data, tilemap data, palette data
```

## Build

```bash
make clean && make
```

## Files

| File | Purpose |
|------|---------|
| `main.c` | Game logic |
| `data.asm` | Asset includes and RAM buffer definitions |
| `res/tiles1.pic` | Background tiles (border, bricks, font) |
| `res/tiles2.pic` | Sprite tiles (ball, paddle) |
| `res/bg1map.dat` | BG1 tilemap (playfield border) |
| `res/bg2map.dat` | BG3 tilemap (4 background patterns) |
| `res/palette.dat` | Main palette (256 colors) |
| `res/backpal.dat` | Level color sets (7 x 8 colors) |

## Lessons Learned

### DMA Timing is Critical

The most important lesson from this port: **overlapping VRAM regions
must be updated atomically**. Splitting DMA transfers "to be safe"
actually caused a visible pink flash during level transitions.

PVSnesLib does 4.5KB+ in a single VBlank routinely. The theoretical
6KB limit has margin, and consistency matters more than caution.

### Reference Implementations Matter

When something doesn't work, comparing against PVSnesLib's working
implementation reveals timing and ordering issues that aren't obvious
from reading specifications.

### Compiler Workarounds

Several patterns avoid QBE compiler issues:

```c
// Use temp var for negation
s16 neg_vx = -vel_x;
vel_x = neg_vx;
// Instead of: vel_x = -vel_x;

// Use explicit addition instead of +=
s16 new_x = pos_x + vel_x;
pos_x = new_x;
// Instead of: pos_x += vel_x;

// Use separate increment for array indexing
a = blocks[b];
b++;
// Instead of: a = blocks[b++];
```

## Credits

- Original game: Ulrich Hecht (SNES SDK)
- PVSnesLib port: alekmaul
- OpenSNES port: OpenSNES team
