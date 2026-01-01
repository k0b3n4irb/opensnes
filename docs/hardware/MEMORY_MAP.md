# SNES Memory Map

Complete memory layout reference for SNES development.

## Address Space Overview

The 65816 CPU has a 24-bit address bus, allowing access to 16 MB of memory.
The address format is `$BB:AAAA` where BB is the bank (0-255) and AAAA is
the offset within that bank (0-65535).

## LoROM Memory Map (Default)

```
Bank    Address         Size    Description
────────────────────────────────────────────────────────────────
$00-$3F $0000-$1FFF     8 KB    Work RAM mirror (first 8KB)
        $2000-$20FF     256     Unused
        $2100-$21FF     256     PPU Registers (Video)
        $2200-$2FFF     3.5 KB  Unused
        $3000-$3FFF     4 KB    DSP, SuperFX, etc. (expansion)
        $4000-$40FF     256     Old-style joypad registers
        $4200-$44FF     768     CPU Registers (DMA, IRQ, etc.)
        $4500-$5FFF     7 KB    Unused
        $6000-$7FFF     8 KB    Expansion RAM (if present)
        $8000-$FFFF     32 KB   ROM (banks 0-63)

$40-$6F $0000-$7FFF     32 KB   ROM (continued)
        $8000-$FFFF     32 KB   ROM (continued)

$70-$7D $0000-$7FFF     32 KB   SRAM (if present)
        $8000-$FFFF     32 KB   ROM

$7E     $0000-$1FFF     8 KB    Work RAM (direct access)
        $2000-$FFFF     56 KB   Work RAM (general use)

$7F     $0000-$FFFF     64 KB   Work RAM (high bank)

$80-$BF                         Mirror of $00-$3F
$C0-$FF                         Mirror of $40-$7F (ROM only)
```

## Key Memory Regions

### Work RAM ($7E:0000 - $7F:FFFF)

128 KB of general-purpose RAM.

| Range | Size | Common Use |
|-------|------|------------|
| $7E:0000-$7E:00FF | 256 B | Direct Page (fast access) |
| $7E:0100-$7E:01FF | 256 B | Stack |
| $7E:0200-$7E:1FFF | 7.5 KB | Variables, BSS |
| $7E:2000-$7E:FFFF | 56 KB | General use |
| $7F:0000-$7F:FFFF | 64 KB | General use / buffers |

### Video RAM ($2116-$2119)

64 KB accessed through registers, not directly mapped.

| Range | Common Use |
|-------|------------|
| $0000-$3FFF | Background tiles |
| $4000-$7FFF | Sprite tiles |
| $8000-$FFFF | Tilemaps |

Actual layout depends on your configuration.

### OAM (Object Attribute Memory)

544 bytes accessed through registers $2102-$2104.

```c
// Structure per sprite (bytes 0-511, 4 bytes each)
struct OAM_Entry {
    u8 x_low;      // X position (low 8 bits)
    u8 y;          // Y position
    u8 tile;       // Tile number (low 8 bits)
    u8 attr;       // vhoopppc (flip, priority, palette, tile high)
};

// Extension table (bytes 512-543)
// 2 bits per sprite: X high bit, size select
```

### CGRAM (Color RAM)

512 bytes (256 colors x 2 bytes each).

| Range | Use |
|-------|-----|
| 0-255 | Background palettes (16 palettes x 16 colors) |
| 256-511 | Sprite palettes (8 palettes x 16 colors) |

Color format: `0BBBBBGGGGGRRRRR` (15-bit)

## PPU Registers ($2100-$213F)

| Address | Name | Description |
|---------|------|-------------|
| $2100 | INIDISP | Display control |
| $2101 | OBJSEL | Sprite settings |
| $2102-03 | OAMADDL/H | OAM address |
| $2104 | OAMDATA | OAM data write |
| $2105 | BGMODE | BG mode and tile size |
| $2106 | MOSAIC | Mosaic effect |
| $2107-0A | BGnSC | BG tilemap addresses |
| $210B-0C | BGnNBA | BG tile data addresses |
| $210D-14 | BGnHOFS/VOFS | BG scroll positions |
| $2115 | VMAIN | VRAM address increment |
| $2116-17 | VMADDL/H | VRAM address |
| $2118-19 | VMDATAL/H | VRAM data write |
| $211A | M7SEL | Mode 7 settings |
| $211B-20 | M7A-D/X/Y | Mode 7 matrix |
| $2121 | CGADD | CGRAM address |
| $2122 | CGDATA | CGRAM data write |
| $212C-2D | TM/TS | Main/sub screen enable |
| $212E-2F | TMW/TSW | Window masks |
| $2130-32 | CGWSEL/CGADSUB/COLDATA | Color math |
| $2133 | SETINI | Screen settings |

## CPU Registers ($4200-$44FF)

| Address | Name | Description |
|---------|------|-------------|
| $4200 | NMITIMEN | Interrupt enable |
| $4201 | WRIO | I/O port write |
| $4202-03 | WRMPYA/B | Multiplication operands |
| $4204-06 | WRDIVL/H/B | Division operands |
| $4207-0A | HTIMEL/H, VTIMEL/H | IRQ timer |
| $420B | MDMAEN | DMA enable |
| $420C | HDMAEN | HDMA enable |
| $420D | MEMSEL | FastROM enable |
| $4210 | RDNMI | NMI flag |
| $4211 | TIMEUP | IRQ flag |
| $4212 | HVBJOY | H/V blank and joypad status |
| $4214-17 | RDDIVL/H, RDMPYL/H | Division/multiplication result |
| $4218-1F | JOY1L-4H | Joypad data |

## DMA Registers ($43x0-$43xF)

8 DMA channels (x = 0-7):

| Offset | Name | Description |
|--------|------|-------------|
| $43x0 | DMAPx | DMA parameters |
| $43x1 | BBADx | B-bus address |
| $43x2-04 | A1TxL/H/B | A-bus address |
| $43x5-06 | DASxL/H | Transfer size / HDMA count |
| $43x7 | DASBx | HDMA indirect bank |
| $43x8-09 | A2AxL/H | HDMA table address |
| $43xA | NTRLx | HDMA line counter |

## Example: Setting Up Memory

```c
// Define variables in different memory regions
#pragma bss-name(push, "BSS")    // Goes to $7E:0200+
u8 game_variables[256];

#pragma bss-name(push, "HRAM")   // Goes to $7F:0000+
u8 large_buffer[16384];

// Direct page variables (fast access)
#pragma zeropage-name(push, "ZEROPAGE")
u8 temp;
u16 frame_counter;
```

## Further Reading

- [PPU Registers](PPU.md)
- [DMA Details](DMA.md)
- [SNESdev Wiki Memory Map](https://snes.nesdev.org/wiki/Memory_map)
