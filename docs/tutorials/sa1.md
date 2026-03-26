# SA-1 Enhancement Chip Tutorial {#tutorial_sa1}

This tutorial covers the SA-1 coprocessor — a second 65816 CPU running at 10.74 MHz
inside the cartridge, giving your SNES game 3× the processing power.

## What Is SA-1?

The SA-1 is a **65c816 clone** clocked at 10.74 MHz (vs 3.58 MHz for the main CPU).
It shares the same instruction set, so you already know how to program it. Games like
*Kirby Super Star*, *Super Mario RPG*, and *Kirby's Dream Land 3* used it to handle
AI, physics, and decompression that the main CPU couldn't keep up with.

| Feature | Main CPU | SA-1 |
|---------|----------|------|
| Clock | 3.58 MHz | 10.74 MHz |
| Instruction set | 65c816 | 65c816 (identical) |
| WRAM access | Yes (128 KB) | **No** |
| PPU access | Yes | **No** |
| APU access | Yes | **No** |
| I-RAM access | Yes ($3000-$37FF) | Yes ($0000-$07FF, mirrors $3000) |
| ROM access | Yes | Yes (separate bus, no conflicts) |
| BW-RAM access | Yes (via $6000 window) | Yes (256 KB max) |

The key constraint: **SA-1 cannot touch WRAM, PPU, or APU**. It can only
read ROM and read/write I-RAM and BW-RAM. All communication with the main CPU
happens through the 2 KB shared I-RAM.

## Memory Architecture

```
┌─────────────────────────────────────────────────────┐
│                    ROM (up to 8 MB)                  │
│              Both CPUs read simultaneously           │
└──────────────────────┬──────────────────────────────┘
                       │
       ┌───────────────┼───────────────┐
       │               │               │
 ┌─────┴─────┐   ┌─────┴─────┐   ┌────┴─────┐
 │ Main CPU  │   │  I-RAM    │   │  SA-1    │
 │ 3.58 MHz  │   │  2 KB     │   │ 10.74MHz │
 │           │   │ shared    │   │          │
 │ WRAM 128K │   │ $3000-    │   │ BW-RAM   │
 │ PPU / APU │   │ $37FF     │   │ 256 KB   │
 └───────────┘   └───────────┘   └──────────┘
```

### I-RAM — The Mailbox

I-RAM is 2 KB of fast SRAM at **$3000-$37FF** (both CPUs see this address).
The SA-1 also sees it at **$0000-$07FF** (its direct page region).

This is how the two CPUs communicate: the main CPU writes data to I-RAM,
the SA-1 reads it, computes results, writes them back, and signals "done."

> **Write protection gotcha**: Both CPUs have independent write protection
> registers (SIWP at $2229, CIWP at $222A). **Bit = 1 means WRITABLE**
> (despite the register name). The default is $00 = all protected!
> You must write $FF to enable writes.

## Getting Started

### 1. Enable SA-1 in Your Makefile

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := mygame.sfc
ROM_NAME := MY SA1 GAME

CSRC     := main.c
USE_LIB  := 1
USE_SA1  := 1
LIB_MODULES := console sprite dma background input sa1

include $(OPENSNES)/make/common.mk
```

`USE_SA1 := 1` does three things:
- Selects the SA-1 ROM header (cartridge type $35)
- Links the SA-1 library variant
- Includes your SA-1 boot stub in the ROM

### 2. Write Your SA-1 Boot Code

Create `sa1_boot.asm` **in your example directory** (not in templates/).
This code runs on the SA-1 at 10.74 MHz:

```asm
.ifdef SA1

.SECTION ".sa1_boot" SUPERFREE

.ACCU 16
.INDEX 16

SA1Start:
    ; Standard 65816 init
    sei
    clc
    xce
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #$37FF
    tcs                     ; Stack at top of I-RAM
    lda #$3000
    tcd                     ; Direct page in I-RAM
    sep #$20
    .ACCU 8

    ; Enable SA-1 I-RAM writes (CRITICAL!)
    lda #$FF
    sta.l $00222A           ; CIWP = $FF (bit=1 = WRITABLE)

    ; Signal ready to main CPU
    lda #$A5
    sta.l $3000             ; Magic byte

    ; === Your SA-1 code here ===
    ; Example: idle loop (replace with your compute loop)
-   wai
    bra -

.ENDS

.endif
```

> **Why `sta.l` everywhere?** The SA-1's data bank register (DB) is undefined
> at boot. Using `sta.l` (long absolute, opcode $8F) bypasses DB entirely.
> This is the safest pattern for SA-1 code.

### 3. Initialize from C

```c
#include <snes.h>
#include <snes/sa1.h>

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);

    if (sa1Init()) {
        // SA-1 is running!
    } else {
        // SA-1 failed to boot (wrong emulator or cartridge)
    }

    setScreenOn();
    while (1) { WaitForVBlank(); }
    return 0;
}
```

`sa1Init()` returns 1 if the SA-1 wrote the $A5 magic byte to I-RAM $3000
within the timeout. The boot sequence (in `crt0.asm`) handles all the
register setup: reset vector, I-RAM write protection, and SA-1 release.

## Communication Patterns

### Pattern 1: Flag Sync (Producer/Consumer)

The SA-1 computes data, sets a flag. The main CPU reads the data, clears the flag.
Used by the **sa1_starfield** example.

```
┌──────────┐     I-RAM      ┌──────────┐
│ SA-1     │  ──────────▸  │ Main CPU │
│          │  $3001 = 1    │          │
│ Compute  │  (data ready) │ Read buf │
│ Write buf│               │ Clear $3001│
│ Set flag │  ◂──────────  │          │
│ Wait...  │  $3001 = 0    │ Display  │
└──────────┘  (consumed)   └──────────┘
```

**SA-1 side** (assembly):
```asm
_main:
    ; Wait for main CPU to clear sync flag
