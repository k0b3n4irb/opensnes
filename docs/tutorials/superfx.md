# Super FX: a First Job {#tutorial_superfx}

This page takes a Super FX program from the Makefile to a result on screen:
the build knobs, writing GSU code, launching a job from C and reading what
it wrote. Running the game while the GSU works, interrupts during a job,
several entry points, double-buffered presentation and saving are
@ref tutorial_superfx_advanced.

This tutorial covers the Super FX coprocessor -- a 16-bit RISC CPU running at
up to 21.47 MHz inside the cartridge, with hardware pixel rendering.

## What Is Super FX?

The Super FX (Graphics Support Unit) is a custom RISC processor found in games
like *Star Fox*, *Yoshi's Island*, and *DOOM*. Unlike the SA-1 (which uses the
same 65816 instruction set), the Super FX has its own unique ISA with 16-bit
registers, hardware PLOT for pixel rendering, and an instruction cache.

| Feature | Main CPU | Super FX (GSU) |
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

## Getting Started

### 1. Enable Super FX in Your Makefile

On top of the template of @ref tools_build, add:

```makefile
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_code.sfx
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background superfx
```

`GSUSRC` lists your Super FX assembly files (`.sfx`). The build system
assembles them with `wla-superfx` and produces flat binaries (`.sfx.bin`).

Two knobs matter for a GSU project: `ROM_BANKS := 32`
gives a 1 MB ROM (Star Fox's size; the default 8 is 256 KB) — the header's
size byte and the asset bank range follow — and `GSU_RAM_KB := 64` is the
Game Pak RAM the cartridge declares in the extended header at `$FFBD`
(fullsnes, snesdev-wiki: Super FX carts declare RAM there and leave `$FFD8`
at zero; the header's licensee byte `$FFDA` is `$33` so the extended header
is recognised). A GSU-side variable is a `.RAMSECTION "name" BANK $70
SLOT 3` on the 65816 side.

### 2. Write GSU Assembly Code

Create `gsu_code.sfx` -- this runs on the Super FX at up to 21.47 MHz:

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

Create `gsu_loader.asm`: it only embeds the GSU binary. The launch is the
library's.

```asm
.ifdef SUPERFX
; The GSU binary. Placed where it fits, in any ROM bank (the library reads
; the bank from the pointer) — or at $n:8000 when the Makefile sets GSU_BANK
GSU_SECTION ".gsu_code"
gsu_program:
    .incbin "gsu_code.sfx.bin"
gsu_program_end:
.ENDS
.endif
```

### 4. Launch from C

```c
#include <snes.h>
#include <snes/superfx.h>

extern const u8 gsu_program[];   /* the label before the .incbin */

int main(void) {
    consoleInit();

    if (!gsuInit()) {            /* sets gsu_cfgr / gsu_scmr / gsu_scbr defaults */
        /* No SuperFX hardware */
        while (1) { WaitForVBlank(); }
    }

    gsuSetProgram(gsu_program);  /* once: where the GSU program lives */
    gsuLaunch();                 /* runs the job from WRAM, returns at STOP */

    /* Read result from GSU R0 */
    u16 result = *(volatile u16*)0x3000;

    setScreenOn();
    while (1) { WaitForVBlank(); }
    return 0;
}
```

`gsuLaunch()` copies nothing and disables nothing: the CPU waits in a WRAM
loop while the GSU owns the cartridge, and interrupts keep working
(@ref tutorial_superfx_advanced explains how). Adjust `gsu_cfgr` (IRQ mask, fast multiply) and `gsu_scmr`
(colour depth, bus grants) before the call when your program needs it —
`examples/chips/superfx_hello` sets both.

## Super FX Assembly Rules

Four mandatory rules for all GSU programs:

### 1. NOP After Every Branch (Delay Slot)

The Super FX pipeline pre-fetches the instruction after a branch.
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

The library writes that tilemap for you: `gsuSetupBitmapTilemap(vram)`
fills a 32×32 tilemap at that VRAM word address in that column-major
order, `tiles_per_column` taken from `gsu_scmr`'s height (16, 20 or 24 for
128, 160 or 192 lines; fullsnes, SCMR), and tile 0 below it. Set
`gsu_scmr` before calling it.

### SRAM to VRAM Transfer

A 4bpp 256×128 framebuffer is 16 KB, far more than a VBlank carries. The
library's answer, used by `examples/chips/superfx_3d`, is to cut the
visible picture down and send the frame during the lines that are dark:

```c
gsuSetupBitmapTilemap(0x4000);   /* once, in force blank */
/* ... each frame, once the GSU job is done: */
gsuDmaFullFrame();               /* waits for the bottom band, then DMAs 16 KB */
/* ... once, after the first frame: */
gsuSetupHdmaBlanking(40, 40);    /* 40 black lines at the top and bottom */
```

`gsuDmaFullFrame()` copies 16 KB from Game Pak RAM (`$70`, offset
`gsu_dma_src_hi` × 256) to VRAM `$0000` on DMA channel 0. It starts only
on a line from which the whole frame lands before the display comes back:
from the first line of the bottom band (225 - bottom) to 152 + top, lines
185 to 192 with 40 + 40; called later, it waits for the next frame's
window. `gsuSetupHdmaBlanking(top, bottom)` uses HDMA channel 1 to force
blank that many lines at the top and the bottom of the screen, the bars
that give the transfer its time; it returns once they are on screen (HDMA
starts a channel at line 0, so the bands appear a frame later). The two
bands must add up to 73 lines or more for a 16 KB frame.

The wait reads STAT78 (`$213F`), the only reset of OPVCT's read-twice
flip-flop (snesdev-wiki), before each read of OPVCT (`$213D`). Code of your
own that waits on the line counter must do the same: without it, every
other read starts on the high byte, whose bits 1-7 are PPU2 open bus
(anomie, fullsnes). Read `$213F`, `$2137`, then `$213D` twice, every time.
`superfx_3d` presents about 30 frames per second, all of them whole.

It uses HDMA channel 1 and arms it like `hdmaEnableMask()` does, so it
combines with your own HDMA channels; just leave channel 1 to it.

`gsuIsPresent()` returns 1 when crt0 found a GSU at boot; use it to fall
back gracefully, as `gsuInit()` does, without `gsuInit()`'s side
effects.

## Going further

@ref tutorial_superfx_advanced covers what a game needs past a first job:
the exclusive bus and the code cache that lets the game run during a job,
the IRQ on STOP, entry points and `GSU_BANK`, the double buffer the NMI
presents, and saving to the battery-backed Game Pak RAM.

## Example ROMs

| Example | What it demonstrates |
|---------|---------------------|
| [superfx_hello](../../examples/chips/superfx_hello/) | Boot, registers, SRAM read/write |
| [superfx_3d](../../examples/chips/superfx_3d/) | Rotating wireframe cube, Bresenham line drawing, 3D projection |
| [superfx_game_skeleton](../../examples/chips/superfx_game_skeleton/) | A game loop at 60 fps while the GSU renders from its cache; double-buffered presentation |
| [superfx_save](../../examples/chips/superfx_save/) | A boot counter kept in the battery-backed Game Pak RAM, through the `sram` module |

## Further Reading

- [Super FX register reference](../hardware/REGISTERS.md) -- register tables and programming details
- [superfx.h API](../../lib/include/snes/superfx.h) -- register macros
- [PeterLemon/SNES](https://github.com/PeterLemon/SNES) -- reference PLOT examples
- [casfx](https://github.com/ARM9/casfx) -- complete Super FX demo
