# SuperFX (GSU) -- Status and Knowledge Base

## Summary

SuperFX support in active development. Phase 0 (build system) and Phase 1 (boot + registers) work.
Phase 2 (bitmap rendering) partially working: STW pipeline validated on bsnes, PLOT has position/color issues.

## Emulator Compatibility

| Emulator | SuperFX Support | Notes |
|----------|----------------|-------|
| **bsnes** | Reference emulator | Cycle-accurate SuperFX emulation. Use for validation. |
| **Mesen2** | Partial | Works for boot/registers. **Backward branch bug CONFIRMED on 2026-03-22 via bsnes comparison** -- GSU programs with backward branches (loops) may not execute correctly. |
| **snes9x** | Not supported | Does not detect SuperFX in our ROM header. opensnes-emu screenshots show "GSU: NOT DETECTED" or black screen. |

## What Works

### Phase 0 -- Build system (commit `28b42ff`)
- `wla-superfx` compiled and installed in `bin/`
- 2-step pipeline: `.sfx` -> `wla-superfx` -> `wlalink -b` -> `.sfx.bin` -> `.incbin`
- `USE_SUPERFX=1` in Makefile, `GSUSRC` for GSU sources
- `hdr_superfx.asm` (cart type $14, LoROM, SRAM 32K)
- `memmap_gsu.inc` (GSU address space)
- `lib/Makefile` build variant `superfx`

### Phase 1 -- Boot + GSU registers (commit `4af0f3b`)
- `superfx_hello`: GSU executes `IWT R0, #$CAFE; STOP`
- SNES CPU reads R0 = $CAFE after STOP -> **bidirectional communication via registers $3000-$303F confirmed**
- VCR ($303B) = $04 (GSU chip version)
- `superfx.h`: complete register definitions
- `crt0.asm`: SUPERFX init (SCMR=$00, BRAMR=$01, VCR read)
- `gsuInit()` returns 1 if GSU detected

### Phase 2 -- Bitmap rendering (partial, commits `dfa25cf`, `cef71c7`)

**STW pipeline**: Works on bsnes. GSU writes to SRAM via STW, SNES CPU DMAs to VRAM.
- `superfx_hello` STW test: writes $BEEF to SRAM[2,3], readback confirmed
- `superfx_bitmap` clear loop: fills 16KB SRAM with STW, DMA to VRAM displays correctly
- Identity tilemap at VRAM $4000, tiles 0-511
- Remaining tilemap entries filled to avoid garbage on bottom half (commit `cef71c7`)

**PLOT rendering**: Partially working, position and color issues remain.

## PLOT Debugging Findings (2026-03-22)

### Observed behavior on bsnes
- PLOT writes pixels, but they appear at wrong position
- Pink/magenta line visible at approximately Y~127 instead of expected Y=0
- Color mismatch: COLOR #$01 (should be red from palette index 1) renders as pink/magenta
- STW-based clear (writing $0000) works correctly -- black background confirmed

### Possible causes
1. SCBR (Screen Base Register) offset calculation: PLOT may use a different SRAM base than expected
2. PLOT pixel coordinate mapping: Y coordinate may wrap or offset due to screen height config
3. Color register (COLR) vs palette: PLOT uses the COLR register, which maps to bitplane bits differently than direct palette index
4. Screen height (HT bits in SCMR): 128px height mode may affect coordinate mapping

### Next steps
1. Study casfx PLOT examples for correct SCBR/SCMR configuration
2. Verify PLOT coordinate system with single-pixel tests at known positions
3. Compare COLR-to-bitplane mapping with SNES 4bpp format documentation

## Technical Architecture

### GSU (Graphics Support Unit)
- 16-bit RISC CPU, 10.74 / 21.47 MHz
- 16 registers 16-bit (R0-R15), R15 = PC
- Unique ISA (~55 mnemonics), NOT compatible with 65816
- No hardware stack (LINK/RET via R11)
- PLOT instruction: writes pixels in SNES bitplane format
- **No C compiler** -- GSU code = assembly only

### Memory
```
+---------------------------------+
|         ROM (shared)            |
|  EXCLUSIVE bus: only one CPU    |
|  at a time (SCMR RON bit)      |
+----------+---------------------+
           |
    +------+------+
    |      |      |
+---+---+ ++----+ ++------+
|SNES   | |SRAM | |GSU    |
|3.58MHz| |32-  | |10.74  |
|       | |128KB| |MHz    |
|WRAM   | |$70: | |       |
|PPU    | |0000 | |NO WRAM|
|APU    | |     | |NO PPU |
+-------+ +-----+ +-------+
```

### GSU Registers (SNES CPU side, $3000-$303F)

