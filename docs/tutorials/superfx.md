# SuperFX (GSU) Tutorial {#tutorial_superfx}

This tutorial covers the SuperFX coprocessor -- a 16-bit RISC CPU running at
up to 21.47 MHz inside the cartridge, with hardware pixel rendering.

## What Is SuperFX?

The SuperFX (Graphics Support Unit) is a custom RISC processor found in games
like *Star Fox*, *Yoshi's Island*, and *DOOM*. Unlike the SA-1 (which uses the
same 65816 instruction set), the SuperFX has its own unique ISA with 16-bit
registers, hardware PLOT for pixel rendering, and an instruction cache.

| Feature | Main CPU | SuperFX (GSU) |
|---------|----------|---------------|
| Architecture | 65816 (CISC) | Custom RISC |
| Clock | 3.58 MHz | 10.74 / 21.47 MHz |
| Instruction set | 65816 | Unique (~55 opcodes) |
| C compiler | Yes (cc65816) | **No** (assembly only) |
| WRAM access | Yes (128 KB) | **No** |
| PPU/APU access | Yes | **No** |
| SRAM access | Yes ($70:0000) | Yes (shared) |
| ROM access | Yes | Yes (**exclusive bus**) |
| Special | -- | PLOT (hardware pixel rendering) |

## Emulator Compatibility

> **Important**: SuperFX examples require specific emulators.

| Emulator | Status |
|----------|--------|
| **bsnes** | Recommended -- cycle-accurate SuperFX emulation |
| **Mesen2** | Partial -- boot and registers work, PLOT rendering functional |
| **snes9x** | Does not detect SuperFX in our ROM header |

## Getting Started

### 1. Enable SuperFX in Your Makefile

```makefile
OPENSNES := $(shell cd ../../../.. && pwd)
TARGET   := mygame.sfc
ROM_NAME := MY SUPERFX GAME

CSRC     := main.c
ASMSRC   := gsu_loader.asm
GSUSRC   := gsu_code.sfx

USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background superfx

include $(OPENSNES)/make/common.mk
```

`GSUSRC` lists your SuperFX assembly files (`.sfx`). The build system
assembles them with `wla-superfx` and produces flat binaries (`.sfx.bin`).

### 2. Write GSU Assembly Code

Create `gsu_code.sfx` -- this runs on the SuperFX at up to 21.47 MHz:

```asm
.include "memmap_gsu.inc"

.BANK 0 SLOT 0
.ORG 0

gsu_main:
    ; Select SRAM bank
    IBT R0, #$70
    RAMB

    ; Your computation here...
    IWT R0, #$CAFE      ; result in R0

    ; IMPORTANT: NOP padding before STOP
    NOP
    NOP
    STOP
    NOP                  ; dummy after STOP (required)
```

### 3. Create the GSU Loader (65816 ASM)

Create `gsu_loader.asm` -- this embeds the GSU binary and provides
the WRAM-based launch function:

```asm
.ifdef SUPERFX

; Embed GSU binary
.SECTION ".gsu_code" SUPERFREE
gsu_program:
    .incbin "gsu_code.sfx.bin"
gsu_program_end:
.ENDS

; WRAM area for the launch stub
.RAMSECTION ".gsu_vars" BANK 0 SLOT 1
gsu_wram_area: dsb 64
.ENDS

; Launch function + WRAM stub (same section for label arithmetic)
.SECTION ".gsu_launcher" SEMIFREE
.ACCU 16
.INDEX 16

launchGSU:
    php
    ; Copy stub to WRAM
    sep #$20
    rep #$10
    ldx #$0000
-   lda.l _wram_stub,x
    sta.l gsu_wram_area,x
    inx
    cpx #(_wram_stub_end - _wram_stub)
    bne -
    jsl gsu_wram_area    ; Execute from WRAM
    plp
    rtl

_wram_stub:
    sep #$20
    .ACCU 8
    lda #$00
    sta.l $4200          ; Disable NMI (vector is in ROM!)
    lda #$80
    sta.l $3037          ; CFGR: IRQ mask
    lda #$01
    sta.l $3039          ; CLSR: 21.47 MHz
    lda #$18
    sta.l $303A          ; SCMR: RAN + RON
    lda #:gsu_program
    sta.l $3034          ; PBR
    rep #$20
    .ACCU 16
    lda #gsu_program
    sta.l $301E          ; R15 -> GO!
    sep #$20
    .ACCU 8
-   lda.l $3030
    and #$20             ; Poll SFR GO flag
    bne -
    lda #$00
    sta.l $303A          ; Reclaim buses
    lda #$81
    sta.l $4200          ; Re-enable NMI
    rtl
_wram_stub_end:

.ENDS
.endif
```

