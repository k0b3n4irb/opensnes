# SNES Hardware Overview

Understanding SNES hardware is essential for effective game development.

## Contents

| Document | Description |
|----------|-------------|
| [CPU](CPU.md) | 65816 processor details |
| [PPU](PPU.md) | Picture Processing Unit (graphics) |
| [APU](APU.md) | Audio Processing Unit (sound) |
| [Memory Map](MEMORY_MAP.md) | RAM, ROM, and register layout |
| [DMA](DMA.md) | Direct Memory Access |

## Quick Reference

### Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 65816 @ 3.58 MHz (NTSC) / 3.55 MHz (PAL) |
| Work RAM | 128 KB |
| VRAM | 64 KB |
| OAM | 544 bytes (128 sprites + size/position bits) |
| CGRAM | 512 bytes (256 colors x 2 bytes) |
| APU | SPC700 @ 1.024 MHz + DSP |
| Audio RAM | 64 KB |

### Resolution

| Mode | Resolution |
|------|------------|
| NTSC | 256x224 or 256x239 |
| PAL | 256x239 or 256x268 |
| Hi-res | 512x224 (limited use) |

### Colors

- 15-bit color (32768 colors)
- 256 simultaneous colors (8 palettes x 16 colors for BG, same for sprites)
- Format: `0BBBBBGGGGGRRRRR`

## Key Concepts

### Vertical Blank (VBlank)

The PPU draws the screen line by line. VBlank is the period when the PPU
is not drawing (returning to top of screen). This is the **only safe time**
to update VRAM, OAM, and CGRAM.

```c
// Wait for VBlank before updating graphics
WaitForVBlank();
// Now safe to DMA to VRAM
```

### Tiles and Sprites

**Tiles** are 8x8 pixel graphics used for both backgrounds and sprites.

**Sprites** (Objects/OBJ) are movable graphics composed of one or more tiles:
- 128 sprites maximum
- Sizes: 8x8, 16x16, 32x32, 64x64
- 32 sprites per scanline limit
- Can be flipped horizontally/vertically

### Background Modes

| Mode | BG1 | BG2 | BG3 | BG4 | Colors |
|------|-----|-----|-----|-----|--------|
| 0 | 4 | 4 | 4 | 4 | 4 per BG |
| 1 | 16 | 16 | 4 | - | Common |
| 2 | 16 | 16 | - | - | Per-tile offset |
| 3 | 256 | 16 | - | - | Direct color |
| 4 | 256 | 4 | - | - | Per-tile offset |
| 5 | 16 | 4 | - | - | Hi-res |
| 6 | 16 | - | - | - | Hi-res + offset |
| 7 | 256 | - | - | - | Mode 7 rotation |

Most games use **Mode 1** (good balance of features).

### Banks and Addressing

The 65816 has a 24-bit address space divided into 256 "banks" of 64KB each:

```
$00:0000 - $00:FFFF  Bank 0 (direct page, stack, I/O)
$7E:0000 - $7E:FFFF  Work RAM (low 64KB)
$7F:0000 - $7F:FFFF  Work RAM (high 64KB)
$80:0000 - $FF:FFFF  ROM and mirrors
```

### LoROM vs HiROM

ROM mapping modes affect how cartridge ROM appears in the address space:

| Mode | ROM Start | Max Size | Common Use |
|------|-----------|----------|------------|
| LoROM | $80:8000 | 4 MB | Most games |
| HiROM | $C0:0000 | 4 MB | Large games |
| ExHiROM | Varies | 8 MB | Very large games |

OpenSNES uses **LoROM** by default.

## Further Reading

- [SNESdev Wiki](https://snes.nesdev.org/) - Comprehensive hardware docs
- [Fullsnes](https://problemkaputt.de/fullsnes.htm) - Detailed technical reference
- [Super Famicom Wiki](https://wiki.superfamicom.org/) - Tutorials and guides
