---
name: port-example
description: Port a PVSnesLib example to OpenSNES SDK, preserving identical visual output
argument-hint: "<pvsneslib_path> (e.g., graphics/Effects/Transparency)"
allowed-tools: Bash(*), Read, Write, Edit, Glob, Grep
---

# /port-example - Port PVSnesLib Example to OpenSNES

Port a PVSnesLib example to OpenSNES SDK, preserving identical visual output.

## Usage
```
/port-example <pvsneslib_path>                    # Port from PVSnesLib path
/port-example graphics/Effects/Transparency       # Relative to pvsneslib/snes-examples/
```

## PVSnesLib Source Location
```
$PVSNESLIB_HOME/snes-examples/
```
Set `PVSNESLIB_HOME` to your local PVSnesLib clone (e.g. `export PVSNESLIB_HOME=~/workspace/pvsneslib`).

## Step-by-Step Porting Protocol

### Phase 1 — Analyze PVSnesLib Source

Read ALL source files from the PVSnesLib example:
- `*.c` — Main source code
- `data.asm` — Asset includes
- `hdr.asm` — ROM header (ignore, OpenSNES has its own)
- `Makefile` — **CRITICAL: extract exact gfx4snes/gfxconv flags**
- `*.bmp` / `*.png` — Source assets
- `*.it` / `*.brr` — Audio assets

Identify:
1. Which BG mode (Mode 0/1/3/7)?
2. How many BG layers and their bit depths (2bpp/4bpp/8bpp)?
3. Which VRAM layout (tile base, tilemap addresses)?
4. Palette banks used (via `-e` flag in gfxconv)?
5. Any HDMA, color math, windowing, or Mode 7?
6. Audio (SNESMOD)?
7. Input handling?

### Phase 2 — Asset Conversion

#### Rule 1: Prefer BMP directly — gfx4snes supports BMP input

Our gfx4snes supports BMP natively via `-t bmp`. This is the SAFEST approach
because it eliminates any palette conversion issues.

```makefile
res/background.pic res/background.pal res/background.map: res/background.bmp
	@$(GFX4SNES) -a -s 8 -u 16 -e 1 -p -m -t bmp -i $<
```

Copy BMPs directly from PVSnesLib to `res/`:
```bash
cp $PVSNESLIB_HOME/snes-examples/<path>/*.bmp res/
```

#### Rule 2: If PNG conversion is needed, use PIL (NEVER ImageMagick)

ImageMagick's `convert` alters indexed palettes. PIL preserves them exactly.

```python
from PIL import Image
img = Image.open('source.bmp')
img.save('res/output.png')  # Preserves indexed palette (mode P)
```

**CRITICAL: Do NOT use PIL's `quantize()`** on the converted image. If the image
has more colors than the target (e.g., 19 colors but `-u 16`), let gfx4snes
handle the reduction with `-a` (palette rearrangement). PIL's quantizer creates
an entirely different palette that produces wrong colors.

#### Rule 3: Copy gfx4snes flags EXACTLY from PVSnesLib Makefile

PVSnesLib uses `$(GFXCONV)`, we use `$(GFX4SNES)`. Same tool, same flags.
Only change: replace `-t bmp` with `-t png` if you converted to PNG (or keep BMP).

Common flag patterns:
```
# 4bpp BG (16 colors), with tilemap, palette entry 1
-a -s 8 -u 16 -e 1 -p -m -t bmp

# 2bpp BG3 Mode 1 (4 colors), with tilemap
-s 8 -o 2 -u 4 -p -m -t bmp

# 8bpp Mode 3 (256 colors), with tilemap
-s 8 -o 256 -u 256 -p -m -t bmp

# Mode 7
-p -m -M 7

# Sprites 16x16, 4bpp
-s 16 -o 16 -u 16 -p -t bmp

# Sprites with transpose for OBJ VRAM layout
-s 32 -o 16 -u 16 -p -T -X 64 -Y 64 -P 2

# 2bpp pre-processed (e.g., superscope)
-s 8 -m -p -u 4 -o 4
```

#### Rule 4: Palette rearrangement with `-a` may output multi-bank palettes

When `-a` is used, gfx4snes may split colors across multiple palette banks.
The `.pal` file will contain ALL banks (e.g., 128 colors = 8 banks × 16).
The assembly DMA loader MUST load the full palette file, not just 16 colors.
The `-e N` offset ensures tilemap entries reference the correct banks.

### Phase 3 — Assembly Data File (data.asm)

