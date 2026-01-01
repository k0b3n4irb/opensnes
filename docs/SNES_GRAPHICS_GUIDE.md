# SNES Graphics Programming Guide

This guide covers everything needed to create graphics for SNES games using OpenSNES.

## Table of Contents
1. [PPU Architecture](#ppu-architecture)
2. [Video Modes](#video-modes)
3. [Backgrounds](#backgrounds)
4. [Sprites](#sprites)
5. [Metasprites](#metasprites)
6. [VRAM Layout](#vram-layout)
7. [Palettes](#palettes)
8. [Asset Requirements](#asset-requirements)
9. [Tools & Resources](#tools--resources)

---

## PPU Architecture

The SNES uses **two PPU chips** working together:

| Chip | Name | Function |
|------|------|----------|
| PPU1 | 5C77 | "The Brain" - addresses VRAM, makes rendering decisions |
| PPU2 | 5C78 | "The Compositor" - generates video signal, accesses palettes |

### Memory Systems

| Memory | Size | Purpose |
|--------|------|---------|
| **VRAM** | 64 KB | Tiles, tilemaps for backgrounds and sprites |
| **OAM** | 544 bytes | Sprite attributes (128 sprites) |
| **CGRAM** | 512 bytes | Color palettes (256 colors × 15-bit) |

---

## Video Modes

The SNES supports 8 background modes. **Mode 1** is most common for games:

| Mode | BG1 | BG2 | BG3 | BG4 | Notes |
|------|-----|-----|-----|-----|-------|
| 0 | 2bpp | 2bpp | 2bpp | 2bpp | 4 layers, 4 colors each |
| **1** | **4bpp** | **4bpp** | **2bpp** | - | **Most common** - 2×16 colors + 1×4 colors |
| 2 | 4bpp | 4bpp | - | - | Per-tile offset (parallax) |
| 3 | 8bpp | 4bpp | - | - | 256 colors + 16 colors |
| 4 | 8bpp | 2bpp | - | - | Per-tile offset |
| 5 | 4bpp | 2bpp | - | - | Hi-res (512px wide) |
| 6 | 4bpp | - | - | - | Hi-res + per-tile offset |
| 7 | 8bpp | - | - | - | Rotation/scaling (Mode 7) |

### Mode 1 Setup (Recommended)
```c
REG_BGMODE = 0x01;  // Mode 1, 8x8 tiles
// BG1: 16 colors (4bpp) - main playfield
// BG2: 16 colors (4bpp) - parallax/details
// BG3: 4 colors (2bpp)  - HUD/status bar
```

---

## Backgrounds

### Tilemap Structure

Each tilemap entry is **16 bits**:
```
VHOP PPCC CCCC CCCC
│││  ││└─────────── Tile number (0-1023)
│││  └└───────────── Palette (0-7)
││└─────────────────  Priority (0-1)
│└──────────────────  Horizontal flip
└───────────────────  Vertical flip
```

### Tilemap Sizes

| Setting | Tiles | Pixels | Memory |
|---------|-------|--------|--------|
| 32×32 | 1024 | 256×256 | 2 KB |
| 64×32 | 2048 | 512×256 | 4 KB |
| 32×64 | 2048 | 256×512 | 4 KB |
| 64×64 | 4096 | 512×512 | 8 KB |

### Tile Formats

| Bit Depth | Colors | Bytes/Tile | Use Case |
|-----------|--------|------------|----------|
| 2bpp | 4 | 16 bytes | HUD, simple BG |
| 4bpp | 16 | 32 bytes | Main backgrounds |
| 8bpp | 256 | 64 bytes | Detailed art |

---

## Sprites

### OAM Structure (544 bytes total)

**Main Table** (512 bytes, 4 bytes × 128 sprites):

| Byte | Bits | Content |
|------|------|---------|
| 0 | 7-0 | X position (low 8 bits) |
| 1 | 7-0 | Y position |
| 2 | 7-0 | Tile number (low 8 bits) |
| 3 | 7 | Vertical flip |
| 3 | 6 | Horizontal flip |
| 3 | 5-4 | Priority (0-3) |
| 3 | 3-1 | Palette (0-7, from CGRAM $80-$FF) |
| 3 | 0 | Tile number bit 8 |

**Extended Table** (32 bytes, 2 bits × 128 sprites):
- Bit 0: X position bit 8 (for X > 255)
- Bit 1: Size select (0=small, 1=large)

### Sprite Sizes

Register `$2101` selects two sizes (small/large):

| OBJSEL | Small | Large |
|--------|-------|-------|
| 0 | 8×8 | 16×16 |
| 1 | 8×8 | 32×32 |
| 2 | 8×8 | 64×64 |
| 3 | 16×16 | 32×32 |
| 4 | 16×16 | 64×64 |
| 5 | 32×32 | 64×64 |
| 6 | 16×32 | 32×64 |
| 7 | 16×32 | 32×32 |

### Scanline Limits (Critical!)

| Limit | Value | Consequence |
|-------|-------|-------------|
| Sprites/line | 32 max | Higher OAM index sprites dropped |
| Slivers/line | 34 max | Sprite parts clipped |

**Solution**: Rotate OAM priority each frame to distribute flicker.

### Sprite Priority

Within OBJ layer: Lower OAM index = on top
With backgrounds: Priority bits 0-3 interleave with BG layers

---

## Metasprites

Metasprites combine multiple hardware sprites into one logical entity.

### Data Format Example
```c
// Each entry: X offset, Y offset, tile, attributes
const u8 player_idle[] = {
    // Sprite 0: top-left
    0, 0, 0x00, 0x30,
    // Sprite 1: top-right
    8, 0, 0x01, 0x30,
    // Sprite 2: bottom-left
    0, 8, 0x10, 0x30,
    // Sprite 3: bottom-right
    8, 8, 0x11, 0x30,
    0xFF  // End marker
};
```

### Animation State Machine
```c
typedef struct {
    u8 frame;           // Current frame index
    u8 timer;           // Frame duration counter
    u8 loop_start;      // Frame to loop back to
    const u8** frames;  // Array of metasprite pointers
} Animation;

void animation_update(Animation* anim) {
    if (--anim->timer == 0) {
        anim->frame++;
        if (anim->frames[anim->frame] == NULL) {
            anim->frame = anim->loop_start;
        }
        anim->timer = FRAME_DURATION;
    }
}
```

---

## VRAM Layout

### Recommended Layout (Mode 1)

| Address | Size | Content |
|---------|------|---------|
| $0000 | 24 KB | BG tiles (4bpp, 768 tiles) |
| $6000 | 2 KB | BG1 tilemap |
| $6800 | 2 KB | BG2 tilemap |
| $7000 | 2 KB | BG3 tilemap |
| $8000 | 16 KB | Sprite tiles (4bpp, 512 tiles) |

### VRAM Address Registers

| Register | Purpose |
|----------|---------|
| $2107 | BG1 tilemap address (bits 7-2 × $400) |
| $2108 | BG2 tilemap address |
| $2109 | BG3 tilemap address |
| $210B | BG1/2 tile address (nibbles × $1000) |
| $210C | BG3/4 tile address |
| $2101 | Sprite tile address + size select |

---

## Palettes

### CGRAM Layout (256 colors × 2 bytes)

| Range | Colors | Use |
|-------|--------|-----|
| $00-$7F | 128 | Background palettes (8 × 16 colors) |
| $80-$FF | 128 | Sprite palettes (8 × 16 colors) |

### Color Format (15-bit BGR)
```
0BBB BBGG GGGR RRRR
 │││││││││││└────── Red (0-31)
 │││││└└└└└──────── Green (0-31)
 └└└└└───────────── Blue (0-31)
```

### Palette Selection

**Backgrounds**: Tilemap bits 10-12 select palette 0-7
**Sprites**: OAM byte 3 bits 1-3 select palette 0-7 (from upper CGRAM)

---

## Asset Requirements

### For SNES Compatibility

| Asset Type | Requirements |
|------------|--------------|
| Tiles | 8×8 pixels, indexed color |
| BG Palette | Max 16 colors per palette (4bpp) |
| Sprite Palette | Max 16 colors, color 0 = transparent |
| Tilemap | Max 1024 unique tiles per layer |
| Character | 16×16 to 32×32 typical |

### Recommended Workflow

1. **Create art** in any tool (Aseprite, GIMP, Photoshop)
2. **Reduce colors** to 16 per palette
3. **Export as indexed PNG** (8×8 grid alignment)
4. **Convert with gfx4snes** or similar tool
5. **Load via DMA** during VBlank

---

## Tools & Resources

### Graphics Conversion Tools

| Tool | Purpose | Link |
|------|---------|------|
| **gfx4snes** | PNG to SNES tiles/palettes | OpenSNES/tools |
| **M1TE2** | Tile editor, map maker | Third-party |
| **SuperTileMapper** | VRAM/tilemap editor | GitHub |
| **Aseprite** | Pixel art (paid) | aseprite.org |
| **GIMP** | Free image editor | gimp.org |

### Free Asset Sources (CC0/Public Domain)

| Source | Link |
|--------|------|
| **itch.io CC0 Pixel Art** | https://itch.io/game-assets/assets-cc0/tag-pixel-art |
| **itch.io 16-bit Assets** | https://itch.io/game-assets/free/tag-16-bit |
| **OpenGameArt CC0** | https://opengameart.org/content/cc0-resources |
| **Kenney Assets** | https://kenney.nl/assets (CC0) |

### Documentation

| Resource | Description |
|----------|-------------|
| [SNESdev Wiki - Sprites](https://snes.nesdev.org/wiki/Sprites) | Comprehensive sprite reference |
| [Super Famicom Wiki](https://wiki.superfamicom.org) | Hardware documentation |
| [nesdoug tutorials](https://nesdoug.com) | C/ASM tutorials with examples |
| [Fabien Sanglard PPU](https://fabiensanglard.net/snes_ppus_how/) | Deep PPU internals |

---

## Quick Reference

### Register Cheat Sheet

```
$2100 INIDISP  - Screen on/off, brightness
$2101 OBJSEL   - Sprite VRAM address, sizes
$2105 BGMODE   - Background mode, tile size
$2107 BG1SC    - BG1 tilemap address, size
$210B BG12NBA  - BG1/2 tile address
$212C TM       - Main screen layer enable
$212D TS       - Sub screen layer enable
$2115 VMAIN    - VRAM address increment mode
$2116 VMADDL   - VRAM address (low)
$2117 VMADDH   - VRAM address (high)
$2118 VMDATAL  - VRAM data write (low)
$2119 VMDATAH  - VRAM data write (high)
$2121 CGADD    - Palette address
$2122 CGDATA   - Palette data write
```

### DMA Registers (for fast transfers)

```
$4300 DMAP0   - DMA parameters
$4301 BBAD0   - B-bus address ($18 for VRAM, $22 for CGRAM)
$4302 A1T0L   - Source address (low)
$4303 A1T0H   - Source address (high)
$4304 A1B0    - Source bank
$4305 DAS0L   - Transfer size (low)
$4306 DAS0H   - Transfer size (high)
$420B MDMAEN  - Start DMA (bit 0 = channel 0)
```
