# PVSnesLib Sprite System Reference

This document provides comprehensive technical documentation of PVSnesLib's sprite system,
verified against the actual source code. Use this as the authoritative reference for
implementing compatible sprite functionality in OpenSNES.

## Table of Contents

1. [OAM Hardware Overview](#1-oam-hardware-overview)
2. [Basic Sprite Functions](#2-basic-sprite-functions)
3. [Dynamic Sprite System](#3-dynamic-sprite-system)
4. [Meta Sprites](#4-meta-sprites)
5. [Lookup Tables](#5-lookup-tables)
6. [VRAM Organization](#6-vram-organization)
7. [Implementation Details](#7-implementation-details)
8. [Common Patterns](#8-common-patterns)

---

## 1. OAM Hardware Overview

### 1.1 SNES Sprite Limits

- **128 sprites** maximum on screen
- **32 sprites per scanline** (hardware limit, causes flickering if exceeded)
- **34 tiles per scanline** (272 pixels maximum)
- **8 palettes** of 16 colors each (colors 128-255 in CGRAM)

### 1.2 Sprite Size Modes (OBJSEL bits 7-5)

| Mode | Value | Small  | Large  | Constant        |
|------|-------|--------|--------|-----------------|
| 0    | $00   | 8x8    | 16x16  | OBJ_SIZE8_L16   |
| 1    | $20   | 8x8    | 32x32  | OBJ_SIZE8_L32   |
| 2    | $40   | 8x8    | 64x64  | OBJ_SIZE8_L64   |
| 3    | $60   | 16x16  | 32x32  | OBJ_SIZE16_L32  |
| 4    | $80   | 16x16  | 64x64  | OBJ_SIZE16_L64  |
| 5    | $A0   | 32x32  | 64x64  | OBJ_SIZE32_L64  |

### 1.3 OAM Memory Layout (544 bytes total)

#### Low Table (bytes 0-511): 128 entries × 4 bytes

| Byte | Bits       | Description                                       |
|------|------------|---------------------------------------------------|
| 0    | xxxxxxxx   | X coordinate (bits 0-7)                           |
| 1    | yyyyyyyy   | Y coordinate (all 8 bits)                         |
| 2    | cccccccc   | Tile number (bits 0-7)                            |
| 3    | vhoopppc   | v=vflip, h=hflip, oo=priority, ppp=palette, c=tile MSB |

#### High Table (bytes 512-543): 2 bits per sprite

| Bits | Description                              |
|------|------------------------------------------|
| Even | X coordinate bit 8 (0=onscreen, 1=+256)  |
| Odd  | Size select (0=small, 1=large)           |

### 1.4 OBJSEL Register ($2101)

```
Bit 7-5: OBJ Size (see table above)
Bit 4-3: Name Select (gap between tiles 0-255 and 256-511)
Bit 2-0: Name Base Address (in 16KB steps)
```

**Configuration formula**: `REG_OBJSEL = (vram_address >> 13) | size_mode`

---

## 2. Basic Sprite Functions

### 2.1 oamInit()

Initializes OAM, places all sprites off-screen.

```c
void oamInit(void);
```

**Called automatically by consoleInit().**

### 2.2 oamInitGfxSet()

Load sprite graphics and configure OAM.

```c
void oamInitGfxSet(
    u8 *tileSource,     // Pointer to tile graphics data
    u16 tileSize,       // Size in bytes (&end - &start)
    u8 *tilePalette,    // Pointer to palette data
    u16 paletteSize,    // Palette size (usually 32 bytes for 16 colors)
    u8 paletteEntry,    // Palette slot 0-7
    u16 vramAddr,       // VRAM destination (word address)
    u8 oamSize          // Size mode constant (OBJ_SIZE16_L32, etc.)
);
```

**Implementation**:
1. DMA tiles to VRAM at `vramAddr`
2. DMA palette to CGRAM at `128 + (paletteEntry * 16)`
3. Set OBJSEL to `(vramAddr >> 13) | oamSize`
4. Clear OAM buffer (all sprites hidden)

### 2.3 oamSet()

Set sprite attributes.

```c
void oamSet(
    u16 id,             // Sprite ID × 4 (0, 4, 8, 12...)
    s16 xspr,           // X position (-256 to 511)
    s16 yspr,           // Y position (0-255)
    u8 priority,        // Priority 0-3 (3 = highest)
    u8 hflip,           // Horizontal flip (0 or 1)
    u8 vflip,           // Vertical flip (0 or 1)
    u16 gfxoffset,      // Tile number
    u8 paletteoffset    // Palette number (0-7)
);
```

### 2.4 oamSetEx()

Set extended attributes (size and visibility).

```c
void oamSetEx(
    u16 id,             // Sprite ID × 4
    u8 size,            // OBJ_SMALL or OBJ_LARGE
    u8 hide             // OBJ_SHOW or OBJ_HIDE
);
```

### 2.5 oamSetXY()

Quick position update.

```c
void oamSetXY(u16 id, s16 x, s16 y);
```

---

## 3. Dynamic Sprite System

### 3.1 Purpose

The dynamic sprite system allows runtime VRAM uploads during VBlank, enabling:
- Many unique sprite frames without pre-loading all to VRAM
- Animation with automatic graphics management
- Efficient VRAM usage for complex games

### 3.2 t_sprites Structure (16 bytes per entry)

```c
typedef struct {
    s16 oamx;           // Offset 0-1: X position on screen
    s16 oamy;           // Offset 2-3: Y position on screen
    u16 oamframeid;     // Offset 4-5: Frame index in sprite sheet
    u8 oamattribute;    // Offset 6: Attributes (vhoopppc)
    u8 oamrefresh;      // Offset 7: Set to 1 to request VRAM upload
    u8 *oamgraphics;    // Offset 8-11: 24-bit pointer to graphics (addr + bank + pad)
    u16 _reserved1;     // Offset 12-13: Padding
    u16 _reserved2;     // Offset 14-15: Padding
} t_sprites;
```

**oambuffer**: Array of 128 t_sprites entries (2048 bytes total)

### 3.3 Attribute Byte Format (oamattribute)

```
Bit 7: V - Vertical flip
Bit 6: H - Horizontal flip
Bit 5-4: OO - Priority (0-3)
Bit 3-1: PPP - Palette number (0-7)
Bit 0: C - Tile number bit 8 (for 512-tile addressing)
```

**Macros**:
```c
#define OBJ_PRIO(p)  ((p) << 4)    // Priority 0-3
#define OBJ_PAL(p)   ((p) << 1)    // Palette 0-7
```

### 3.4 Core Functions

#### oamInitDynamicSprite()

```c
void oamInitDynamicSprite(
    u16 gfxsp0adr,      // VRAM address for LARGE sprites (e.g., 0x0000)
    u16 gfxsp1adr,      // VRAM address for SMALL sprites (e.g., 0x1000)
    u16 oamsp0init,     // Starting OAM slot for large sprites
    u16 oamsp1init,     // Starting OAM slot for small sprites
    u8 oamsize          // Size mode (OBJ_SIZE16_L32, etc.)
);
```

**Implementation details**:
1. Clear VRAM upload queue (768 bytes)
2. Reset all counters to 0
3. Store VRAM addresses:
   - `spr0addrgfx` = gfxsp0adr (large sprites)
   - `spr1addrgfx` = gfxsp1adr (small sprites)
4. Determine 16x16 sprite location:
   - If `oamsize == OBJ_SIZE16_L32`: `spr16addrgfx = spr1addrgfx` (16x16 is SMALL)
   - Otherwise: `spr16addrgfx = spr0addrgfx` (16x16 is LARGE)
5. Call `oamInit()` to clear OAM
6. Set OBJSEL register

#### oamDynamic16Draw() / oamDynamic32Draw() / oamDynamic8Draw()

```c
void oamDynamic16Draw(u16 id);  // Draw 16x16 sprite
void oamDynamic32Draw(u16 id);  // Draw 32x32 sprite
void oamDynamic8Draw(u16 id);   // Draw 8x8 sprite
```

**Implementation flow**:
1. Convert `id` to oambuffer offset: `y = id * 16`
2. If `oamrefresh == 1`:
   - Clear refresh flag
   - Calculate source: `lkup*oamS[frameid] + oamgraphics`
   - Add to VRAM upload queue
3. Get current OAM slot from `oamnumberperframe`
4. Look up tile ID from `lkup*idT` or `lkup*idT0`
5. Write position, tile, attributes to oamMemory
6. Update high table (size bit, X MSB)
7. Increment sprite counter

#### oamInitDynamicSpriteEndFrame()

```c
void oamInitDynamicSpriteEndFrame(void);
```

Called after drawing all sprites each frame:
1. Hide sprites that were visible last frame but not drawn this frame
2. Save current sprite count for next frame comparison
3. Reset sprite counters for next frame

#### oamVramQueueUpdate()

```c
void oamVramQueueUpdate(void);
```

Process VRAM upload queue during VBlank:
1. Maximum 7 sprites per frame (42 bytes of queue data)
2. DMA graphics from ROM to VRAM
3. Remaining entries cascade to next frame

### 3.5 Frame Update Sequence

**CRITICAL ORDER**:

```c
while (1) {
    // 1. Update sprite state
    for (i = 0; i < num_sprites; i++) {
        oambuffer[i].oamx = new_x;
        oambuffer[i].oamy = new_y;

        // Animation
        if (animating) {
            oambuffer[i].oamframeid = new_frame;
            oambuffer[i].oamrefresh = 1;  // Request VRAM upload
        }

        // 2. Draw each sprite
        oamDynamic16Draw(i);
    }

    // 3. End frame processing
    oamInitDynamicSpriteEndFrame();

    // 4. Wait for VBlank
    WaitForVBlank();

    // 5. Process VRAM uploads (during VBlank)
    oamVramQueueUpdate();
}
```

---

## 4. Meta Sprites

### 4.1 Purpose

Compose large characters from multiple hardware sprites (e.g., a 32x48 character
made of six 16x16 sprites).

### 4.2 t_metasprite Structure

```c
typedef struct {
    s16 dx;         // X offset from origin
    s16 dy;         // Y offset from origin
    u16 dtile;      // Tile index
    u8 props;       // Properties (palette, flip, priority)
} t_metasprite;
```

**Terminator**: `dx = -128` marks end of metasprite definition.

### 4.3 Functions

```c
// Static metasprite (graphics pre-loaded to VRAM)
void oamMetaDraw16(
    u8 id,              // Starting OAM slot
    u16 x, u16 y,       // Screen position
    u8 *metasprite,     // Metasprite definition array
    u8 size,            // OBJ_SMALL or OBJ_LARGE
    u16 vram_offset     // VRAM address of tiles
);

// Dynamic metasprite (uploads graphics to VRAM)
void oamMetaDrawDyn16(
    u8 id,
    u16 x, u16 y,
    u8 *metasprite,
    u8 *tiles,          // Pointer to tile graphics
    u8 size
);
```

### 4.4 Graphics Conversion

```bash
gfx4snes -s 16 -o 16 -u 16 -T -X 32 -Y 48 -P 2 -i sprite.png
```

- `-T`: Generate metasprite structure
- `-X`, `-Y`: Composite dimensions
- `-P`: Priority levels

---

## 5. Lookup Tables

### 5.1 Purpose

Pre-computed tables map between:
- **frameid** → source offset in sprite sheet
- **sprite slot** → OAM tile number
- **sprite slot** → VRAM destination address

### 5.2 16x16 Sprites (64 max)

#### lkup16oamS - Source offsets in sprite sheet

Pattern: 8 columns × 8 rows, $40 column stride, $400 row stride

```
Row 0: $0000, $0040, $0080, $00C0, $0100, $0140, $0180, $01C0
Row 1: $0400, $0440, $0480, $04C0, $0500, $0540, $0580, $05C0
Row 2: $0800, $0840, $0880, $08C0, $0900, $0940, $0980, $09C0
...
```

**Formula**: `offset = (row * $400) + (col * $40)`

Each 16x16 sprite uses 128 bytes (64 bytes × 2 rows of tiles).

#### lkup16idT - Tile IDs (16x16 is SMALL mode)

Pattern: Base $0100, increment by 2 horizontally, by $20 vertically

```
Row 0: $0100, $0102, $0104, $0106, $0108, $010A, $010C, $010E
Row 1: $0120, $0122, $0124, $0126, $0128, $012A, $012C, $012E
...
```

Uses second character name table (tiles 256+).

#### lkup16idT0 - Tile IDs (16x16 is LARGE mode)

Pattern: Base $0000, increment by 2 horizontally, by $20 vertically

```
Row 0: $0000, $0002, $0004, $0006, $0008, $000A, $000C, $000E
Row 1: $0020, $0022, $0024, $0026, $0028, $002A, $002C, $002E
...
```

Uses first character name table (tiles 0-255).

#### lkup16idB - VRAM destination blocks

Pattern: $20 column stride, $200 row stride

```
Row 0: $0000, $0020, $0040, $0060, $0080, $00A0, $00C0, $00E0
Row 1: $0200, $0220, $0240, $0260, $0280, $02A0, $02C0, $02E0
...
```

### 5.3 32x32 Sprites (16 max)

#### lkup32oamS - Source offsets

```
$0000, $0080, $0100, $0180,
$0800, $0880, $0900, $0980,
$1000, $1080, $1100, $1180,
$1800, $1880, $1900, $1980
```

#### lkup32idT - Tile IDs

```
$0000, $0004, $0008, $000C,
$0040, $0044, $0048, $004C,
$0080, $0084, $0088, $008C,
$00C0, $00C4, $00C8, $00CC
```

#### lkup32idB - VRAM blocks

```
$0000, $0040, $0080, $00C0,
$0400, $0440, $0480, $04C0,
$0800, $0840, $0880, $08C0,
$0C00, $0C40, $0C80, $0CC0
```

### 5.4 8x8 Sprites (128 max)

- **lkup8oamS**: Linear $20 increments ($0000, $0020, $0040...)
- **lkup8idT**: Sequential from $0100 ($0100, $0101, $0102...)
- **lkup8idB**: Linear $10 increments ($0000, $0010, $0020...)

---

## 6. VRAM Organization

### 6.1 Standard Layout

```
$0000 - $0FFF: Large sprites (32x32, 64x64)
$1000 - $1FFF: Small sprites (8x8, 16x16)
$2000+:        Background tiles
```

### 6.2 Tile Number to VRAM Mapping

For 4bpp sprites: `VRAM_word_address = tile_number * 16`

| Tile Number | VRAM Word Address |
|-------------|-------------------|
| 0           | $0000             |
| 1           | $0010             |
| 16 ($10)    | $0100             |
| 256 ($100)  | $1000             |

### 6.3 SNES Sprite Tile Layout

Sprites larger than 8x8 are stored in **columns then rows** with a 16-tile row stride.

**16x16 sprite** (4 tiles in 2×2 pattern):
```
Tile 0: Top-left      (at tile N)
Tile 1: Top-right     (at tile N+1)
Tile 2: Bottom-left   (at tile N+16)
Tile 3: Bottom-right  (at tile N+17)
```

**32x32 sprite** (16 tiles in 4×4 pattern):
```
N+0  N+1  N+2  N+3
N+16 N+17 N+18 N+19
N+32 N+33 N+34 N+35
N+48 N+49 N+50 N+51
```

---

## 7. Implementation Details

### 7.1 Queue Entry Structure (6 bytes)

```
Offset 0-1: Source address (16-bit)
Offset 2:   Source bank
Offset 3-4: VRAM destination address
Offset 5:   Sprite type (OBJ_SPRITE32=1, OBJ_SPRITE16=2, OBJ_SPRITE8=4)
```

### 7.2 DMA Transfer Sizes

| Sprite Size | Bytes per Transfer | Transfers | Total  |
|-------------|-------------------|-----------|--------|
| 8x8         | 32 ($20)          | 1         | 32     |
| 16x16       | 64 ($40)          | 2         | 128    |
| 32x32       | 128 ($80)         | 4         | 512    |

### 7.3 VBlank Budget

- **Safe limit**: ~4KB per frame
- **Maximum 7 sprites** uploaded per frame (42 queue bytes)
- Excess entries cascade to next frame

### 7.4 Key Variables

| Variable            | Purpose                                    |
|---------------------|--------------------------------------------|
| spr0addrgfx         | VRAM base for LARGE sprites               |
| spr1addrgfx         | VRAM base for SMALL sprites               |
| spr16addrgfx        | VRAM base for 16x16 (depends on size mode)|
| oamnumberspr0       | Current slot counter for large sprites    |
| oamnumberspr1       | Current slot counter for small sprites    |
| oamnumberperframe   | OAM byte offset (increments by 4)         |
| oamqueuenumber      | Queue byte offset (increments by 6)       |

---

## 8. Common Patterns

### 8.1 Basic Static Sprite

```c
// Load graphics
oamInitGfxSet(sprite_tiles, tile_size, sprite_pal, 32, 0, 0x0000, OBJ_SIZE16_L32);

// Set sprite
oamSet(0, 100, 100, 3, 0, 0, 0, 0);  // ID=0, pos=(100,100), prio=3, tile=0
oamSetEx(0, OBJ_SMALL, OBJ_SHOW);

// Main loop
while (1) {
    WaitForVBlank();
}
```

### 8.2 Dynamic Animated Sprite

```c
// Load palette only
dmaCopyCGram(sprite_pal, 128, 32);

// Initialize dynamic engine
oamInitDynamicSprite(0x0000, 0x0000, 0, 0, OBJ_SIZE16_L32);

// Set up oambuffer
oambuffer[0].oamx = 100;
oambuffer[0].oamy = 100;
oambuffer[0].oamframeid = 0;
oambuffer[0].oamattribute = OBJ_PRIO(2) | OBJ_PAL(0);
oambuffer[0].oamrefresh = 1;
oambuffer[0].oamgraphics = (u16)sprite_tiles;
oambuffer[0].oamgfxbank = 0;  // Bank 0 for LoROM

// Main loop
while (1) {
    // Animate
    oambuffer[0].oamframeid = (oambuffer[0].oamframeid + 1) % NUM_FRAMES;
    oambuffer[0].oamrefresh = 1;

    // Draw
    oamDynamic16Draw(0);

    // Finalize
    oamInitDynamicSpriteEndFrame();
    WaitForVBlank();
    oamVramQueueUpdate();
}
```

### 8.3 Multiple Falling Sprites

```c
#define NUM_SPRITES 8

// Initialize all sprites
for (i = 0; i < NUM_SPRITES; i++) {
    oambuffer[i].oamx = 20 + (i * 28);
    oambuffer[i].oamy = -16 - (i * 20);  // Staggered start
    oambuffer[i].oamframeid = i % 8;
    oambuffer[i].oamattribute = OBJ_PRIO(2) | OBJ_PAL(0);
    oambuffer[i].oamrefresh = 1;
    oambuffer[i].oamgraphics = (u16)sprite_tiles;
    oambuffer[i].oamgfxbank = 0;
}

// Main loop
while (1) {
    for (i = 0; i < NUM_SPRITES; i++) {
        // Move down
        oambuffer[i].oamy += 2;
        if (oambuffer[i].oamy > 224) {
            oambuffer[i].oamy = -16;  // Wrap to top
        }

        // Animate every 4 frames
        if ((frame & 3) == 0) {
            oambuffer[i].oamframeid++;
            oambuffer[i].oamrefresh = 1;
        }

        oamDynamic16Draw(i);
    }

    oamInitDynamicSpriteEndFrame();
    WaitForVBlank();
    oamVramQueueUpdate();

    frame++;
}
```

---

## References

- [PVSnesLib Wiki - Sprites](https://github.com/alekmaul/pvsneslib/wiki/Sprites)
- [PVSnesLib Wiki - Dynamic Sprites](https://github.com/alekmaul/pvsneslib/wiki/Dynamic-Sprites)
- [PVSnesLib Wiki - Meta Sprites](https://github.com/alekmaul/pvsneslib/wiki/Meta-Sprites)
- [PVSnesLib Source Code](https://github.com/alekmaul/pvsneslib)
- SNES Development Manual (Nintendo)
