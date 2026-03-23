# SuperFX (GSU) -- Complete Knowledge Base

## Summary

SuperFX support complete through Phase 2. Build system, boot, SRAM communication,
and PLOT bitmap rendering all validated on bsnes. 16-color rainbow gradient renders
correctly at 21.47 MHz with CACHE optimization.

## Emulator Compatibility

| Emulator | Support | Notes |
|----------|---------|-------|
| **bsnes** | Reference | Cycle-accurate. Use for all validation. |
| **Mesen2** | Partial | Boot/registers OK. PLOT loops affected by same delay slot behavior. |
| **snes9x** | Not detected | Cart type $14 not recognized. Checksum or extended header issue? Compare with Star Fox. |

## SuperFX Assembly Rules (MANDATORY)

### Rule 1: NOP after every BNE/BRA (branch delay slot)
The SuperFX is a pipelined RISC processor. The instruction after a branch
ALWAYS executes (taken or not). Without NOP, side effects occur.
```asm
    DEC R4
    BNE _loop
    NOP              ; MANDATORY — delay slot
    INC R2           ; this executes normally (not in delay slot)
```

### Rule 2: NOP x2 before STOP (STOP pre-fetch + MC1 safety)
STOP in the pipeline pre-fetch halts the GSU immediately. MC1 (Mario Chip 1)
has an additional bug where store-then-STOP can prevent the GO flag from clearing.
```asm
    BNE _loop
    NOP              ; delay slot
    NOP              ; MC1 store-STOP safety
    STOP
    NOP              ; dummy opcode post-STOP (required: "WAIT won't clear pipeline")
```

### Rule 3: RPIX after PLOT loops (pixel cache flush)
PLOT writes to an internal 8-pixel cache. The cache auto-flushes at tile
boundaries (X/8 changes), but the LAST 8 pixels of each row stay in cache.
RPIX forces a flush as a side effect.
```asm
_col:
    PLOT
    DEC R4
    BNE _col
    NOP              ; delay slot
    IBT R1, #$00
    RPIX             ; flush pixel cache to SRAM
```

### Rule 4: CACHE before hot loops (~6x speedup)
CACHE loads 512 bytes of code into the GSU instruction cache.
With cache + 21 MHz: ~1 cycle/instruction vs ~5 cycles from ROM.
```asm
    CACHE            ; loads next 512 bytes into instruction cache
_loop:
    PLOT
    ...
```

## Phase Status

### Phase 0 -- Build system ✅
- `wla-superfx` built and installed
- 2-step pipeline: `.sfx` -> wla-superfx -> wlalink -b -> `.sfx.bin` -> .incbin
- `USE_SUPERFX=1`, `GSUSRC` for GSU sources
- `hdr_superfx.asm` (cart type $14, LoROM, SRAM 32K)
- Library build variant `superfx`

### Phase 1 -- Boot + communication ✅
- GSU detected via VCR ($303B) = $04
- Registers R0-R15 bidirectional (R0=$CAFE confirmed)
- SRAM shared: STB ($42/$55) + STW ($BEEF) confirmed
- WRAM stub mandatory for all GSU launches (ROM bus exclusive)
- NMI must be disabled during GSU execution

### Phase 2 -- PLOT bitmap rendering ✅
- 16-color rainbow gradient via PLOT hardware
- Column-major tilemap: `tilemap[row*32+col] = col*16 + row`
- SCMR=$19 (4bpp + RAN + RON), SCBR=$00, CMODE POR=$00
- CLSR=$01 (21.47 MHz), CFGR=$80 (IRQ mask)
- CACHE before both loops (clear + PLOT)
- RPIX after inner PLOT loop (pixel cache flush)
- Full pipeline: GSU PLOT -> SRAM $70:0000 -> DMA 16KB -> VRAM -> screen

## Technical Architecture

### GSU (Graphics Support Unit)
- 16-bit RISC CPU, 10.74 / 21.47 MHz (CLSR selectable)
- 16 registers (R0-R15), R15 = PC
- Unique ISA (~55 mnemonics), NOT 65816 compatible
- No hardware stack (LINK/RET via R11)
- 512-byte instruction cache (CACHE instruction)
- PLOT: hardware pixel-to-bitplane conversion
- **No C compiler** -- assembly only

