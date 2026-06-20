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

## SA-1 SIWP/CIWP polarity — DISPUTED, kept at $FF (2026-06-20)

Investigated the P1-3 finding (crt0 writes `$FF` to SIWP `$2229` under the
guess "*maybe bit=1 means WRITABLE*"). **Documentation and our emulators
flatly disagree on the polarity — and the emulators win for now.**

Documentation side (says `$FF` is wrong, `$00` writable):
- [Super Famicom Dev Wiki — SA-1 registers](https://wiki.superfamicom.org/sa-1-registers):
  "$2229 ... 0 = Disable protection, 1 = Enable protection"; full write
  access = `$00`.
- [PeterLemon/SNES SNES_SA-1.INC](https://github.com/PeterLemon/SNES/blob/master/LIB/SNES_SA-1.INC)
  names it "SA-1 I-RAM Write Protection (S-CPU Controlled)".

Emulator side (says `$FF` is correct), tested in Mesen2 (our accuracy
reference) on `sa1_hello`:
- `$FF` → crt0 I-RAM self-test passes, `sa1_status=$A5`, `$3000=$A5`.
- `$00` → self-test **fails**, `sa1_status=$FF`, `$3000=$DB` (the SNES-CPU
  write to I-RAM was blocked).
- snes9x agrees with Mesen2 (the CI suite is green with `$FF`).

### LESSON (the important part)

I initially trusted the wiki, flipped crt0 to `$00`, and it **broke** SA-1
in Mesen2. Reverted to `$FF`. *Empirical test before doc-driven "fix"* — the
project's own rule (`debugging.md`: verify in Mesen2, never guess) would
have caught this up front. Don't flip this again without a **real SA-1
cartridge** test that proves the wiki polarity; the crt0 self-test
(`_sa1_iram_fail` → `sa1_status=$FF`) is the runtime safety net either way.

**State:** crt0 stays at `$FF` (both sites); `KNOWN_LIMITATIONS.md`,
`docs/hardware/REGISTERS.md`, `docs/tutorials/sa1.md` rewritten to present
the conflict honestly instead of asserting either polarity. **Open:**
hardware verification; and the whose-writes detail (does SIWP gate S-CPU
or SA-1 writes?) — unresolved by primary sources.

## Next Steps

- [ ] Study SuperFX memory map and cartridge header format
- [ ] Test if `wla-superfx` can assemble a minimal GSU program
- [ ] Research SA-1 memory map (possibly simpler to support since same ISA)
- [ ] Check emulator support for homebrew SuperFX/SA-1 ROMs
- [ ] Evaluate: which chip provides the most value for modern homebrew?
