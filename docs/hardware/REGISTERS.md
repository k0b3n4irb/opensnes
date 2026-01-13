# SNES Hardware Register Reference

Complete reference for all SNES hardware registers commonly used in game development.

---

## Table of Contents

- [PPU Registers ($2100-$213F)](#ppu-registers-2100-213f)
- [CPU Registers ($4200-$421F)](#cpu-registers-4200-421f)
- [DMA Registers ($4300-$437F)](#dma-registers-4300-437f)
- [Joypad Registers ($4016-$4017, $4218-$421F)](#joypad-registers)

---

## PPU Registers ($2100-$213F)

### Display Control

#### $2100 - INIDISP (Display Control)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | Force Blank | 1 = Screen off (black), VRAM/OAM/CGRAM accessible |
| 6-4 | - | Unused |
| 3-0 | Brightness | 0 = Black, 15 = Full brightness |

```c
// Turn screen on with full brightness
REG_INIDISP = 0x0F;

// Force blank (screen off)
REG_INIDISP = 0x80;
```

#### $2101 - OBJSEL (Sprite Settings)

| Bit | Name | Description |
|-----|------|-------------|
| 7-5 | Size | Sprite size combination (see table) |
| 4-3 | Name Select | Sprite tile address offset |
| 2-0 | Base | Sprite tile base address (/8192) |

**Sprite Sizes:**

| Value | Small | Large |
|-------|-------|-------|
| 0 | 8x8 | 16x16 |
| 1 | 8x8 | 32x32 |
| 2 | 8x8 | 64x64 |
| 3 | 16x16 | 32x32 |
| 4 | 16x16 | 64x64 |
| 5 | 32x32 | 64x64 |
| 6 | 16x32 | 32x64 |
| 7 | 16x32 | 32x32 |

```c
// 8x8 small, 16x16 large sprites, tiles at $0000
REG_OBJSEL = 0x00;

// 16x16 small, 32x32 large sprites
REG_OBJSEL = 0x60;
```

### OAM (Sprite Memory)

#### $2102-$2103 - OAMADDL/H (OAM Address)

| Register | Bits | Description |
|----------|------|-------------|
| $2102 | 7-0 | OAM address low byte |
| $2103 | 7 | OAM priority rotation |
| $2103 | 0 | OAM address bit 9 |

#### $2104 - OAMDATA (OAM Data Write)

Write sprite data to OAM. Address auto-increments.

```c
// Set sprite 0 position
REG_OAMADDL = 0;
REG_OAMADDH = 0;
REG_OAMDATA = 100;  // X position
REG_OAMDATA = 80;   // Y position
REG_OAMDATA = 0;    // Tile number
REG_OAMDATA = 0;    // Attributes
```

### Background Mode

#### $2105 - BGMODE (Background Mode)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | BG4 Tile Size | 0 = 8x8, 1 = 16x16 |
| 6 | BG3 Tile Size | 0 = 8x8, 1 = 16x16 |
| 5 | BG2 Tile Size | 0 = 8x8, 1 = 16x16 |
| 4 | BG1 Tile Size | 0 = 8x8, 1 = 16x16 |
| 3 | Mode 1 BG3 Priority | 0 = Normal, 1 = High |
| 2-0 | BG Mode | 0-7 (see table below) |

**Background Modes:**

| Mode | BG1 | BG2 | BG3 | BG4 | Notes |
|------|-----|-----|-----|-----|-------|
| 0 | 4-color | 4-color | 4-color | 4-color | 4 layers |
| 1 | 16-color | 16-color | 4-color | - | Most common |
| 2 | 16-color | 16-color | - | - | Offset-per-tile |
| 3 | 256-color | 16-color | - | - | Direct color |
| 4 | 256-color | 4-color | - | - | Offset-per-tile |
| 5 | 16-color | 4-color | - | - | Hi-res 512px |
| 6 | 16-color | - | - | - | Hi-res + offset |
| 7 | 256-color | - | - | - | Rotation/scaling |

```c
// Mode 1 with 8x8 tiles
REG_BGMODE = 0x01;

// Mode 1 with 16x16 BG1 tiles
REG_BGMODE = 0x11;
```

### Background Tilemap

#### $2107-$210A - BG1SC-BG4SC (Tilemap Address/Size)

| Bit | Name | Description |
|-----|------|-------------|
| 7-2 | Address | Tilemap VRAM address (/1024) |
| 1-0 | Size | 00=32x32, 01=64x32, 10=32x64, 11=64x64 |

```c
// BG1 tilemap at $7000 (32x32 tiles)
REG_BG1SC = 0x70;

// BG1 tilemap at $7000 (64x64 tiles)
REG_BG1SC = 0x73;
```

#### $210B-$210C - BG12NBA/BG34NBA (Tile Data Address)

| Bit | Name | Description |
|-----|------|-------------|
| 7-4 | BG2/BG4 | Tile data address (/4096) |
| 3-0 | BG1/BG3 | Tile data address (/4096) |

```c
// BG1 tiles at $0000, BG2 at $4000
REG_BG12NBA = 0x40;
```

### Background Scroll

#### $210D-$2114 - BGnHOFS/BGnVOFS (Scroll Registers)

Write twice: first low byte, then high byte.

```c
// Scroll BG1 horizontally by 100 pixels
REG_BG1HOFS = 100 & 0xFF;
REG_BG1HOFS = 100 >> 8;

// Scroll BG1 vertically by 50 pixels
REG_BG1VOFS = 50 & 0xFF;
REG_BG1VOFS = 50 >> 8;
```

### VRAM Access

#### $2115 - VMAIN (VRAM Address Increment)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | Increment | 0 = After $2118, 1 = After $2119 |
| 3-2 | Translation | Address translation mode |
| 1-0 | Step | 00 = +1, 01 = +32, 10 = +128, 11 = +128 |

```c
// Increment after high byte write, step by 1
REG_VMAIN = 0x80;
```

#### $2116-$2117 - VMADDL/H (VRAM Address)

Word address (not byte address).

```c
// Set VRAM address to $4000 (word address $2000)
REG_VMADDL = 0x00;
REG_VMADDH = 0x20;
```

#### $2118-$2119 - VMDATAL/H (VRAM Data Write)

```c
// Write word $1234 to VRAM
REG_VMDATAL = 0x34;  // Low byte
REG_VMDATAH = 0x12;  // High byte (triggers increment)
```

### Color (CGRAM)

#### $2121 - CGADD (CGRAM Address)

Color index (0-255).

```c
// Set palette address to color 0
REG_CGADD = 0;

// Set palette address to sprite palette 0 (color 128)
REG_CGADD = 128;
```

#### $2122 - CGDATA (CGRAM Data Write)

15-bit color: `0BBBBBGGGGGRRRRR`

```c
// Set color to white ($7FFF)
REG_CGDATA = 0xFF;  // Low byte
REG_CGDATA = 0x7F;  // High byte

// Set color to red ($001F)
REG_CGDATA = 0x1F;
REG_CGDATA = 0x00;

// Set color to green ($03E0)
REG_CGDATA = 0xE0;
REG_CGDATA = 0x03;

// Set color to blue ($7C00)
REG_CGDATA = 0x00;
REG_CGDATA = 0x7C;
```

### Screen Enable

#### $212C - TM (Main Screen Enable)

| Bit | Name | Description |
|-----|------|-------------|
| 4 | OBJ | Enable sprites on main screen |
| 3 | BG4 | Enable BG4 on main screen |
| 2 | BG3 | Enable BG3 on main screen |
| 1 | BG2 | Enable BG2 on main screen |
| 0 | BG1 | Enable BG1 on main screen |

```c
// Enable BG1 and sprites
REG_TM = 0x11;

// Enable BG1, BG2, and sprites
REG_TM = 0x13;
```

#### $212D - TS (Sub Screen Enable)

Same bit layout as TM, for sub screen (color math).

---

## CPU Registers ($4200-$421F)

### Interrupt Control

#### $4200 - NMITIMEN (Interrupt Enable)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | NMI Enable | Enable VBlank NMI |
| 5 | V-IRQ | Enable V-counter IRQ |
| 4 | H-IRQ | Enable H-counter IRQ |
| 0 | Joypad | Enable auto-joypad read |

```c
// Enable NMI and auto-joypad
REG_NMITIMEN = 0x81;

// Enable NMI, V-IRQ, and auto-joypad
REG_NMITIMEN = 0xA1;
```

#### $4207-$420A - HTIMEx/VTIMEx (IRQ Timer)

H position (0-339) and V position (0-261/311) for IRQ trigger.

```c
// Trigger IRQ at scanline 128
REG_VTIMEL = 128;
REG_VTIMEH = 0;
REG_HTIMEL = 0;
REG_HTIMEH = 0;
```

#### $4210 - RDNMI (NMI Status)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | NMI | 1 = NMI occurred (cleared on read) |
| 3-0 | Version | CPU version |

**IMPORTANT:** Read this register in NMI handler to acknowledge NMI!

```c
// In NMI handler:
volatile u8 dummy = REG_RDNMI;  // Acknowledge NMI
```

#### $4211 - TIMEUP (IRQ Status)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | IRQ | 1 = IRQ occurred (cleared on read) |

**IMPORTANT:** Read this register in IRQ handler to acknowledge IRQ!

#### $4212 - HVBJOY (Blank/Joypad Status)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | VBlank | 1 = In VBlank |
| 6 | HBlank | 1 = In HBlank |
| 0 | Joypad | 1 = Auto-joypad read in progress |

```c
// Wait for auto-joypad to complete
while (REG_HVBJOY & 0x01) {}
```

### Math Hardware

#### $4202-$4203 - WRMPYA/B (Multiply)

8-bit x 8-bit = 16-bit multiplication.

```c
REG_WRMPYA = 25;
REG_WRMPYB = 4;
// Wait 8 cycles
u16 result = REG_RDMPYL | (REG_RDMPYH << 8);  // = 100
```

#### $4204-$4206 - WRDIV (Divide)

16-bit / 8-bit division with remainder.

```c
REG_WRDIVL = 100 & 0xFF;
REG_WRDIVH = 100 >> 8;
REG_WRDIVB = 7;
// Wait 16 cycles
u16 quotient = REG_RDDIVL | (REG_RDDIVH << 8);   // = 14
u16 remainder = REG_RDMPYL | (REG_RDMPYH << 8);  // = 2
```

### DMA Control

#### $420B - MDMAEN (General DMA Enable)

Write a bit mask to start DMA on channels 0-7.

```c
// Start DMA on channel 0
REG_MDMAEN = 0x01;

// Start DMA on channels 0 and 1
REG_MDMAEN = 0x03;
```

#### $420C - HDMAEN (HDMA Enable)

Enable HDMA on channels 0-7. HDMA runs automatically each scanline.

---

## DMA Registers ($4300-$437F)

Each channel has 16 bytes of registers at $43x0-$43xF where x = channel (0-7).

### Channel Configuration

#### $43x0 - DMAPx (DMA Parameters)

| Bit | Name | Description |
|-----|------|-------------|
| 7 | Direction | 0 = A→B (CPU→PPU), 1 = B→A |
| 6 | HDMA Mode | 0 = Absolute, 1 = Indirect |
| 5 | - | Unused |
| 4 | Auto Inc | 0 = Auto, 1 = Fixed source |
| 3 | Fixed/Dec | 0 = Increment, 1 = Decrement |
| 2-0 | Mode | Transfer mode (see table) |

**Transfer Modes:**

| Mode | Pattern | Description |
|------|---------|-------------|
| 0 | 1 byte | Write to single register |
| 1 | 2 bytes | Write to reg, reg+1 alternating |
| 2 | 2 bytes | Write to reg twice |
| 3 | 4 bytes | Write to reg, reg+1 x2 each |
| 4 | 4 bytes | Write to reg, reg+1, reg+2, reg+3 |

```c
// Mode 1 (for VRAM): write to $2118/$2119 alternating
REG_DMAP(0) = 0x01;

// Mode 0 (for CGRAM): write to $2122
REG_DMAP(0) = 0x00;
```

#### $43x1 - BBADx (B-Bus Address)

PPU register address (low byte only, $21xx).

```c
// DMA to VRAM data port ($2118)
REG_BBAD(0) = 0x18;

// DMA to CGRAM data port ($2122)
REG_BBAD(0) = 0x22;

// DMA to OAM data port ($2104)
REG_BBAD(0) = 0x04;
```

#### $43x2-$43x4 - A1TxL/H/B (Source Address)

24-bit source address (CPU side).

```c
// Source at $7E:2000
REG_A1TL(0) = 0x00;
REG_A1TH(0) = 0x20;
REG_A1B(0) = 0x7E;
```

#### $43x5-$43x6 - DASxL/H (Transfer Size)

Number of bytes to transfer (0 = 65536).

```c
// Transfer 1024 bytes
REG_DASL(0) = 0x00;
REG_DASH(0) = 0x04;
```

### DMA Example

```c
// DMA 2048 bytes to VRAM at $0000
void dmaToVRAM(const void* src, u16 vramAddr, u16 size) {
    // Set VRAM address
    REG_VMAIN = 0x80;
    REG_VMADDL = vramAddr & 0xFF;
    REG_VMADDH = vramAddr >> 8;

    // Configure DMA channel 0
    REG_DMAP(0) = 0x01;           // Mode 1 (2-register)
    REG_BBAD(0) = 0x18;           // Destination: $2118 (VMDATAL)
    REG_A1TL(0) = (u16)src & 0xFF;
    REG_A1TH(0) = (u16)src >> 8;
    REG_A1B(0) = 0x7E;            // Source bank
    REG_DASL(0) = size & 0xFF;
    REG_DASH(0) = size >> 8;

    // Start DMA
    REG_MDMAEN = 0x01;
}
```

---

## Joypad Registers

### $4218-$421F - JOY1-4 (Auto-Read Results)

After auto-joypad read completes (check $4212 bit 0):

| Register | Description |
|----------|-------------|
| $4218 | Joypad 1 low byte |
| $4219 | Joypad 1 high byte |
| $421A | Joypad 2 low byte |
| $421B | Joypad 2 high byte |
| $421C-$421F | Joypads 3-4 (multitap) |

**Button Bit Layout:**

| Byte | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|------|-------|-------|-------|-------|-------|-------|-------|-------|
| Low ($4218) | B | Y | Select | Start | Up | Down | Left | Right |
| High ($4219) | A | X | L | R | - | - | - | - |

```c
// Read joypad 1
while (REG_HVBJOY & 0x01) {}  // Wait for auto-read
u16 joy = REG_JOY1L | (REG_JOY1H << 8);

// Check buttons
if (joy & 0x0080) { /* A pressed */ }
if (joy & 0x8000) { /* B pressed */ }
if (joy & 0x0800) { /* Up pressed */ }
```

---

## OpenSNES Register Macros

The SDK provides convenient macros in `<snes/registers.h>`:

```c
// Direct register access
REG_INIDISP = 0x0F;
REG_BGMODE = 0x01;
REG_TM = 0x11;

// DMA channel macros
REG_DMAP(0) = 0x01;   // Channel 0
REG_BBAD(1) = 0x22;   // Channel 1
REG_A1TL(2) = 0x00;   // Channel 2

// PPU status (read-only)
if (REG_HVBJOY & 0x80) { /* In VBlank */ }
```

---

## Further Reading

- [Memory Map](MEMORY_MAP.md) - Full address space layout
- [PPU Details](PPU.md) - Graphics subsystem
- [DMA Details](DMA.md) - DMA transfer modes
- [Fullsnes](https://problemkaputt.de/fullsnes.htm) - Complete technical reference
