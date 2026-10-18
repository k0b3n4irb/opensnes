# SNES Memory Map {#memory_map}

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
| $7E:0100-$7E:1FFF | 7.75 KB | Variables, BSS, and the stack |
| $7E:2000-$7E:FFFF | 56 KB | General use |
| $7F:0000-$7F:FFFF | 64 KB | General use / buffers |

In OpenSNES the stack starts at `$1FFF` and grows down, under the C
globals of the `$0000-$1FFF` band; `FAR` objects live above `$2000`
(`docs/tutorials/far_ram.md`).

### Video RAM, OAM and CGRAM

The PPU's three memories are not in the CPU's address space: they are
reached through ports ($2116-$2119 for VRAM, $2102-$2104 for OAM,
$2121-$2122 for CGRAM). Their sizes, layout and formats are in
@ref hardware_ppu; OAM byte by byte is in @ref oam.

### Hardware registers

The registers mapped at $2100-$21FF, $4200-$44FF and $4300-$437F are
indexed, then detailed, in @ref registers.

## Example: Setting Up Memory

```c
// A plain global lives in the 8 KB band $00:0000-$1FFF
u8 game_variables[256];
u16 frame_counter;

// A large buffer goes to far RAM, $7E:2000-$FFFF
FAR u8 large_buffer[16384];
```

The linker places both; the build stops if the plain band overflows. See
[Far RAM](../tutorials/far_ram.md).

## Enhancement Chip Memory

### SA-1 Cartridge

The SA-1 adds a second 65816 CPU and extra RAM to the cartridge.

```
Address         Size      Description
────────────────────────────────────────────────────────────────
$2200-$23FF     512 B     SA-1 registers (CPU control, DMA, math, IRQ)
$3000-$37FF     2 KB      I-RAM (shared between SNES CPU and SA-1)
$6000-$7FFF     8 KB      BW-RAM window (8 KB view into BW-RAM)
$40-$4F:0000    up to     BW-RAM (up to 256 KB of fast static RAM; the rest of the range mirrors)
                256 KB
```

**I-RAM ($3000-$37FF)** is the primary communication channel between the
SNES main CPU and the SA-1. Both processors can read and write it, gated
by the SIWP ($2229) and CIWP ($222A) write-protection registers.

**BW-RAM** is battery-backed SRAM mapped in banks $40-$4F, with an 8 KB
window visible at $6000-$7FFF for convenient access from bank $00 code.

### Super FX Cartridge

The Super FX (GSU) has its own register space and uses cartridge SRAM as
a shared framebuffer.

```
Address         Size      Description
────────────────────────────────────────────────────────────────
$3000-$303F     64 B      GSU registers (R0-R15, SFR, SCMR, PBR, etc.)
$3100-$32FF     512 B     GSU code cache (the SNES can read and write it)
$70-$71:0000    32-64 KB  Cartridge SRAM (shared framebuffer + data)
```

**GSU registers ($3000-$303F)** control the Super FX processor. The SNES
CPU writes parameters and starts the GSU by writing to R15 ($301E-$301F).

**Code cache ($3100-$32FF)** holds the currently cached GSU instructions.
The SNES CPU can also write opcodes there, in 16-byte lines, to load code
into the cache itself (fullsnes, "Writing to Code-Cache (by SNES CPU)").

**Cartridge SRAM ($70-$71)** serves as the GSU framebuffer. The Super FX
PLOT instruction writes pixels here, and the SNES reads it back for
display. Bus ownership is controlled by the SCMR register ($303A) —
the SNES and GSU cannot access SRAM simultaneously.

## Further Reading

- [Hardware Registers](REGISTERS.md) - PPU, DMA, I/O registers
- [The PPU](PPU.md) - VRAM, OAM and CGRAM
- [OAM](OAM.md) - Sprite attribute memory
- [SNESdev Wiki Memory Map](https://snes.nesdev.org/wiki/Memory_map)
