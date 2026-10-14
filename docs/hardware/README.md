# SNES Hardware Overview {#hardware_overview}

Understanding SNES hardware is essential for effective game development.

## Contents

| Document | Description |
|----------|-------------|
| [The PPU](PPU.md) | Background modes, tiles, VRAM, palettes, sprite sizes |
| [The APU](APU.md) | SPC700, S-DSP, audio RAM, BRR samples, echo |
| [Memory Map](MEMORY_MAP.md) | RAM, ROM, and register layout |
| [Registers](REGISTERS.md) | Complete hardware register reference |
| [OAM](OAM.md) | Sprite attribute memory documentation |

## Specifications

| Component | Specification |
|-----------|---------------|
| CPU | 65816 @ 3.58 MHz (NTSC) / 3.55 MHz (PAL) |
| Work RAM | 128 KB |
| PPU | 64 KB VRAM, 544-byte OAM, 512-byte CGRAM: @ref hardware_ppu |
| APU | SPC700 + S-DSP with 64 KB of audio RAM: @ref hardware_apu |

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

### Tiles, sprites and background modes

Everything on screen is built from 8x8 tiles: background layers arranged by
a tilemap, and up to 128 movable sprites. The eight background modes trade
layers against colours; Mode 1 is the usual choice. The mode table, the
sprite sizes and the per-line limits are in @ref hardware_ppu.

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
