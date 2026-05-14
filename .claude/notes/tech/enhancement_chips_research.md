---
name: SNES Enhancement Chips — SDK extension research
description: Research notes on extending OpenSNES to support SNES cartridge coprocessors (SuperFX, DSP, SA-1, etc.)
type: project
---

## Context (2026-03-21)

The SNES base hardware (65816 @ 3.58MHz + SPC-700) is limited. Many landmark
games used cartridge enhancement chips for 3D, compression, or faster processing.
WLA-DX already supports SuperFX assembly (`wla-superfx`).

## Enhancement Chips Overview

Source: https://en.wikipedia.org/wiki/List_of_Super_NES_enhancement_chips

| Chip | Purpose | Notable Games | WLA-DX Support |
|------|---------|---------------|----------------|
| **SuperFX (GSU)** | RISC coprocessor for 3D/2D effects | Star Fox, Yoshi's Island, Doom | `wla-superfx` ✓ |
| **DSP-1/2/3/4** | Math coprocessor (matrix, trig) | Pilotwings, Mario Kart, Top Gear 3000 | No (hardware registers) |
| **SA-1** | Fast 65816 @ 10.74MHz + DMA | Kirby Super Star, Super Mario RPG | No (same ISA as 65816) |
| **S-DD1** | Decompression (graphics streaming) | Star Ocean, Street Fighter Alpha 2 | No (hardware registers) |
| **SPC7110** | Decompression + extended ROM | Far East of Eden Zero | No |
| **Cx4** | Math coprocessor (wireframe 3D) | Mega Man X2, Mega Man X3 | No |
| **OBC-1** | OAM expansion (guided sprites) | Metal Combat | No |

## SuperFX — Best Starting Point

**Why:** WLA-DX already has `wla-superfx`. Star Fox and Yoshi's Island are
iconic. Active homebrew community. Emulator support is mature (snes9x, Mesen2).

**Documentation:**
- https://en.wikipedia.org/wiki/Super_FX
- https://en.wikibooks.org/wiki/Super_NES_Programming/Super_FX_tutorial
- GSU instruction set: 16-bit RISC, 16 registers, bitmap rendering pipeline

**What extending OpenSNES would require:**
1. ROM header support for SuperFX cartridge type ($FFD6 = $13-$1A)
2. Memory map for SuperFX cartridges (different from standard LoROM/HiROM)
3. `wla-superfx` integration in our build system (separate ASM for GSU code)
4. Library functions for 65816↔GSU communication (register mapping, IRQ)
5. Example: simple polygon renderer or sprite scaler

## SA-1 — Highest Impact for Game Developers

**Why:** Same ISA as 65816 but 3× faster (10.74MHz). Games get a second CPU
for free. No new instruction set to learn. Super Mario RPG used it for the
battle system.

**What it requires:** Different memory map, DMA setup, IRQ routing between
main 65816 and SA-1. Our compiler output would work on SA-1 unmodified
(same ISA). Just need ROM header + memory map support.

## Next Steps

- [ ] Study SuperFX memory map and cartridge header format
- [ ] Test if `wla-superfx` can assemble a minimal GSU program
- [ ] Research SA-1 memory map (possibly simpler to support since same ISA)
- [ ] Check emulator support for homebrew SuperFX/SA-1 ROMs
- [ ] Evaluate: which chip provides the most value for modern homebrew?