### 4. Launch from C

```c
#include <snes.h>
#include <snes/superfx.h>

extern void launchGSU(void);

int main(void) {
    consoleInit();

    if (!gsuInit()) {
        /* No SuperFX hardware */
        while (1) { WaitForVBlank(); }
    }

    launchGSU();

    /* Read result from GSU R0 */
    u16 result = *(volatile u16*)0x3000;

    setScreenOn();
    while (1) { WaitForVBlank(); }
    return 0;
}
```

## The Exclusive Bus

When SCMR has RON=1, the GSU owns the ROM bus. **The SNES CPU cannot
read ROM -- not even its own code.** This is why:

1. The launch/poll code must execute from **WRAM**
2. **NMI must be disabled** (the NMI vector is in ROM)
3. All reference projects (casfx, DOOM-FX, PeterLemon) use WRAM execution

## SuperFX Assembly Rules

Four mandatory rules for all GSU programs:

### 1. NOP After Every Branch (Delay Slot)

The SuperFX pipeline pre-fetches the instruction after a branch.
It **always executes**, whether the branch is taken or not.

```asm
    DEC R4
    BNE _loop
    NOP              ; MANDATORY -- delay slot
```

### 2. NOP Padding Before STOP

STOP in the pipeline halts the GSU before the branch can take effect.

```asm
    NOP              ; delay slot safety
    NOP              ; MC1 store-STOP bug safety
    STOP
    NOP              ; dummy opcode (pipeline clear)
```

### 3. RPIX After PLOT Loops

PLOT writes to an 8-pixel cache. The last pixels stay in cache unless flushed.

```asm
_col:
    PLOT
    DEC R4
    BNE _col
    NOP              ; delay slot
    IBT R1, #$00
    RPIX             ; flush pixel cache to SRAM
```

### 4. CACHE Before Hot Loops

CACHE loads 512 bytes of code. Instructions execute in 1 cycle (vs 3-5 from ROM).

```asm
    CACHE            ; ~6x speedup with 21 MHz clock
_loop:
    PLOT
    DEC R4
    BNE _loop
    NOP
```

## PLOT Bitmap Rendering

The PLOT instruction writes pixels in SNES bitplane format to SRAM.

### Setup

From the SNES CPU (in the WRAM stub):
```asm
stz.l $3038          ; SCBR = $00 (screen base at SRAM $0000)
lda #$19
sta.l $303A          ; SCMR = 4bpp + RAN + RON + height 128
```

From the GSU:
```asm
IBT R0, #$00
CMODE                ; POR = $00 (transparent, no dither)

IBT R0, #$05         ; color index 5
COLOR                ; set plot color

IBT R1, #$00         ; X = 0
IBT R2, #$00         ; Y = 0
PLOT                 ; writes pixel at (R1, R2), R1 auto-increments
```

### Column-Major Tile Layout

PLOT stores tiles **column by column**, not row by row:

```
SRAM tile 0  = screen position (col=0, row=0)
SRAM tile 1  = screen position (col=0, row=1)   <- NOT (1,0)!
SRAM tile 16 = screen position (col=1, row=0)
```

The SNES PPU tilemap must compensate:
```
tilemap[row * 32 + col] = col * tiles_per_column + row
```

For 4bpp height=128: `tilemap[row*32 + col] = col*16 + row`

### SRAM to VRAM Transfer

After the GSU finishes, DMA the framebuffer during forced blank:

```c
setScreenOff();
/* DMA 16KB from SRAM $70:0000 to VRAM $0000 */
/* (see gsu_loader.asm dmaBitmapToVRAM function) */
setScreenOn();
```

## Examples

| Example | What it demonstrates |
|---------|---------------------|
| [superfx_hello](../../../examples/memory/superfx_hello/) | Boot, registers, SRAM read/write |
| [superfx_3d](../../../examples/memory/superfx_3d/) | Rotating wireframe cube, Bresenham line drawing, 3D projection |

## Further Reading

- [SuperFX register reference](../../.claude/SUPERFX.md) -- complete register documentation
- [superfx.h API](../../lib/include/snes/superfx.h) -- register macros
- [PeterLemon/SNES](https://github.com/PeterLemon/SNES) -- reference PLOT examples
- [casfx](https://github.com/ARM9/casfx) -- complete SuperFX demo
