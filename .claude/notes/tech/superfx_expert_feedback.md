---
name: SuperFX expert feedback
description: Detailed technical feedback from experienced SNES developer on our SuperFX implementation. MUST consult when debugging PLOT, POR, CACHE, or branch delay slot issues.
type: reference
---

## Source
Feedback from user's friend (experienced SNES game developer) on 2026-03-23.

## Key Findings

### Branch Delay Slot (CONFIRMED)
- SuperFX is a RISC processor with a 1-instruction branch delay slot
- Instruction after BNE/BRA ALWAYS executes (taken or not)
- Minimum 1 NOP after last BNE before STOP
- 2 NOPs recommended for MC1 (Mario Chip 1) — store→STOP bug can prevent GO flag clearing
- 4 NOPs is conservative but safe
- Dummy NOP after STOP is required ("putting GSU in WAIT won't clear pipeline opcode")

### POR (Plot Option Register) via CMODE
- Bit 0: Transparent (0=skip color 0, 1=plot color 0)
- Bit 1: Dither (0=off, 1=alternates nibbles based on X^Y parity)
- Bit 2-3: Color mode (freeze high nibble, duplicate)
- DITHERING BUG: "doesn't handle transparent pixels properly — determines nibble in parallel with checking bottom nibble is zero"
- Clear framebuffer: use POR=$01 (opaque) during clear, POR=$00 during render

### CACHE
- NOT required for PLOT correctness
- ~3x performance: 1 cycle/instruction (cached) vs 3 cycles (ROM@10MHz) vs 5 cycles (ROM@21MHz)
- 512 bytes, 32 blocks of 16 bytes
- Place CACHE just before the PLOT loop

### CLSR Clock Speed
- MC1: 10.74 MHz only
- GSU-1/GSU-2: 10.74 or 21.47 MHz
- `IBT R0, #$01; sta.l $3034` to enable fast clock
- PLOT hardware has own timing, not affected by CLSR

### snes9x Detection
- Cart types $13/$14/$15/$1A should be recognized
- Check: map mode $7FD5=$20, extended header $7FB0-$7FBF, checksum validity
- Compare header byte-by-byte with Star Fox dump

### Reference Projects
- **PeterLemon/SNES** (github.com/PeterLemon/SNES) — CHIP/GSU/4BPP/PlotPixel/256x128/ has complete working PLOT example
- **libSFX** (github.com/Optiroc/libSFX) — full SNES/SuperFX framework
- **GSU-Development-Kit** (github.com/DiscoManOfficial/GSU-Development-Kit) — modern GSU dev kit

## DMA 60 FPS — HDMA Blanking (PeterLemon pattern, 2026-03-26)

**Pattern**: HDMA table on channel 1 controls REG_INIDISP ($2100) per scanline:
- First ~19 scanlines: forced blank (bit 7 = screen OFF)
- Middle ~185 scanlines: normal display (brightness 15)
- Last scanline: forced blank

During forced blank scanlines + VBlank, DMA bandwidth to VRAM is unlimited.
DMA launched at scanline ~205 (end of active display).
Result: full 16KB framebuffer transferred in 1 frame = **60 FPS**.

Trade-off: ~19 scanlines of black bars at top (same as Star Fox).

## Double-Buffering Fix (ARM9/casfx pattern, 2026-03-26)

**Root cause of our blinking**: swapping buffer before complete transfer.

**Correct pattern (split-frame)**:
1. Two VRAM areas: SCREEN1=$0000, SCREEN2=$3000
2. Transfer half the framebuffer per VBlank (2 VBlanks total)
3. **Swap ONLY after both halves are written** (not after each half)
4. Swap via REG_BG12NBA (tile base address pointer), not DMA source
5. GsuWaitForStop() before DMA, GsuResume() after

**Our bug**: we swapped after each VBlank instead of after 2 VBlanks.

## snes9x Header Fix (2026-03-26)

**Changes needed for snes9x detection**:
- $FFD6 = `$13` (ROM+GSU, like Star Fox) NOT `$14`
- SRAM size: move from `$FFD8` to `$FFBD` (Expansion RAM Size field)
- $FFD8 = `$00` (SuperFX quirk)
- $FFD5 = `$20` (LoROM, confirmed)
- VCR $303B: $01=MC1, $02=GSU-0, $03=GSU-1, $04=GSU-2