Use the **assembly DMA loader pattern** for all asset loading. This handles
SUPERFREE bank bytes correctly (C's dmaCopyVram hardcodes bank $00).

```asm
;--- Data sections (SUPERFREE = linker places optimally) ---
.section ".rodata1" superfree
tiles:
.incbin "res/background.pic"
tiles_end:
tilemap:
.incbin "res/background.map"
tilemap_end:
palette:
.incbin "res/background.pal"
palette_end:
.ends

;--- Assembly DMA loader (handles bank bytes) ---
.section ".loader" superfree
loadGraphics:
    php

    ; Set VMAIN for word increment
    sep #$20
    lda #$80
    sta.l $2115

    ; DMA tiles to VRAM
    rep #$20
    lda #<vram_addr>
    sta.l $2116
    lda #(tiles_end - tiles)
    sta.l $4305
    lda #tiles
    sta.l $4302
    sep #$20
    lda #:tiles             ; bank byte from LINKER — the key!
    sta.l $4304
    lda #$01
    sta.l $4300             ; mode: word write
    lda #$18
    sta.l $4301             ; dest: VMDATAL ($2118)
    lda #$01
    sta.l $420B             ; start DMA ch0

    ; DMA palette to CGRAM
    sep #$20
    lda #<start_color>
    sta.l $2121             ; CGADD
    rep #$20
    lda #(palette_end - palette)
    sta.l $4305
    lda #palette
    sta.l $4302
    sep #$20
    lda #:palette
    sta.l $4304
    lda #$00
    sta.l $4300             ; mode: byte write
    lda #$22
    sta.l $4301             ; dest: CGDATA ($2122)
    lda #$01
    sta.l $420B

    ; DMA tilemap to VRAM (same pattern as tiles)
    ; ...

    plp
    rtl
.ends
```

### Phase 4 — C Main File (main.c)

#### PVSnesLib → OpenSNES API Mapping

| PVSnesLib | OpenSNES | Notes |
|-----------|----------|-------|
| `consoleInit()` | `consoleInit()` | Same |
| `setMode(BG_MODE1, 0)` | `setMode(BG_MODE1, 0)` | Same |
| `setScreenOn()` | `setScreenOn()` | Same |
| `WaitForVBlank()` | `WaitForVBlank()` | Same |
| `bgInitTileSet(bg, ...)` | Assembly DMA loader | See Phase 3 |
| `bgInitMapSet(bg, ...)` | Assembly DMA loader | See Phase 3 |
| `bgSetScroll(bg, x, y)` | `bgSetScroll(bg, x, y)` | Same (bg 0-indexed) |
| `bgSetDisable(bg)` | `setMainScreen(LAYER_BG1 \| ...)` | Use bitmask |
| `bgSetEnableSub(bg)` | `setSubScreen(LAYER_BGx)` | Macro in video.h |
| `setColorEffect(a, b)` | `colorMathInit()` + library calls | See below |
| `padsCurrent(0)` | `padHeld(0)` | Held buttons |
| `padsDown(0)` | `padPressed(0)` | New presses |
| `oamSet(...)` | Direct `oamMemory[]` writes | oamSet has framesize=158! |
| `oamSetEx(...)` | Direct `oamMemory[512+]` writes | High table |
| `hdmaSetup(ch, ...)` | `hdmaSetup(ch, ...)` | Same |
| `hdmaEnable(ch)` | `hdmaEnable(1 << ch)` | **BITMASK not channel number!** |

#### Color Math Mapping

PVSnesLib's `setColorEffect(CM_SUBBGOBJ_ENABLE, CM_MSCR_BACK | CM_MSCR_BG1)` becomes:
```c
colorMathInit();
colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
colorMathSetOp(COLORMATH_ADD);
colorMathEnable(COLORMATH_BG1 | COLORMATH_BACKDROP);
```

#### BG Register Setup (when not using bgInitTileSet)

```c
/* BG tilemap address: shift word addr right by 8, OR with size */
REG_BG1SC = (0x2000 >> 8) | 0x00;  /* SC_32x32 at VRAM $2000 */

/* Tile base: low nibble = BG1, high nibble = BG2 */
REG_BG12NBA = 0x01;  /* BG1 at $1000, BG2 at $0000 */
REG_BG34NBA = 0x10;  /* BG3 at $0000, BG4 at $1000 */
```

#### Module Dependencies (LIB_MODULES)

```
console   — ALWAYS required (NMI handler)
sprite    — ALWAYS with console (oamUpdate in NMI)
dma       — ALWAYS with sprite (OAM DMA)
background — if using bgSetScroll or BG library functions
input     — if using padPressed/padHeld
colormath — if using color math effects
hdma      — if using HDMA effects
text      — if displaying text (also needs dma)
text4bpp  — if using 4bpp text (Mode 1/3)
snesmod   — if using audio (also needs console)
```

Minimum viable: `console sprite dma`
Typical game: `console sprite dma input background`

### Phase 5 — Makefile

```makefile
OPENSNES := $(shell cd ../../../.. && pwd)  # Adjust depth to reach SDK root

TARGET     := example_name.sfc
ROM_NAME   := EXAMPLE NAME HERE     # 21 chars max

USE_LIB    := 1
LIB_MODULES := console sprite dma input background

CSRC       := main.c
ASMSRC     := data.asm

# Optional: SNESMOD audio
# USE_SNESMOD    := 1
# SOUNDBANK_SRC  := res/music.it res/sfx.it

include $(OPENSNES)/make/common.mk

# Graphics rules (copy flags from PVSnesLib Makefile)
res/bg.pic res/bg.pal res/bg.map: res/bg.bmp
	@echo "[GFX] Converting background..."
	@$(GFX4SNES) <exact PVSnesLib flags> -i $<

combined.asm: res/bg.pic

clean: clean-example
.PHONY: clean-example
clean-example:
	@rm -f res/*.pic res/*.pal res/*.map res/*.inc res/*_data.as
```

### Phase 6 — Build and Verify

1. Full rebuild: `make clean && make`
2. Compiler tests: `./tests/compiler/run_tests.sh --allow-known-bugs`
3. Example validation: `OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick`
4. **STOP** — Ask user to validate in Mesen2 (Category C protocol)
5. Compare side-by-side with PVSnesLib ROM if possible

## Critical Pitfalls (from hard experience)

### 1. hdmaEnable takes a BITMASK, not a channel number
```c
hdmaEnable(1 << 3);   // CORRECT: enable channel 3
hdmaEnable(3);         // WRONG: enables channels 0+1 !
```

### 2. oamSet() is catastrophically slow
`oamSet()` has framesize=158. More than 2-3 calls per frame = jerky movement.
Write directly to `oamMemory[]` instead:
```c
extern u8 oamMemory[];
extern volatile u8 oam_update_flag;
oamMemory[id*4 + 0] = (u8)x;
oamMemory[id*4 + 1] = (u8)y;
oamMemory[id*4 + 2] = tile;
oamMemory[id*4 + 3] = attr;  // vhoopppc
oam_update_flag = 1;
```

### 3. cc65816 pushes args LEFT-TO-RIGHT (not like PVSnesLib's tcc816)
PVSnesLib ASM functions ported verbatim have SWAPPED stack offsets.
`f(a, b)` → our compiler: b at lower SP offset, a at higher.

### 4. HDMA bank byte bug
`hdmaSetup()` hardcodes bank $00 for ROM addresses. SUPERFREE const data
may be in bank $01+. Use non-const tables (RAM = bank $7E = $00 mirror)
or `hdmaSetupBank()` with explicit bank.

### 5. Tilemap padding for 256×224 images
gfx4snes generates 32×28 tilemap (1792 bytes). SC_32x32 needs 2048 bytes.
Unwritten rows 28-31 show VRAM garbage. Pad .map to 2048 bytes or use
256×256 source images.

### 6. VBlank DMA budget is ~5KB
NMI overhead (~8600 cycles) leaves ~41K cycles. DMA = 8 cycles/byte.
For large transfers, use forced blank (`REG_INIDISP = 0x80`) before DMA.

### 7. `unsigned int` is 4 bytes, `unsigned long` is 8 bytes
NOT the x86 convention. Use `u16` / `u32` explicitly.

### 8. Never use PIL quantize() on BMP→PNG conversion
PIL's quantizer creates an entirely different palette. Either:
- Use BMP directly (`-t bmp`)
- Or save PNG preserving the original indexed palette (`img.save()` without quantize)

### 9. padUpdate() is a NO-OP
NMI handler reads joypads directly. Don't call padUpdate().
Use `padPressed(0)` for new presses, `padHeld(0)` for held buttons.

### 10. SUPERFREE bank $00 overflow
Each `static const` array gets its own SUPERFREE section. If bank $00
($8000-$FFFF = 32KB) fills up, data spills to bank $01+ and
`lda.l $0000,x` reads garbage. Combine related const arrays.

## Audio Porting (SNESMOD)

Copy `.it` files from PVSnesLib example to `res/`.
```makefile
USE_SNESMOD    := 1
SOUNDBANK_SRC  := res/music.it res/sfx.it
```

PVSnesLib audio API maps 1:1:
- `spcLoad(0)` → `spcLoad(0)`
- `spcPlay(0)` → `spcPlay(0)`
- `spcPlaySound(sfx)` → `spcPlaySound(sfx)`
- `spcProcess()` → `spcProcess()`

## Checklist Before Requesting User Validation

- [ ] All source assets (.bmp/.png/.it/.brr) in `res/`
- [ ] gfx4snes flags match PVSnesLib exactly
- [ ] Assembly DMA loader with `:label` bank bytes
- [ ] Palette loaded at correct CGRAM offset (check `-e` flag)
- [ ] `hdmaEnable(1 << ch)` not `hdmaEnable(ch)`
- [ ] No `oamSet()` in hot loops (use `oamMemory[]`)
- [ ] `make clean && make` passes
- [ ] `validate_examples.sh --quick` passes
- [ ] Side-by-side screenshot comparison prepared