| Register | Address | Role |
|----------|---------|------|
| R0-R15 | $3000-$301F | General 16-bit registers. **Writing R15H ($301F) starts the GSU** |
| SFR | $3030-$3031 | Status/Flags. Bit 5 = GO (1=running). Bit 15 = IRQ |
| BRAMR | $3033 | Backup RAM register (bit 0: 1=SRAM writable by SNES CPU) |
| PBR | $3034 | Program Bank Register (ROM bank for GSU code) |
| SCBR | $3038 | Screen Base Register (PLOT framebuffer base in SRAM) |
| CLSR | $3039 | Clock Select (0=10.74MHz, 1=21.47MHz) |
| SCMR | $303A | Screen Mode: color + height + **bus ownership (RAN/RON)** |
| VCR | $303B | Version Code Register (read-only, $04 on our hardware) |
| RAMBR | $303C | RAM Bank Register (read-only on SNES CPU side) |

### SCMR ($303A) -- Critical Register

```
Bit 7-6: unused
Bit 5:   HT1 (height bit 1)
Bit 4:   RON (1 = GSU owns ROM, SNES CPU CANNOT read ROM!)
Bit 3:   RAN (1 = GSU owns RAM, SNES CPU CANNOT read SRAM!)
Bit 2:   HT0 (height bit 0)
Bit 1-0: MD (00=2bpp, 01=4bpp, 11=8bpp)

Height: HT1:HT0 = 00->128px, 01->160px, 10->192px, 11->OBJ
```

### Exclusive Bus -- MAJOR CONSTRAINT

When SCMR has RON=1, the SNES CPU CANNOT read ROM -- not even its own code!
SFR polling must execute from **WRAM** (stub copied to $00:0xxx).

Current sequence (in gsu_loader.asm):
1. SNES CPU writes R0 (parameter) to $3000
2. Copies launch stub to WRAM
3. JSL to WRAM
4. [WRAM] Configure SCBR, SCMR (RAN+RON), PBR, R15 -> GSU starts
5. [WRAM] Poll SFR bit 5 (GO) until 0
6. [WRAM] SCMR = $00 (bus returns to SNES CPU)
7. RTL back to ROM

### Display Pipeline (validated)

```
GSU PLOT/STW -> SRAM $70:0000 -> DMA ch0 -> VRAM $0000 -> Identity tilemap -> Screen
                                   |                          |
                           Source bank $70            VRAM $4000 (32x16 tiles)
                           Mode $01 (2-reg)           tile[i] = i
                           Dest $18 (VMDATAL)
```

## Files Created

| File | Status |
|------|--------|
| `compiler/Makefile` | Ajout `wla-superfx` |
| `templates/hdr_superfx.asm` | Header ROM (cart $14) |
| `templates/memmap_gsu.inc` | Memory map GSU |
| `templates/gsu_boot.sfx` | Boot stub par defaut |
| `make/common.mk` | USE_SUPERFX, GSUSRC, pipeline |
| `lib/Makefile` | Build variant superfx |
| `lib/include/snes/superfx.h` | Registers + API |
| `lib/source/superfx.c` | gsuInit() |
| `templates/crt0.asm` | .ifdef SUPERFX init |
| `examples/memory/superfx_hello/` | Boot diagnostic -- working |
| `examples/graphics/effects/superfx_bitmap/` | STW pipeline works, PLOT WIP |

## Resources

- [x] casfx (GitHub) -- consulted for PLOT/CMODE/COLOR sequence, SCBR config
- [x] DOOM-FX (GitHub) -- consulted for GSU init sequence and bus arbitration
- [x] snes-sfx-demo (GitHub) -- consulted for minimal SuperFX homebrew example
- [ ] Mesen2 source code for SuperFX (exact register behavior)
- [ ] SnesLab wiki SuperFX: https://sneslab.net/wiki/Super_FX
- [ ] jsgroth blog SuperFX: https://jsgroth.dev/blog/posts/snes-coprocessors-part-7/
- [ ] Super FX Development Kit (SMW Central): https://www.smwcentral.net/?p=viewthread&t=81692
- [ ] sfxasm (alternative assembler): https://github.com/MrGlockenspiel/sfxasm
- [ ] DiscoC (experimental C compiler for GSU): https://github.com/DiscoManOfficial/DiscoC

## Lessons Learned

**Do not guess.** Find working code, compare step by step, validate each stage.
The SA-1 approach (same ISA, Mesen2 debugger verification) does not apply to SuperFX
because it is a RISC CPU with an exclusive bus and a different memory model.

**Use bsnes as reference emulator.** Mesen2 has a confirmed backward branch bug
for GSU code. Always validate SuperFX behavior on bsnes first, then cross-check
with Mesen2 for register-level debugging.

**snes9x does not support SuperFX** in our ROM configuration. opensnes-emu
(snes9x-based) cannot be used for SuperFX testing -- screenshots will show
"GSU: NOT DETECTED" or a black screen.
