# Super Nintendo Entertainment System - Complete Technical Reference

> Comprehensive hardware documentation compiled from Nintendo Development Manuals (Book I & II) and nocash fullsnes specifications

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Memory Architecture](#2-memory-architecture)
3. [CPU 65C816](#3-cpu-65c816)
4. [Picture Processing Unit (PPU)](#4-picture-processing-unit-ppu)
5. [Audio Processing Unit (APU)](#5-audio-processing-unit-apu)
6. [DMA and HDMA](#6-dma-and-hdma)
7. [Controllers and Peripherals](#7-controllers-and-peripherals)
8. [Enhancement Chips](#8-enhancement-chips)
9. [Cartridge Format](#9-cartridge-format)
10. [Timing Specifications](#10-timing-specifications)
11. [Programming Reference](#11-programming-reference)

---

## 1. System Overview

### 1.1 System Specifications

| Component | Specification |
|-----------|---------------|
| **Main CPU** | Ricoh 5A22 (65C816-based) |
| **CPU Clock** | 1.79 / 2.68 / 3.58 MHz (variable) |
| **Graphics** | S-PPU1 + S-PPU2 (dual PPU chips) |
| **Sound CPU** | Sony SPC700 @ 1.024 MHz |
| **Sound DSP** | S-DSP (8 voices, 32 kHz) |
| **Work RAM** | 128 KB |
| **Video RAM** | 64 KB |
| **Audio RAM** | 64 KB |
| **Color Palette** | 32,768 colors (15-bit RGB) |
| **On-Screen Colors** | Up to 256 |
| **Sprites** | 128 objects, up to 64×64 pixels |
| **Resolution** | 256-512 × 224-478 pixels |

### 1.2 Display Specifications

| Parameter | NTSC | PAL |
|-----------|------|-----|
| Frame Rate | 60.099 Hz | 50.007 Hz |
| Active Lines | 224 or 239 | 224 or 239 |
| Total Lines | 262 | 312 |
| Scanline Length | 341 dots (1364 cycles) | 341 dots (1364 cycles) |
| Master Clock | 21.477270 MHz | 21.281370 MHz |

### 1.3 Clock Speed Summary

| Region | Speed | Master Cycles | Usage |
|--------|-------|---------------|-------|
| Fast | 3.58 MHz | 6 | Fast ROM, I/O ports, internal |
| Medium | 2.68 MHz | 8 | WRAM, Slow ROM, SRAM |
| Slow | 1.79 MHz | 12 | Joypad ports (4000h-41FFh) |

---

## 2. Memory Architecture

### 2.1 24-Bit Address Space Overview

The SNES uses a 24-bit address bus (000000h-FFFFFFh), divided into 8-bit bank numbers (00h-FFh) and 16-bit offsets (0000h-FFFFh).

```
Bank     Offset          Content                              Speed
─────────────────────────────────────────────────────────────────────────
00-3F    0000h-1FFFh    WRAM Mirror (first 8KB)              2.68 MHz
         2000h-20FFh    Unused                                3.58 MHz
         2100h-21FFh    PPU Registers (B-Bus)                3.58 MHz
         2200h-3FFFh    Unused / Expansion                   3.58 MHz
         4000h-41FFh    Joypad I/O                           1.78 MHz
         4200h-5FFFh    CPU I/O Registers                    3.58 MHz
         6000h-7FFFh    Expansion (SRAM in HiROM)            2.68 MHz
         8000h-FFFFh    ROM (LoROM: 32KB banks)              2.68 MHz

40-7D    0000h-FFFFh    ROM (HiROM: 64KB banks)              2.68 MHz

7E-7F    0000h-FFFFh    Work RAM (128KB total)               2.68 MHz
         7E:0000-1FFF   Low WRAM (8KB, mirrored)
         7E:2000-FFFF   High WRAM (56KB)
         7F:0000-FFFF   High WRAM (64KB)

80-BF    0000h-7FFFh    System Area (mirrors 00-3F)          Variable
         8000h-FFFFh    ROM (WS2 LoROM, fast optional)       2.68/3.58 MHz

C0-FF    0000h-FFFFh    ROM (WS2 HiROM, fast optional)       2.68/3.58 MHz
```

### 2.2 Memory Not Mapped to CPU Bus

These memories are accessible only via I/O ports:

| Memory | Size | Purpose | Access Port |
|--------|------|---------|-------------|
| OAM | 544 bytes | Sprite attributes | 2102h-2104h, 2138h |
| VRAM | 64 KB | Tiles, BG maps | 2115h-2119h, 2139h-213Ah |
| CGRAM | 512 bytes | Color palette | 2121h-2122h, 213Bh |
| APU RAM | 64 KB | Sound data/code | 2140h-2143h (indirect) |
| APU ROM | 64 bytes | IPL Boot ROM | (internal to APU) |

### 2.3 ROM Mapping Modes

#### LoROM (Mode 20) - 32KB Banks
```
Banks 00-3F: ROM at 8000h-FFFFh (lower 32KB of each 64KB ROM bank)
Banks 80-BF: ROM mirror (optionally fast via 420Dh)
Banks 40-6F: Extended ROM area
Banks 70-7D: SRAM at 0000h-7FFFh (32KB banks)
```

#### HiROM (Mode 21) - 64KB Banks
```
Banks 40-7D: Full 64KB ROM banks
Banks C0-FF: ROM mirror (optionally fast)
Banks 00-3F: ROM at 8000h-FFFFh (upper halves)
Banks 30-3F: SRAM at 6000h-7FFFh (8KB banks)
```

#### ExHiROM (Mode 25)
Used by only 2 games (Tales of Phantasia, Dai Kaiju Monogatari 2):
```
Banks C0-FF: Fast HiROM (4MB)
Banks 40-7D: Slow HiROM extension
```

### 2.4 WRAM Access

**Direct Access:**
- Full 128KB at 7E0000h-7FFFFFh
- First 8KB mirrored to xx0000h-xx1FFFh (xx=00-3F, 80-BF)

**I/O Port Access (for DMA):**

| Address | Name | Description |
|---------|------|-------------|
| 2180h | WMDATA | WRAM Data Read/Write (auto-increment) |
| 2181h | WMADDL | WRAM Address Low |
| 2182h | WMADDM | WRAM Address Mid |
| 2183h | WMADDH | WRAM Address High (bit 0 only) |

---

## 3. CPU 65C816

### 3.1 Register Set

```
┌─────────────────────────────────────────────────────────────────────┐
│  Accumulator (A/C)         8 or 16 bits (controlled by M flag)     │
├─────────────────────────────────────────────────────────────────────┤
│  Index Register X          8 or 16 bits (controlled by X flag)     │
├─────────────────────────────────────────────────────────────────────┤
│  Index Register Y          8 or 16 bits (controlled by X flag)     │
├─────────────────────────────────────────────────────────────────────┤
│  Stack Pointer (S)         16 bits (8-bit in emulation mode)       │
├─────────────────────────────────────────────────────────────────────┤
│  Direct Page (D)           16 bits (base for zero-page addressing) │
├─────────────────────────────────────────────────────────────────────┤
│  Program Counter (PC)      16 bits                                  │
├─────────────────────────────────────────────────────────────────────┤
│  Program Bank (PB)         8 bits (extends PC to 24-bit)           │
├─────────────────────────────────────────────────────────────────────┤
│  Data Bank (DB)            8 bits (default bank for data access)   │
├─────────────────────────────────────────────────────────────────────┤
│  Processor Status (P)      8 bits: N V M X D I Z C                 │
│                            + hidden Emulation flag (E)              │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 Processor Status Flags

| Bit | Native Mode | Emulation Mode | Description |
|-----|-------------|----------------|-------------|
| 7 | N | N | Negative (sign bit) |
| 6 | V | V | Overflow |
| 5 | M | 1 | Memory/Accumulator size (0=16-bit, 1=8-bit) |
| 4 | X | B | Index size (0=16-bit) / Break flag |
| 3 | D | D | Decimal mode (BCD for ADC/SBC) |
| 2 | I | I | IRQ disable |
| 1 | Z | Z | Zero |
| 0 | C | C | Carry |
| - | E | E | Emulation mode (hidden, accessed via XCE) |

### 3.3 Addressing Modes

| Mode | Syntax | Example | Effective Address |
|------|--------|---------|-------------------|
| Immediate | #nn | LDA #$12 | Constant value |
| Direct Page | nn | LDA $12 | [D + nn] |
| Direct Page,X | nn,X | LDA $12,X | [D + nn + X] |
| Direct Page,Y | nn,Y | LDX $12,Y | [D + nn + Y] |
| Absolute | nnnn | LDA $1234 | [DB:nnnn] |
| Absolute,X | nnnn,X | LDA $1234,X | [DB:nnnn + X] |
| Absolute,Y | nnnn,Y | LDA $1234,Y | [DB:nnnn + Y] |
| Absolute Long | nnnnnn | LDA $123456 | [nnnnnn] |
| Absolute Long,X | nnnnnn,X | LDA $123456,X | [nnnnnn + X] |
| Indirect | (nn) | LDA ($12) | [[D + nn]] |
| Indirect,X | (nn,X) | LDA ($12,X) | [[D + nn + X]] |
| Indirect,Y | (nn),Y | LDA ($12),Y | [[D + nn] + Y] |
| Indirect Long | [nn] | LDA [$12] | [FAR[D + nn]] |
| Indirect Long,Y | [nn],Y | LDA [$12],Y | [FAR[D + nn] + Y] |
| Stack Relative | nn,S | LDA $12,S | [S + nn] |
| Stack Indirect,Y | (nn,S),Y | LDA ($12,S),Y | [[S + nn] + Y] |

### 3.4 Instruction Summary

#### Data Transfer
```
LDA/LDX/LDY   Load register from memory
STA/STX/STY   Store register to memory
STZ           Store zero to memory
TAX/TAY/TXA   Transfer between A, X, Y
TXY/TYX       Transfer between X and Y
TCD/TDC       Transfer between A (16-bit) and D
TCS/TSC       Transfer between A (16-bit) and S
TSX/TXS       Transfer between X and S
MVN/MVP       Block move (increment/decrement)
PHA/PHX/PHY   Push register
PLA/PLX/PLY   Pull register
PHB/PLB       Push/Pull Data Bank
PHD/PLD       Push/Pull Direct Page
PHK           Push Program Bank
PHP/PLP       Push/Pull Processor Status
PEA/PEI/PER   Push effective address
```

#### Arithmetic and Logic
```
ADC           Add with carry
SBC           Subtract with carry
CMP/CPX/CPY   Compare
AND/ORA/EOR   Logical operations
BIT           Bit test
INC/DEC       Increment/Decrement memory
INX/INY       Increment index
DEX/DEY       Decrement index
ASL/LSR       Shift left/right
ROL/ROR       Rotate through carry
TRB/TSB       Test and Reset/Set bits
```

#### Branch and Jump
```
BCC/BCS       Branch on carry clear/set
BEQ/BNE       Branch on equal/not equal
BMI/BPL       Branch on minus/plus
BVC/BVS       Branch on overflow clear/set
BRA           Branch always (8-bit)
BRL           Branch always long (16-bit)
JMP           Jump (absolute, indirect)
JML           Jump long (24-bit)
JSR           Jump to subroutine
JSL           Jump to subroutine long
RTS/RTL       Return from subroutine/long
RTI           Return from interrupt
```

#### Status Register
```
CLC/SEC       Clear/Set carry
CLD/SED       Clear/Set decimal
CLI/SEI       Clear/Set interrupt disable
CLV           Clear overflow
REP           Reset processor bits
SEP           Set processor bits
XCE           Exchange carry and emulation
```

### 3.5 CPU Control Registers

| Address | Name | R/W | Description |
|---------|------|-----|-------------|
| 4200h | NMITIMEN | W | NMI, Timer, Joypad Enable |
| 4201h | WRIO | W | Programmable I/O Output |
| 4202h | WRMPYA | W | Multiplicand A (8-bit) |
| 4203h | WRMPYB | W | Multiplier B (triggers multiply) |
| 4204h | WRDIVL | W | Dividend Low |
| 4205h | WRDIVH | W | Dividend High |
| 4206h | WRDIVB | W | Divisor (triggers divide) |
| 4207h | HTIMEL | W | H-Count Timer Low |
| 4208h | HTIMEH | W | H-Count Timer High (bit 0) |
| 4209h | VTIMEL | W | V-Count Timer Low |
| 420Ah | VTIMEH | W | V-Count Timer High (bit 0) |
| 420Bh | MDMAEN | W | General DMA Enable |
| 420Ch | HDMAEN | W | H-DMA Enable |
| 420Dh | MEMSEL | W | ROM Access Speed (bit 0) |
| 4210h | RDNMI | R | NMI Flag / CPU Version |
| 4211h | TIMEUP | R | H/V Timer IRQ Flag |
| 4212h | HVBJOY | R | H/V Blank and Joypad Status |
| 4213h | RDIO | R | Programmable I/O Input |
| 4214h | RDDIVL | R | Division Quotient Low |
| 4215h | RDDIVH | R | Division Quotient High |
| 4216h | RDMPYL | R | Multiply Product / Divide Remainder Low |
| 4217h | RDMPYH | R | Multiply Product / Divide Remainder High |
| 4218h-421Fh | JOYnL/H | R | Joypad Data (auto-read) |

### 3.6 Hardware Math

**Multiplication (8×8=16):**
- Write multiplicand to 4202h, multiplier to 4203h
- Result available in 4216h-4217h after 8 CPU cycles

**Division (16÷8=16 quotient, 16 remainder):**
- Write dividend to 4204h-4205h, divisor to 4206h
- Quotient in 4214h-4215h, remainder in 4216h-4217h after 16 cycles

### 3.7 Exception Vectors

| Address | Native Mode | Emulation Mode |
|---------|-------------|----------------|
| 00FFE4h | COP | - |
| 00FFE6h | BRK | - |
| 00FFE8h | ABORT | - |
| 00FFEAh | NMI | - |
| 00FFEEh | IRQ | - |
| 00FFF4h | - | COP |
| 00FFF8h | - | ABORT |
| 00FFFAh | - | NMI |
| 00FFFCh | RESET | RESET |
| 00FFFEh | - | IRQ/BRK |

---

## 4. Picture Processing Unit (PPU)

### 4.1 PPU Overview

The SNES uses two PPU chips (S-PPU1 and S-PPU2) working together:
- **S-PPU1**: Handles OAM, generates pixel data
- **S-PPU2**: Handles palette, windows, color math, outputs video

### 4.2 PPU Register Map

#### Display Control

| Address | Name | R/W | Description |
|---------|------|-----|-------------|
| 2100h | INIDISP | W | Master brightness, Forced blank |
| 212Ch | TM | W | Main screen layer enable |
| 212Dh | TS | W | Sub screen layer enable |
| 2133h | SETINI | W | Interlace, Pseudo-hires, etc. |

**INIDISP (2100h):**
```
Bit 7:   Forced Blank (1=display off, VRAM accessible)
Bit 3-0: Master Brightness (0=black, 1-15=brightness)
```

**TM/TS (212Ch/212Dh):**
```
Bit 4: OBJ enable
Bit 3: BG4 enable
Bit 2: BG3 enable
Bit 1: BG2 enable
Bit 0: BG1 enable
```

#### Background Control

| Address | Name | Description |
|---------|------|-------------|
| 2105h | BGMODE | BG mode and character size |
| 2106h | MOSAIC | Mosaic size and enable |
| 2107h-210Ah | BGnSC | BG screen base and size |
| 210Bh-210Ch | BG12NBA/BG34NBA | BG character base |
| 210Dh-2114h | BGnHOFS/VOFS | BG scroll registers |

**BGMODE (2105h):**
```
Bit 7:   BG4 tile size (0=8×8, 1=16×16)
Bit 6:   BG3 tile size
Bit 5:   BG2 tile size
Bit 4:   BG1 tile size
Bit 3:   BG3 priority (Mode 1 only)
Bit 2-0: BG Mode (0-7)
```

#### VRAM Access

| Address | Name | Description |
|---------|------|-------------|
| 2115h | VMAIN | VRAM address increment mode |
| 2116h-2117h | VMADDL/H | VRAM address (word address) |
| 2118h-2119h | VMDATAL/H | VRAM data write |
| 2139h-213Ah | RDVRAML/H | VRAM data read |

**VMAIN (2115h):**
```
Bit 7:   Increment after High/Low (0=low, 1=high)
Bit 3-2: Address translation (0-3 for bitmap modes)
Bit 1-0: Increment step (0=1, 1=32, 2=128, 3=128)
```

#### Mode 7 Registers

| Address | Name | Description |
|---------|------|-------------|
| 211Ah | M7SEL | Mode 7 settings |
| 211Bh-211Eh | M7A-M7D | Rotation/scaling matrix |
| 211Fh-2120h | M7X/M7Y | Center coordinate |
| 210Dh-210Eh | M7HOFS/VOFS | Mode 7 scroll (shared with BG1) |

#### Palette (CGRAM)

| Address | Name | Description |
|---------|------|-------------|
| 2121h | CGADD | CGRAM address (word) |
| 2122h | CGDATA | CGRAM data write (2 writes per color) |
| 213Bh | RDCGRAM | CGRAM data read |

**Color Format (15-bit):**
```
Bit 14-10: Blue (0-31)
Bit 9-5:   Green (0-31)
Bit 4-0:   Red (0-31)
```

#### OAM (Sprites)

| Address | Name | Description |
|---------|------|-------------|
| 2101h | OBSEL | OBJ size and base |
| 2102h-2103h | OAMADDL/H | OAM address and priority |
| 2104h | OAMDATA | OAM data write |
| 2138h | RDOAM | OAM data read |

#### Window Registers

| Address | Name | Description |
|---------|------|-------------|
| 2123h-2125h | W12SEL, W34SEL, WOBJSEL | Window mask settings |
| 2126h-2129h | WH0-WH3 | Window positions |
| 212Ah-212Bh | WBGLOG, WOBJLOG | Window logic |
| 212Eh-212Fh | TMW, TSW | Window mask disable |

#### Color Math

| Address | Name | Description |
|---------|------|-------------|
| 2130h | CGWSEL | Color math control A |
| 2131h | CGADSUB | Color math control B |
| 2132h | COLDATA | Fixed color for math |

#### Status Registers

| Address | Name | Description |
|---------|------|-------------|
| 2134h-2136h | MPYL/M/H | Signed multiply result (M7A×M7B) |
| 2137h | SLHV | Latch H/V counter (read=strobe) |
| 213Ch-213Dh | OPHCT/OPVCT | Latched H/V counter |
| 213Eh | STAT77 | PPU1 status (range/time over) |
| 213Fh | STAT78 | PPU2 status (field, PAL/NTSC) |

### 4.3 BG Modes

| Mode | BG1 | BG2 | BG3 | BG4 | Special Features |
|------|-----|-----|-----|-----|------------------|
| 0 | 4-color | 4-color | 4-color | 4-color | 4 layers, separate palettes |
| 1 | 16-color | 16-color | 4-color | - | BG3 priority bit |
| 2 | 16-color | 16-color | OPT | - | Offset-per-tile |
| 3 | 256-color | 16-color | - | - | Direct color option |
| 4 | 256-color | 4-color | OPT | - | Offset-per-tile |
| 5 | 16-color | 4-color | - | - | 512-pixel hi-res |
| 6 | 16-color | - | OPT | - | Hi-res + OPT |
| 7 | 256-color | EXTBG | - | - | Rotation/Scaling |

### 4.4 BG Map Entry Format (Modes 0-6)

```
15   14   13   12-10   9-0
┌────┬────┬────┬──────┬──────────────┐
│ V  │ H  │Pri │ Pal  │ Character #  │
│flip│flip│    │(0-7) │   (0-1023)   │
└────┴────┴────┴──────┴──────────────┘
```

### 4.5 Mode 7

**Transformation Matrix:**
```
┌ X' ┐   ┌ A  B ┐   ┌ X - CX ┐   ┌ CX ┐
│    │ = │      │ × │        │ + │    │
└ Y' ┘   └ C  D ┘   └ Y - CY ┘   └ CY ┘

Where:
  A = M7A = cos(θ) × ScaleX    (1.7.8 fixed point)
  B = M7B = sin(θ) × ScaleX
  C = M7C = -sin(θ) × ScaleY
  D = M7D = cos(θ) × ScaleY
  CX, CY = M7X, M7Y = Center (13-bit signed)
  X, Y = Screen coordinate + M7HOFS/VOFS
```

**M7SEL (211Ah):**
```
Bit 7-6: Screen over (0-1=wrap, 2=transparent, 3=tile 0)
Bit 1:   V-flip (256×256 screen)
Bit 0:   H-flip (256×256 screen)
```

### 4.6 Sprites (OBJ)

#### OAM Structure (544 bytes)

**Primary OAM (512 bytes = 128 × 4 bytes):**

| Byte | Content |
|------|---------|
| 0 | X position (bits 0-7) |
| 1 | Y position (bits 0-7) |
| 2 | Tile number (bits 0-7) |
| 3 | Attributes |

**Attributes (byte 3):**
```
Bit 7:   V-flip
Bit 6:   H-flip
Bit 5-4: Priority (0-3)
Bit 3-1: Palette (0-7)
Bit 0:   Tile number bit 8
```

**Secondary OAM (32 bytes = 128 × 2 bits):**
```
Bit 1: Size select (0=small, 1=large)
Bit 0: X position bit 8
```

#### OBJ Sizes (OBSEL bits 7-5)

| Value | Small | Large |
|-------|-------|-------|
| 0 | 8×8 | 16×16 |
| 1 | 8×8 | 32×32 |
| 2 | 8×8 | 64×64 |
| 3 | 16×16 | 32×32 |
| 4 | 16×16 | 64×64 |
| 5 | 32×32 | 64×64 |
| 6 | 16×32 | 32×64 |
| 7 | 16×32 | 32×32 |

#### Sprite Limits

| Limit | Description | Flag |
|-------|-------------|------|
| Range Over | >32 sprites on scanline | STAT77.6 |
| Time Over | >34 tiles (8×8) on scanline | STAT77.7 |

### 4.7 VRAM Organization

#### Tile Data Format

**Planar Format (8×8 tile):**
```
Colors    Bytes/tile    Plane arrangement
4         16 bytes      Planes 0,1 interleaved
16        32 bytes      Planes 0,1,2,3 interleaved
256       64 bytes      All 8 planes interleaved
```

**Mode 7 Format:**
- 128×128 tile map at even addresses
- 8×8 pixel tiles at odd addresses (8-bit pixels)

#### Character Base Addresses

| Register | Step Size |
|----------|-----------|
| BGnSC (map) | 2KB (1K words) |
| BGnNBA (tiles) | 8KB (4K words) |
| OBSEL (sprites) | 16KB (8K words) |

### 4.8 Layer Priority

```
Mode 0       Mode 1       Mode 7
────────     ────────     ────────
             BG3.1a*
OBJ.3        OBJ.3        OBJ.3
BG1.1        BG1.1
BG2.1        BG2.1
OBJ.2        OBJ.2        OBJ.2
BG1.0        BG1.0        BG2.1p
BG2.0        BG2.0
OBJ.1        OBJ.1        OBJ.1
BG3.1        BG3.1b*      BG1
BG4.1
OBJ.0        OBJ.0        OBJ.0
BG3.0        BG3.0a*      BG2.0p
BG4.0        BG3.0b*
Backdrop     Backdrop     Backdrop

* Mode 1 BG3 priority controlled by BGMODE.3
  .Np = per-pixel priority in Mode 7 EXTBG
```

### 4.9 Color Math

**CGWSEL (2130h):**
```
Bit 7-6: Color math force (0=never, 1=outside window, 2=inside, 3=always)
Bit 5-4: Sub screen source (0=sub screen, 1=fixed color)
Bit 1:   Direct color mode (256-color BG)
Bit 0:   Sub screen enable
```

**CGADSUB (2131h):**
```
Bit 7:   Operation (0=add, 1=subtract)
Bit 6:   Half result
Bit 5:   Backdrop enable
Bit 4:   OBJ enable (palettes 4-7 only)
Bit 3-0: BG4-BG1 enable
```

---

## 5. Audio Processing Unit (APU)

### 5.1 APU Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                      Audio Processing Unit                      │
├─────────────────┬───────────────────┬─────────────────────────┤
│   SPC700 CPU    │      S-DSP        │     64KB Audio RAM      │
│   @ 1.024 MHz   │   8 Voices        │                         │
│                 │   @ 32 kHz        │                         │
├─────────────────┴───────────────────┴─────────────────────────┤
│                    64-byte IPL Boot ROM                        │
└────────────────────────────────────────────────────────────────┘
```

### 5.2 Main CPU ↔ APU Communication

| CPU Address | APU Address | Description |
|-------------|-------------|-------------|
| 2140h | 00F4h | Port 0 |
| 2141h | 00F5h | Port 1 |
| 2142h | 00F6h | Port 2 |
| 2143h | 00F7h | Port 3 |
| 2144h-217Fh | (mirrors) | Ports mirrored |

### 5.3 SPC700 Registers

```
Register    Size      Description
─────────────────────────────────────────────
A           8-bit     Accumulator
X           8-bit     Index X
Y           8-bit     Index Y
SP          8-bit     Stack Pointer (page 01)
PC          16-bit    Program Counter
PSW         8-bit     Status: N V P B H I Z C
```

**PSW Flags:**
| Bit | Name | Description |
|-----|------|-------------|
| 7 | N | Negative |
| 6 | V | Overflow |
| 5 | P | Direct Page (0=00xx, 1=01xx) |
| 4 | B | Break flag |
| 3 | H | Half-carry |
| 2 | I | Interrupt enable |
| 1 | Z | Zero |
| 0 | C | Carry |

### 5.4 SPC700 Instruction Set Summary

```
Category          Examples
─────────────────────────────────────────────────────────────
Data Transfer     MOV, MOVW, PUSH, POP
Arithmetic        ADC, SBC, CMP, ADDW, SUBW, CMPW
Logic             AND, OR, EOR, NOT
Shift/Rotate      ASL, LSR, ROL, ROR
Bit Operations    SET1, CLR1, TSET1, TCLR1, AND1, OR1, EOR1
Branch            BRA, BEQ, BNE, BCS, BCC, BMI, BPL, BVS, BVC
                  BBC, BBS, CBNE, DBNZ
Jump/Call         JMP, CALL, PCALL, TCALL, RET, RETI, BRK
Control           NOP, SLEEP, STOP, CLRC, SETC, NOTC, CLRV
                  CLRP, SETP, EI, DI
Special           MUL, DIV, DAA, DAS, XCN
```

### 5.5 APU Memory Map

```
Address       Content
────────────────────────────────────
0000h-00EFh   Direct Page RAM / Zero Page
00F0h-00FFh   I/O Registers
0100h-01FFh   Stack (Page 1)
0200h-FFBFh   General Purpose RAM
FFC0h-FFFFh   IPL Boot ROM (can be disabled)
```

### 5.6 DSP Registers

#### Per-Voice Registers (x = 0-7)

| Address | Name | Description |
|---------|------|-------------|
| x0h | VxVOLL | Left volume (signed 8-bit) |
| x1h | VxVOLR | Right volume (signed 8-bit) |
| x2h | VxPITCHL | Pitch low (14-bit total) |
| x3h | VxPITCHH | Pitch high |
| x4h | VxSRCN | Source (sample) number |
| x5h | VxADSR1 | ADSR settings 1 |
| x6h | VxADSR2 | ADSR settings 2 |
| x7h | VxGAIN | Gain settings |
| x8h | VxENVX | Current envelope (read) |
| x9h | VxOUTX | Current sample output (read) |

#### Global Registers

| Address | Name | Description |
|---------|------|-------------|
| 0Ch | MVOLL | Main volume left |
| 1Ch | MVOLR | Main volume right |
| 2Ch | EVOLL | Echo volume left |
| 3Ch | EVOLR | Echo volume right |
| 4Ch | KON | Key on (bit per voice) |
| 5Ch | KOFF | Key off |
| 6Ch | FLG | Flags (noise clock, echo, mute, reset) |
| 7Ch | ENDX | Voice end flags (read) |
| 0Dh | EFB | Echo feedback |
| 2Dh | PMON | Pitch modulation enable |
| 3Dh | NON | Noise enable |
| 4Dh | EON | Echo enable |
| 5Dh | DIR | Sample directory page |
| 6Dh | ESA | Echo buffer start |
| 7Dh | EDL | Echo delay (4ms steps) |
| 0Fh-7Fh | C0-C7 | FIR filter coefficients |

### 5.7 BRR Audio Format

**Block Structure (9 bytes = 16 samples):**

```
Byte 0: Header
  Bit 7-4: Range/Shift (0-12)
  Bit 3-2: Filter (0-3)
  Bit 1:   Loop flag
  Bit 0:   End flag

Bytes 1-8: Sample data (4 bits × 16 samples)
```

**Filter Coefficients:**

| Filter | Coefficient 1 | Coefficient 2 |
|--------|---------------|---------------|
| 0 | 0 | 0 |
| 1 | 15/16 (0.9375) | 0 |
| 2 | 61/32 (1.90625) | -15/16 (-0.9375) |
| 3 | 115/64 (1.796875) | -13/16 (-0.8125) |

**Sample Directory:**
Located at DIR×100h, each entry is 4 bytes:
- Bytes 0-1: Start address
- Bytes 2-3: Loop address

### 5.8 ADSR Envelope

**ADSR1 (x5h):**
```
Bit 7:   ADSR enable (0=GAIN mode, 1=ADSR mode)
Bit 6-4: Decay rate (0-7)
Bit 3-0: Attack rate (0-15)
```

**ADSR2 (x6h):**
```
Bit 7-5: Sustain level (0-7 → 1/8 to 8/8)
Bit 4-0: Sustain rate (0-31)
```

### 5.9 APU Timing

| Clock | Frequency |
|-------|-----------|
| Oscillator | 24.576 MHz |
| SPC700 | 1.024 MHz (÷24) |
| DSP Sample | 32 kHz (÷768) |
| Timer 0, 1 | 8 kHz base (÷3072) |
| Timer 2 | 64 kHz base (÷384) |

---

## 6. DMA and HDMA

### 6.1 DMA Overview

The SNES has 8 DMA channels (0-7) that can transfer data between A-Bus (CPU memory) and B-Bus (PPU/APU registers).

**DMA Control Registers:**

| Address | Name | Description |
|---------|------|-------------|
| 420Bh | MDMAEN | General DMA enable (bit per channel) |
| 420Ch | HDMAEN | HDMA enable (bit per channel) |

### 6.2 DMA Channel Registers

For each channel x (0-7), registers are at 43x0h-43xBh:

| Offset | Name | Description |
|--------|------|-------------|
| 0 | DMAPx | DMA parameters |
| 1 | BBADx | B-Bus address (21xxh register) |
| 2-4 | A1TxL/H/B | A-Bus address (24-bit) |
| 5-6 | DASxL/H | Byte counter / Indirect address |
| 7 | DASBx | Indirect bank (HDMA) |
| 8-9 | A2AxL/H | HDMA table address (internal) |
| A | NTRLx | HDMA line counter |

### 6.3 DMA Parameters (43x0h)

```
Bit 7: Direction (0=A→B CPU to PPU, 1=B→A PPU to CPU)
Bit 6: Indirect mode (HDMA only)
Bit 5: Unused
Bit 4-3: A-Bus step (0=inc, 1=fixed, 2=dec, 3=fixed)
Bit 2-0: Transfer mode
```

### 6.4 Transfer Modes

| Mode | Bytes | B-Bus Pattern | Typical Use |
|------|-------|---------------|-------------|
| 0 | 1 | p | WRAM (2180h) |
| 1 | 2 | p, p+1 | VRAM (2118h/2119h) |
| 2 | 2 | p, p | OAM, CGRAM |
| 3 | 4 | p, p, p+1, p+1 | Scroll registers |
| 4 | 4 | p, p+1, p+2, p+3 | APU ports |
| 5-7 | (same as 1-3) | | |

### 6.5 HDMA Table Format

**Direct Mode:**
```
Repeat:
  1 byte: Line count (0=end, bit 7=repeat)
  N bytes: Data (N depends on transfer mode)
```

**Indirect Mode:**
```
Repeat:
  1 byte: Line count (0=end, bit 7=repeat)
  2 bytes: Pointer to data
```

### 6.6 DMA Restrictions

- WRAM-to-WRAM not possible (same chip can't read and write)
- DMA halts CPU until complete
- HDMA has priority over general DMA
- Avoid general DMA near H-Blank when HDMA active

---

## 7. Controllers and Peripherals

### 7.1 Standard Controller

**Data Format (16 bits, directly readable from 4218h-421Fh):**

```
Bit  Button
───────────
15   B
14   Y
13   Select
12   Start
11   Up
10   Down
9    Left
8    Right
7    A
6    X
5    L
4    R
3-0  Controller ID (0000 = standard pad)
```

### 7.2 Controller I/O Registers

**Auto-Read Mode:**

| Address | Name | Description |
|---------|------|-------------|
| 4218h-4219h | JOY1L/H | Controller 1 data |
| 421Ah-421Bh | JOY2L/H | Controller 2 data |
| 421Ch-421Dh | JOY3L/H | Controller 3 data (multitap) |
| 421Eh-421Fh | JOY4L/H | Controller 4 data (multitap) |

Auto-read triggered by setting NMITIMEN.0, occurs during V-Blank.

**Manual Read Mode:**

| Address | Name | Description |
|---------|------|-------------|
| 4016h | JOYWR (W) | Latch strobe (bit 0) |
| 4016h | JOYA (R) | Port 1, pin 4 data |
| 4017h | JOYB (R) | Port 2, pin 4 data |

### 7.3 Controller Type IDs

| ID (bits 15-12) | Device |
|-----------------|--------|
| 0000 | Standard Controller |
| 0001 | Mouse / Super Scope |
| 0010-1110 | Reserved |
| 1111 | No controller |

### 7.4 Super NES Mouse

**Data Format (32 bits):**

```
Bits 0-7:   Fixed (00h)
Bit 8:      Right button (1=pressed)
Bit 9:      Left button (1=pressed)
Bits 10-11: Speed mode (00=slow, 01=normal, 10=fast)
Bits 12-15: Signature (0001)
Bit 16:     Y direction (0=down, 1=up)
Bits 17-23: Y displacement (0-127)
Bit 24:     X direction (0=right, 1=left)
Bits 25-31: X displacement (0-127)
```

**Specifications:**
- Resolution: 50 counts/inch
- Max tracking speed: 250 mm/sec
- Dimensions: 98 × 64 × 35 mm

### 7.5 Super Scope

**Data Format (16 bits):**

```
Bit 15: Fire button (1=pressed)
Bit 14: Cursor button
Bit 13: Turbo switch (1=on)
Bit 12: Pause button
Bit 11: Off-screen flag
Bits 8-11: Signature (00x1)
```

**Position Detection:**
1. Super Scope detects CRT light
2. PPU latches H/V counters (via 2137h read)
3. Position read from 213Ch (H) and 213Dh (V)

### 7.6 MultiPlayer 5 (Multitap)

Connects to controller port 2, provides 4 additional controller slots.

**Reading Sequence (5-player mode):**
```
1. Write 1 then 0 to 4016h (latch)
2. Set 4201h.7 = 1
3. Read controller 2 from 4017h.0
4. Read controller 3 from 4017h.1
5. Set 4201h.7 = 0
6. Read controller 4 from 4017h.0
7. Read controller 5 from 4017h.1
```

**Limitations:**
- Must be in port 2 (not port 1)
- Cannot use Super Scope or Mouse simultaneously
- Cannot chain multiple MultiPlayer 5 units

---

## 8. Enhancement Chips

### 8.1 SA-1 (Super Accelerator)

**Specifications:**

| Feature | Value |
|---------|-------|
| CPU Core | 65C816 (same as SNES) |
| Clock Speed | 10.74 MHz (4× faster) |
| Internal RAM | 2 KB (I-RAM) |
| Max ROM | 64 Mbit |
| Max RAM | 2 Mbit (BW-RAM) |

**Features:**
- Parallel processing with main CPU
- Hardware multiply (16×16=32, 5 cycles)
- Hardware divide (16÷16=16, 5 cycles)
- Cumulative sum (40-bit result)
- Character conversion (bitmap → SNES format)
- Variable-length bit processing (1-16 bits)
- DMA between ROM, I-RAM, BW-RAM
- H/V timer synchronized to PPU

**Register Map (selected):**

| Address | Name | Access | Description |
|---------|------|--------|-------------|
| 2200h | CCNT | SNES W | SA-1 CPU control |
| 2209h | SCNT | SA-1 W | SNES CPU control |
| 2250h | MCNT | Both W | Arithmetic control |
| 2251h-2254h | MA/MB | Both W | Math operands |
| 2306h-230Ah | MR | Both R | Math result (40-bit) |

**Memory Access Speed:**

| Configuration | SA-1 Speed | SNES Speed |
|---------------|------------|------------|
| SA-1 only uses ROM | 10.74 MHz | N/A |
| Both use ROM | 5.37 MHz | 2.68 MHz |
| SNES only uses ROM | N/A | 2.68 MHz |

### 8.2 Super FX (GSU)

**Specifications:**

| Feature | Value |
|---------|-------|
| Architecture | 16-bit RISC |
| Clock Speed | 10.7 MHz (GSU-1) / 21.4 MHz (GSU-2) |
| Registers | 16 general-purpose (R0-R15) |
| Cache RAM | 512 bytes |
| Game Pak RAM | Up to 64 KB |

**Register Set:**
```
R0-R10:  General purpose
R11:     Link register (return address)
R12:     Loop counter
R13:     Loop target
R14:     ROM address pointer (with buffering)
R15:     Program counter
```

**Status Register (SFR):**
```
Bit 0:  Z (Zero)
Bit 1:  CY (Carry)
Bit 2:  S (Sign)
Bit 3:  OV (Overflow)
Bit 4:  G (Go/running)
Bit 5:  R (Reading ROM via R14)
Bit 6:  ALT1 (Instruction mode)
Bit 7:  ALT2 (Instruction mode)
Bit 11: BIRQ (Break/IRQ)
```

**Screen Modes:**
| Color Depth | Height Options |
|-------------|----------------|
| 4 colors (2bpp) | 128, 160, 192, or OBJ |
| 16 colors (4bpp) | 128, 160, 192, or OBJ |
| 256 colors (8bpp) | 128, 160, 192, or OBJ |

**Instruction Timing:**
| Location | Cycles |
|----------|--------|
| Cache RAM | 1 |
| Game Pak RAM | 3 |
| Game Pak ROM | 5-6 |

### 8.3 DSP-1 (Digital Signal Processor)

**Functions:**
- Trigonometry (sin, cos, arctan)
- 2D/3D coordinate rotation
- Vector operations
- Perspective projection
- Raster calculations

**Commands:**

| Code | Name | Description |
|------|------|-------------|
| 00h | Multiply | 16×16-bit signed multiply |
| 04h | Polar2D | 2D rotation |
| 08h | Gyrate | Object rotation on surface |
| 0Ch | Parameter | Set projection parameters |
| 0Ah | Raster | Calculate raster data |
| 06h | Project | 3D to 2D projection |
| 10h | MemTest | Memory test |
| 1Ch | Polar3D | 3D rotation (Y→X→Z order) |
| 11h | Triangle | Trigonometric calculation |
| 01h | Inverse | Inverse calculation |

**Memory Map:**
```
LoROM: 20-3F:8000-BFFF, 20-3F:C000-FFFF
HiROM: 00-1F:6000-6FFF, 80-9F:6000-6FFF
```

### 8.4 Other Enhancement Chips

| Chip | Type | Games | Purpose |
|------|------|-------|---------|
| DSP-2 | NEC uPD77C25 | 1 | Bitmap scaling (Dungeon Master) |
| DSP-3 | NEC uPD77C25 | 1 | Game logic (SD Gundam GX) |
| DSP-4 | NEC uPD77C25 | 1 | Racing math (Top Gear 3000) |
| CX4 | RISC @ 20MHz | 2 | Wire-frame 3D (Mega Man X2/X3) |
| OBC1 | Controller | 1 | Sprite management (Metal Combat) |
| S-DD1 | Decompressor | 2 | Data decompression |
| SPC7110 | Decompressor | 3 | Data decompression + RTC |
| ST010 | NEC uPD96050 | 2 | AI/Math (F1 Race of Champions) |
| ST011 | NEC uPD96050 | 1 | AI/Math (Hayazashi Nidan Morita Shogi) |
| ST018 | ARM @ 21.4MHz | 1 | Shogi AI (Hayazashi Nidan Morita Shogi 2) |
| S-RTC | RTC | 1 | Real-time clock |
| SGB | CPU + LCD | 2 | Game Boy emulation |

---

## 9. Cartridge Format

### 9.1 ROM Header Location

| Map Mode | Header Address | ROM File Offset |
|----------|----------------|-----------------|
| LoROM | 00FFB0h-00FFFFh | 007FB0h |
| HiROM | 00FFB0h-00FFFFh | 00FFB0h |
| ExHiROM | 40FFB0h-40FFFFh | 40FFB0h |

Add 200h if file has copier header ((filesize AND 3FFh) = 200h).

### 9.2 Header Structure

**Main Header (FFC0h-FFDFh):**

| Offset | Size | Content |
|--------|------|---------|
| FFC0h | 21 | Game Title (ASCII, space-padded) |
| FFD5h | 1 | Map Mode / Speed |
| FFD6h | 1 | Chipset |
| FFD7h | 1 | ROM Size (log2) |
| FFD8h | 1 | RAM Size (log2) |
| FFD9h | 1 | Country/Region |
| FFDAh | 1 | Developer ID (33h = extended) |
| FFDBh | 1 | Version |
| FFDCh | 2 | Checksum complement |
| FFDEh | 2 | Checksum |

**Extended Header (FFB0h-FFBFh, when FFDAh=33h):**

| Offset | Size | Content |
|--------|------|---------|
| FFB0h | 2 | Maker code (ASCII) |
| FFB2h | 4 | Game code (ASCII) |
| FFB6h | 6 | Reserved |
| FFBCh | 1 | Expansion FLASH size |
| FFBDh | 1 | Expansion RAM size |
| FFBEh | 1 | Special version |
| FFBFh | 1 | Chipset subtype |

### 9.3 Map Mode (FFD5h)

```
Bit 5:   Always 1
Bit 4:   Speed (0=slow 200ns, 1=fast 120ns)
Bit 3-0: Mode
  0 = LoROM (Mode 20)
  1 = HiROM (Mode 21)
  2 = LoROM + S-DD1 (Mode 22)
  3 = LoROM + SA-1 (Mode 23)
  5 = ExHiROM (Mode 25)
  A = HiROM + SPC7110
```

### 9.4 Chipset (FFD6h)

```
High nibble: Co-processor type
  0x = DSP
  1x = GSU (Super FX)
  2x = OBC1
  3x = SA-1
  4x = S-DD1
  5x = S-RTC
  Ex = Other (SGB, Satellaview)
  Fx = Custom (subtype in FFBFh)

Low nibble: Configuration
  0 = ROM
  1 = ROM + RAM
  2 = ROM + RAM + Battery
  3 = ROM + Coprocessor
  4 = ROM + Coprocessor + RAM
  5 = ROM + Coprocessor + RAM + Battery
  6 = ROM + Coprocessor + Battery
```

**Common Values:**

| Value | Configuration |
|-------|---------------|
| 00h | ROM only |
| 02h | ROM + RAM + Battery |
| 03h | ROM + DSP |
| 05h | ROM + DSP + RAM + Battery |
| 15h | ROM + GSU + RAM + Battery |
| 35h | ROM + SA-1 + RAM + Battery |
| 45h | ROM + S-DD1 + RAM + Battery |
| E3h | Super Game Boy |

### 9.5 Country Codes (FFD9h)

| Code | Region | Video |
|------|--------|-------|
| 00h | Japan | NTSC |
| 01h | USA/Canada | NTSC |
| 02h | Europe | PAL |
| 03h | Scandinavia | PAL |
| 06h | France | SECAM/PAL |
| 09h | Germany | PAL |
| 0Dh | South Korea | NTSC |
| 10h | Brazil | PAL-M (60Hz) |
| 11h | Australia | PAL |

### 9.6 Exception Vectors (FFE0h-FFFFh)

| Address | Native Mode | Emulation Mode |
|---------|-------------|----------------|
| FFE4h | COP | - |
| FFE6h | BRK | - |
| FFE8h | ABORT | - |
| FFEAh | NMI | - |
| FFEEh | IRQ | - |
| FFF4h | - | COP |
| FFF8h | - | ABORT |
| FFFAh | - | NMI |
| FFFCh | RESET | RESET |
| FFFEh | - | IRQ/BRK |

---

## 10. Timing Specifications

### 10.1 Clock Frequencies

**NTSC:**
| Clock | Frequency | Derivation |
|-------|-----------|------------|
| Crystal (X1) | 21.477270 MHz | Base |
| Master | 21.477270 MHz | ÷1 |
| Dot | 5.369318 MHz | ÷4 |
| CPU Fast | 3.579545 MHz | ÷6 |
| CPU Medium | 2.684659 MHz | ÷8 |
| CPU Slow | 1.789773 MHz | ÷12 |

**PAL:**
| Clock | Frequency | Derivation |
|-------|-----------|------------|
| Crystal (X1) | 17.734475 MHz | Base |
| Master | 21.281370 MHz | ×6÷5 |
| Dot | 5.320343 MHz | ÷4 |
| CPU Fast | 3.546895 MHz | ÷6 |
| CPU Medium | 2.660171 MHz | ÷8 |
| CPU Slow | 1.773448 MHz | ÷12 |

**APU:**
| Clock | Frequency | Derivation |
|-------|-----------|------------|
| Crystal (X2) | 24.576 MHz | Base |
| SPC700 | 1.024 MHz | ÷24 |
| DSP Sample | 32 kHz | ÷768 |
| Timer 0/1 | 8 kHz | ÷3072 |
| Timer 2 | 64 kHz | ÷384 |

### 10.2 Scanline Timing

| Type | Master Cycles | Dots |
|------|---------------|------|
| Normal | 1364 | 341 (340 visible) |
| Short (NTSC) | 1360 | 340 |
| Long (PAL interlace) | 1368 | 341 |

**Short scanline:** Line 240, NTSC, non-interlace, field 1
**Long scanline:** Line 311, PAL, interlace, field 1

### 10.3 Frame Timing

| Mode | Total Lines | Active | V-Blank |
|------|-------------|--------|---------|
| NTSC 224 | 262 | 1-224 | 225-261 |
| NTSC 239 | 262 | 1-239 | 240-261 |
| PAL 224 | 312 | 1-224 | 225-311 |
| PAL 239 | 312 | 1-239 | 240-311 |

### 10.4 H/V Event Timeline

| H-Dot | V-Line | Event |
|-------|--------|-------|
| 0 | 0 | Clear V-Blank flag, reset NMI flag |
| 0.5 | 225/240 | Set NMI flag |
| 1 | 0 | Toggle field flag |
| 1 | any | Clear H-Blank flag |
| 6 | 0 | HDMA table reload |
| 10 | 225/240 | Reload OAM address |
| 22-277 | 1-224/239 | Active display |
| ~33-96 | 225/240 | Joypad auto-read |
| ~134 | any | DRAM refresh (40 cycles) |
| 274 | any | Set H-Blank flag |
| 278 | 0-224/239 | HDMA transfers |
| HTIME+3.5 | VTIME | H/V IRQ trigger |

### 10.5 CPU Timing Notes

- DRAM refresh: 40 master cycles per scanline (~3% overhead)
- DMA pauses CPU completely
- HDMA: ~8 cycles overhead + transfer time per channel

---

## 11. Programming Reference

### 11.1 Initialization Sequence

```assembly
; Minimal SNES initialization
        SEI                     ; Disable IRQs
        CLC
        XCE                     ; Switch to native mode
        REP     #$38            ; 16-bit A/X/Y, clear D flag

        LDA     #$0000
        TCD                     ; D = 0000

        LDA     #$01FF
        TCS                     ; S = 01FF

        SEP     #$20            ; 8-bit A

        LDA     #$8F
        STA     $2100           ; Forced blank, brightness 0

        STZ     $2101           ; OAM settings
        STZ     $2102
        STZ     $2103
        ; ... initialize remaining PPU registers ...

        LDA     #$00
        STA     $420C           ; Disable HDMA
        STA     $420B           ; Disable DMA

        LDA     #$FF
        STA     $4200           ; Enable NMI, auto-joypad
```

### 11.2 Common Register Settings

**Enable Display:**
```assembly
        LDA     #$0F
        STA     $2100           ; Full brightness, display on
```

**Set BG Mode:**
```assembly
        LDA     #$01            ; Mode 1
        STA     $2105
```

**VRAM Write:**
```assembly
        LDA     #$80
        STA     $2115           ; Increment on high byte write

        REP     #$20
        LDA     #$0000          ; VRAM address
        STA     $2116

        LDA     #$1234          ; Data
        STA     $2118           ; Write to VRAM
```

**DMA to VRAM:**
```assembly
        LDA     #$01
        STA     $4300           ; A→B, increment, mode 1 (word)
        LDA     #$18
        STA     $4301           ; B-bus = 2118/2119

        REP     #$20
        LDA     #SourceAddr
        STA     $4302
        SEP     #$20
        LDA     #^SourceAddr
        STA     $4304

        REP     #$20
        LDA     #ByteCount
        STA     $4305

        SEP     #$20
        LDA     #$01
        STA     $420B           ; Start DMA channel 0
```

### 11.3 Useful Formulas

**VRAM Address for BG Map:**
```
VRAM_Addr = (SC_Base × 2048) + (Y × 64) + (X × 2)
```

**VRAM Address for Tile:**
```
VRAM_Addr = (NBA_Base × 8192) + (Tile# × BytesPerTile)
```

**Mode 7 Rotation:**
```
A = cos(angle) × 256 / scale
B = sin(angle) × 256 / scale
C = -sin(angle) × 256 / scale
D = cos(angle) × 256 / scale
```

**Pitch to Frequency (APU):**
```
Frequency = (Pitch × 32000) / 4096
```

### 11.4 Hardware Limitations

| Resource | Limit |
|----------|-------|
| Sprites per scanline | 32 |
| Sprite tiles per scanline | 34 (8×8) |
| Max ROM (standard) | 4 MB |
| Max ROM (with chip) | 8+ MB |
| Max SRAM | 256 KB |
| VRAM per frame | ~5 KB (during V-Blank) |
| DMA speed | ~2.68 MB/s |

### 11.5 Best Practices

1. **VRAM Access**: Only during V-Blank or Forced Blank
2. **OAM Access**: Prefer H-Blank, V-Blank, or Forced Blank
3. **CGRAM Access**: Can sometimes work during H-Blank
4. **DMA During V-Blank**: Stay within ~5KB to avoid overflow
5. **Avoid DMA near H-Blank**: When HDMA channels active
6. **Controller Reading**: Wait for auto-read to complete (HVBJOY.0)
7. **NMI Handler**: Keep short, acknowledge with RDNMI read

---

## Appendix A: Quick Register Reference

### PPU Registers (21xxh)

| Addr | Name | Description |
|------|------|-------------|
| 2100 | INIDISP | Brightness/Forced blank |
| 2101 | OBSEL | Sprite size/base |
| 2102-03 | OAMADDR | OAM address |
| 2104 | OAMDATA | OAM data write |
| 2105 | BGMODE | BG mode |
| 2106 | MOSAIC | Mosaic |
| 2107-0A | BGnSC | BG map base/size |
| 210B-0C | BGnNBA | BG tile base |
| 210D-14 | BGnOFS | BG scroll |
| 2115 | VMAIN | VRAM increment mode |
| 2116-17 | VMADDR | VRAM address |
| 2118-19 | VMDATA | VRAM data write |
| 211A | M7SEL | Mode 7 settings |
| 211B-20 | M7x | Mode 7 matrix |
| 2121 | CGADDR | Palette address |
| 2122 | CGDATA | Palette data write |
| 2123-2B | Window | Window registers |
| 212C-2D | TM/TS | Layer enable |
| 2130-32 | Color Math | Color math settings |
| 2133 | SETINI | Display settings |
| 2134-36 | MPY | Math result |
| 2137 | SLHV | Latch H/V |
| 2138 | OAMRD | OAM data read |
| 2139-3A | VMRD | VRAM data read |
| 213B | CGRD | Palette data read |
| 213C-3D | HV | H/V counter |
| 213E-3F | STAT | PPU status |

### CPU Registers (42xxh)

| Addr | Name | Description |
|------|------|-------------|
| 4200 | NMITIMEN | NMI/IRQ enable |
| 4201 | WRIO | Programmable I/O out |
| 4202-03 | WRMPYA/B | Multiply |
| 4204-06 | WRDIV | Divide |
| 4207-0A | HTIME/VTIME | H/V timer |
| 420B | MDMAEN | DMA enable |
| 420C | HDMAEN | HDMA enable |
| 420D | MEMSEL | ROM speed |
| 4210 | RDNMI | NMI flag |
| 4211 | TIMEUP | IRQ flag |
| 4212 | HVBJOY | Status |
| 4213 | RDIO | Programmable I/O in |
| 4214-17 | RDDIV/RDMPY | Math results |
| 4218-1F | JOYn | Joypad data |

---

## Appendix B: Memory Speed Map

```
                    00-3F / 80-BF          40-7D / C0-FF
           ┌────────────────────────┬──────────────────────┐
    0000h  │  WRAM Mirror (2.68M)   │                      │
    1FFFh  │                        │    HiROM / WS2       │
           ├────────────────────────┤    (2.68M or 3.58M   │
    2000h  │  I/O (3.58M)           │     via MEMSEL)      │
    3FFFh  │                        │                      │
           ├────────────────────────┤                      │
    4000h  │  Joypad (1.78M)        │                      │
    41FFh  │                        │                      │
           ├────────────────────────┤                      │
    4200h  │  I/O (3.58M)           │                      │
    5FFFh  │                        │                      │
           ├────────────────────────┤                      │
    6000h  │  Expansion (2.68M)     │                      │
    7FFFh  │                        │                      │
           ├────────────────────────┤                      │
    8000h  │  LoROM (2.68M / 3.58M) │                      │
    FFFFh  │  (WS1 or WS2)          │                      │
           └────────────────────────┴──────────────────────┘

                    7E-7F
           ┌────────────────────────┐
    0000h  │                        │
           │       WRAM             │
           │     (2.68 MHz)         │
           │      128 KB            │
    FFFFh  │                        │
           └────────────────────────┘
```

---

*Document compiled from Nintendo SNES Development Manuals (Book I & II) and nocash fullsnes specifications. For educational and preservation purposes.*