-   lda.l $3001
    bne -

    ; Compute and write results to I-RAM buffer
    ; ... your compute code ...

    ; Signal ready
    lda #$01
    sta.l $3001
    jmp _main
```

**Main CPU side** (C):
```c
#define SA1_SYNC (*(volatile u8*)0x3001)
#define SA1_BUF  ((volatile u8*)0x3010)

while (1) {
    while (!SA1_SYNC) { }   /* Wait for SA-1 */

    /* Read results from I-RAM */
    for (i = 0; i < count; i++) {
        result[i] = SA1_BUF[i];
    }

    SA1_SYNC = 0;           /* Tell SA-1 we consumed */
    WaitForVBlank();
}
```

### Pattern 2: Free-Running Counter

The SA-1 increments a value continuously. The main CPU reads it whenever needed.
No synchronization required — just atomic reads. See sa1_hello for register readback examples.

```asm
; SA-1 side: increment 32-bit counter forever
_sa1_count_loop:
    lda.l $003002       ; Load low word
    inc a
    sta.l $003002       ; Store low word
    bne _sa1_count_loop
    lda.l $003004       ; Carry: increment high word
    inc a
    sta.l $003004
    jmp _sa1_count_loop
```

```c
/* Main CPU side: just read the counter */
u16 lo = *(volatile u16*)0x3002;
u16 hi = *(volatile u16*)0x3004;
```

## I-RAM Layout Convention

We recommend this layout for SA-1 projects:

| Address | Size | Purpose |
|---------|------|---------|
| $3000 | 1 | Magic/status byte ($A5 = ready) |
| $3001 | 1 | Sync flag (0 = idle, 1 = data ready) |
| $3002-$300F | 14 | Control variables (counters, params) |
| $3010-$37FF | 2032 | Data buffer (up to 508 entries × 4 bytes) |

Both CPUs must agree on this layout. Define constants in both your
C code and assembly:

```c
#define SA1_SYNC      (*(volatile u8*)0x3001)
#define SA1_BUFFER    ((volatile u8*)0x3010)
```

## SA-1 Assembly Tips

### Use `sta.l` / `lda.l` for Everything

The SA-1's DB register is undefined at boot. Don't use `sta addr` (absolute) —
use `sta.l addr` (long absolute) to specify the full 24-bit address.

```asm
; WRONG — depends on DB register:
sta $3008

; CORRECT — full 24-bit address, no DB dependency:
sta.l $3008
```

### ROM Table Lookups with `lda.l table,x`

To read data tables in ROM from the SA-1, use `lda.l table,x` (opcode $BF).
This is a long indexed read — no DB dependency:

```asm
; Sine table in the same SUPERFREE section
lda.l $3006         ; index
rep #$20
.ACCU 16
and #$00FF          ; zero-extend for 16-bit X
tax
sep #$20
.ACCU 8
lda.l sine_table,x  ; opcode $BF — full 24-bit + X
```

### No `stx.l` — Save X Through A

The 65816 has no `stx.l` instruction. To save/restore X via I-RAM:

```asm
txa                 ; X → A
sta.l $3008         ; save to I-RAM

; ... later ...
lda.l $3008         ; reload
tax                 ; A → X
```

### Explicit `.ACCU` After Every `rep`/`sep`

WLA-DX loses accumulator width tracking after branch merges. Always add
explicit `.ACCU 8`/`.ACCU 16` directives:

```asm
rep #$20
.ACCU 16            ; ALWAYS add this!
; ... 16-bit code ...

sep #$20
.ACCU 8             ; ALWAYS add this!
; ... 8-bit code ...
```

## Debugging in Mesen2

Mesen2 has a dedicated SA-1 debugger:

1. **Debug → SA-1 Debugger** — separate window for SA-1 registers, disassembly
2. **Uncheck "Break on Power/Reset"** — otherwise the SA-1 freezes at boot
3. **Memory viewer** — switch to "SA-1 Bus" to see I-RAM from SA-1's perspective
4. **Watch $3000** — verify the $A5 magic byte appears after boot

Common issues:
- **SA-1 PC stuck at $0000**: CIWP not set — SA-1 can't write I-RAM
- **I-RAM reads return $FF**: SIWP not set — main CPU can't read I-RAM
- **Counter not incrementing**: Check that `$2200` was written correctly ($00 = release)

## Examples

| Example | What it demonstrates |
|---------|---------------------|
| [sa1_hello](../../../examples/memory/sa1_hello/) | Boot diagnostic — verifies SA-1 initialization |
| [sa1_starfield](../../../examples/memory/sa1_starfield/) | 128 sprites with sine-wave Lissajous patterns |

## What SA-1 Can't Do

- **No PPU access** — SA-1 can compute sprite positions but can't write OAM/VRAM
- **No APU access** — can't trigger sound effects
- **No WRAM access** — can't read/write the main CPU's 128 KB RAM
- **No joypad reading** — input comes through main CPU → I-RAM

The SA-1 is a **compute accelerator**. The main CPU remains the "director" —
it reads input, talks to the PPU/APU, and delegates heavy math to the SA-1.

## Further Reading

- [SA-1 Register Reference](../../.claude/SA-1.md) — complete register documentation
- [sa1.h API](../../lib/include/snes/sa1.h) — library header with register macros