### Memory
```
         ROM (shared, exclusive bus)
              |
    +---------+---------+
    |         |         |
 SNES CPU   SRAM     GSU
 3.58 MHz  32-128KB  10.74/21.47 MHz
 WRAM/PPU  $70:0000  NO WRAM/PPU/APU
```

### Key Registers (SNES CPU side, $3000-$303F)

| Register | Address | Role |
|----------|---------|------|
| R0-R15 | $3000-$301F | 16-bit GP. **Writing R15H starts GSU** |
| SFR | $3030 | Status. Bit 5 = GO (running) |
| BRAMR | $3033 | Bit 0: SRAM writable by SNES CPU |
| PBR | $3034 | Program bank (ROM bank for GSU code) |
| CFGR | $3037 | Config: bit 7 = IRQ mask |
| SCBR | $3038 | Screen base (SRAM offset = SCBR * 1024) |
| CLSR | $3039 | Clock: $00=10.74MHz, $01=21.47MHz |
| SCMR | $303A | Screen mode + bus ownership |
| VCR | $303B | Chip version (read-only, $04) |

### SCMR ($303A)
```
Bit 5: HT1    Bit 4: RON    Bit 3: RAN    Bit 2: HT0    Bits 1-0: MD
Height: 00=128, 01=160, 10=192, 11=OBJ
Color:  00=2bpp, 01=4bpp, 11=8bpp
RON=1: GSU owns ROM (SNES CPU CANNOT read ROM!)
RAN=1: GSU owns RAM (SNES CPU CANNOT read SRAM!)
```

### POR (Plot Option Register, via CMODE instruction)
```
Bit 0: Transparent  (0=skip color 0, 1=plot color 0)
Bit 1: Dither       (0=off, 1=alternate nibbles on X^Y parity)
Bit 2: High Nibble  (duplicate high nibble to low)
Bit 3: Freeze High  (keep upper 4 bits of COLR)
Bit 4: OBJ mode
```
WARNING: Dither has a hardware bug with transparent pixels.

### PLOT Address Formula (4bpp, height=128, SCBR=0)
```
address = (X/8)*512 + (Y/8)*32 + (Y%8)*2
```
Tiles stored COLUMN-MAJOR: tile_index = col * 16 + row

### Launch Sequence (WRAM stub)
```
1. Disable NMI ($4200=$00)
2. CFGR=$80 (IRQ mask)
3. CLSR=$01 (21 MHz)
4. SCBR=$00 (screen base)
5. SCMR=$19 (4bpp + RAN + RON)
6. PBR = :gsu_program (bank byte)
7. R15 = gsu_program (16-bit write triggers GO)
8. Poll SFR bit 5 until 0
9. SCMR=$00 (reclaim buses)
10. Re-enable NMI ($4200=$81)
```

## Files

| File | Status |
|------|--------|
| `compiler/Makefile` | wla-superfx build |
| `templates/hdr_superfx.asm` | ROM header (cart $14) |
| `templates/memmap_gsu.inc` | GSU memory map |
| `templates/gsu_boot.sfx` | Default GSU boot stub |
| `make/common.mk` | USE_SUPERFX, GSUSRC pipeline |
| `lib/include/snes/superfx.h` | Register definitions |
| `lib/source/superfx.c` | gsuInit() |
| `templates/crt0.asm` | .ifdef SUPERFX init |
| `examples/memory/superfx_hello/` | Boot + SRAM diagnostic ✅ |
| `examples/graphics/effects/superfx_bitmap/` | PLOT gradient ✅ |

## Resources

- [x] casfx (ARM9) -- PLOT config, CMODE, tilemap, double buffering
- [x] DOOM-FX (Randy Linden) -- init sequence, bus arbitration, multi-DMA
- [x] snes-sfx-demo (secondsun) -- minimal homebrew, libSFX framework
- [x] PeterLemon/SNES -- **KEY**: complete PLOT 4bpp 256x128 example, RPIX flush
- [x] Expert feedback (2026-03-23) -- delay slot, POR, CACHE, CLSR, MC1 bugs
- [ ] libSFX (Optiroc) -- full SNES/SuperFX framework
- [ ] GSU-Development-Kit (DiscoMan) -- modern dev kit
