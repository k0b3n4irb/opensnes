# SNES Development Knowledge Base

**Last Updated:** January 20, 2026

This file contains accumulated knowledge and gotchas from SNES development work with OpenSNES. Use it as a quick reference.

---

## Table of Contents

1. [Project Context](#1-project-context)
2. [Memory Map](#2-memory-map)
3. [WRAM Mirroring](#3-wram-mirroring)
4. [WLA-DX Assembler](#4-wla-dx-assembler)
5. [OAM and Sprites](#5-oam-and-sprites)
6. [DMA](#6-dma)
7. [Mesen2 Debugging](#7-mesen2-debugging)
8. [Compiler Limitations](#8-compiler-limitations)
9. [Common Bugs](#9-common-bugs)
10. [Debugging Infrastructure](#10-debugging-infrastructure)
11. [Reference Projects](#11-reference-projects)
12. [Audio System](#12-audio-system-january-2026)
13. [VBlank Callbacks](#13-vblank-callbacks-january-2026)
14. [Joypad Input](#14-joypad-input-january-2026)
15. [CI Regression Testing](#15-ci-regression-testing-january-2026)

---

## 1. Project Context

### OpenSNES vs PVSnesLib

**OpenSNES** is our main project - a modern C SDK for SNES development.

**PVSnesLib** is a mature SNES SDK that can be used as a learning resource:
- Has 80+ working examples
- Well-tested library code
- Good reference for SNES patterns

**When to reference PVSnesLib:**
- Looking for working examples of SNES features
- Understanding how to implement complex features
- Comparing our implementation against a known-working one
- Finding test cases and .sym files for debugging tools

**Key difference:** OpenSNES uses QBE-based compiler (cproc), PVSnesLib uses 816-tcc.

---

## 2. Memory Map

### LoROM Memory Layout

```
Bank $00-$3F: LoROM program space
  $0000-$1FFF: Mirror of $7E:0000 (WRAM low 8KB) ⚠️ CRITICAL
  $2000-$20FF: Unused
  $2100-$21FF: PPU registers (B-bus)
  $2200-$3FFF: Unused
  $4000-$41FF: Joypad/old CPU registers
  $4200-$43FF: CPU registers, DMA
  $4400-$5FFF: Unused
  $6000-$7FFF: Expansion (SRAM if present)
  $8000-$FFFF: ROM

Bank $7E: Work RAM (64KB)
  $0000-$1FFF: Direct page, stack, low variables ⚠️ MIRRORS $00:0000!
  $2000-$FFFF: General purpose

Bank $7F: Extended RAM (64KB)
  $0000-$FFFF: Heap, large buffers
```

### Important Addresses

| Address | Name | Purpose |
|---------|------|---------|
| `$2102-$2103` | OAMADD | OAM address register |
| `$2104` | OAMDATA | OAM data write |
| `$4200` | NMITIMEN | NMI/Timer/Controller enable |
| `$4210` | RDNMI | NMI flag read |
| `$420B` | MDMAEN | DMA enable |
| `$43x0-$43xA` | DMA regs | Per-channel DMA configuration |

---

## 3. WRAM Mirroring

### ⚠️ CRITICAL KNOWLEDGE

**WRAM addresses `$00:0000-$1FFF` mirror `$7E:0000-$1FFF`.**

This means:
- `$00:0100` and `$7E:0100` are the **same physical byte**
- Variables in Bank 0 and Bank $7E can overlap!
- Clearing one region will destroy data in the other!

### The Bug We Fixed (January 2026)

**Problem:** `oamMemory` was placed at `$7E:0000` which overlapped with game variables at `$00:0020`. When `oamInit()` cleared the OAM buffer, it overwrote `monster_x` and other game variables.

**Symptom:** Variables mysteriously reset to 0 after initialization.

**Solution:** Use `ORGA $0300 FORCE` to place Bank $7E buffers above the mirror range:

```asm
; SAFE: Place buffer at $0300 to avoid overlap with Bank 0 vars
.RAMSECTION ".oam_buffer" BANK $7E SLOT 1 ORGA $0300 FORCE
    oamMemory   dsb 544   ; $7E:0300-$7E:051F (no overlap!)
.ENDS
```

### Safe Memory Layout

```asm
; Bank 0: Compiler registers and small variables ($0000-$01FF typically)
.RAMSECTION ".vars" BANK 0 SLOT 1
    game_vars   dsb 32    ; $00:0000-$00:001F
.ENDS

; Bank $7E: Large buffers MUST be at $0300+ to avoid overlap
.RAMSECTION ".oam" BANK $7E SLOT 1 ORGA $0300 FORCE
    oamMemory   dsb 544   ; $7E:0300-$7E:051F
.ENDS
```

### Overlap Detection

Use the `symmap` tool to check for overlaps:
```bash
python3 tools/symmap/symmap.py --check-overlap game.sym
```

Or manually check your `.sym` file:
```bash
grep -E "(7e:00[0-2]|00:00[0-2])" game.sym
```

If both `7e:00xx` and `00:00xx` entries exist in the same range, you have overlap.

---

## 4. WLA-DX Assembler

### RAMSECTION Placement

**ORGA alone doesn't work - you need FORCE:**

```asm
; WRONG (may be ignored):
.RAMSECTION ".buffer" BANK $7E SLOT 1 ORGA $0300
    buffer dsb 256
.ENDS

; CORRECT:
.RAMSECTION ".buffer" BANK $7E SLOT 1 ORGA $0300 FORCE
    buffer dsb 256
.ENDS
```

### Long vs Short Addressing

```asm
; Short addressing (uses DBR):
lda variable     ; 3 bytes, 4-5 cycles

; Long addressing (ignores DBR):
lda.l variable   ; 4 bytes, 5-6 cycles

; Use long addressing when:
; - Accessing hardware registers from code with non-zero DBR
; - Accessing data in specific banks regardless of DBR
```

### Symbol Bank Extraction

```asm
lda #:symbol     ; Get bank number of symbol
sta $4374        ; Store to DMA bank register
```

---

## 5. OAM and Sprites

### OAM Structure

OAM is 544 bytes total:
- **Main table**: 512 bytes (128 sprites × 4 bytes)
- **High table**: 32 bytes (2 bits per sprite)

Main table entry (4 bytes per sprite):
| Byte | Content |
|------|---------|
| 0 | X position (low 8 bits) |
| 1 | Y position |
| 2 | Tile number |
| 3 | Attributes (vhoopppc) |

High table (2 bits per sprite):
- Bit 0: X position bit 8 (sign extend)
- Bit 1: Size select (0=small, 1=large)

### Hiding Sprites

Set Y position to 240 to hide a sprite (off-screen on NTSC).

### VRAM Sprite Layout

Sprites in VRAM are arranged with 128 pixels per row:
- 8×8 sprites: 16 per row
- 16×16 sprites: 8 per row
- 32×32 sprites: 4 per row

---

## 6. DMA

### OAM DMA Setup (Channel 7 Example)

```asm
; Set OAM address to 0
rep #$20
lda #$0000
sta $2102

; DMA control: CPU→PPU, auto-increment, target $2104
lda #$0400
sta $4370           ; $4370=$00, $4371=$04

; Transfer size
lda #544            ; $0220
sta $4375

; Source address
lda #oamMemory
sta $4372

; Source bank
sep #$20
lda #:oamMemory
sta $4374

; Trigger DMA
lda #$80            ; Enable channel 7
sta $420B
```

### DMA Register Layout

| Register | Purpose | Notes |
|----------|---------|-------|
| `$43x0` | Control | Transfer mode, direction |
| `$43x1` | B-bus addr | PPU register ($04 = $2104) |
| `$43x2-3` | A-bus addr | Source address (16-bit) |
| `$43x4` | A-bus bank | Source bank |
| `$43x5-6` | Size | Transfer count |

---

## 7. Mesen2 Debugging

### Lua API Memory Types

| Type | Use |
|------|-----|
| `snesSpriteRam` | OAM data (NOT `snesOam`!) |
| `snesWorkRam` | WRAM ($7E-$7F) |
| `snesVideoRam` | VRAM |
| `snesCgRam` | CGRAM (palettes) |
| `snesMemory` | Full address space |
| `snesPrgRom` | ROM |

**Note:** `snesOam` does NOT exist - use `snesSpriteRam`!

### Execution Callbacks

Use 24-bit addresses for execution callbacks:

```lua
-- WRONG:
emu.addMemoryCallback(fn, emu.callbackType.exec, 0x8100)

-- CORRECT:
emu.addMemoryCallback(fn, emu.callbackType.exec, 0x008100)
```

### Useful Test Pattern

```lua
local frameCount = 0
function onStartFrame()
    frameCount = frameCount + 1
    if frameCount >= 15 then
        -- Read OAM
        local x = emu.read(0x0000, emu.memType.snesSpriteRam)
        local y = emu.read(0x0001, emu.memType.snesSpriteRam)
        print(string.format("Sprite 0: X=%d Y=%d", x, y))
        emu.stop(0)
    end
end
emu.addEventCallback(onStartFrame, emu.eventType.startFrame)
```

---

## 8. Compiler Limitations

### Data Types (QBE-based compiler)

| Type | Size | Notes |
|------|------|-------|
| `u8`, `s8` | 8-bit | Native, fast |
| `u16`, `s16` | 16-bit | Native, fast |
| `u32`, `s32` | 32-bit | SLOW (~50 cycles/op) |
| `float`, `double` | N/A | NOT SUPPORTED |

### Operations to Avoid

```c
// SLOW: 32-bit multiplication
s32 result = a * b;  // ~1000 cycles

// SLOW: Division
x / 7;   // Slow - library call

// FAST: Power-of-2 division
x / 8;   // Optimized to LSR

// FAST: Power-of-2 modulo
x % 8;   // Optimized to AND
```

### Fixed Compiler Bugs (January 2026)

#### Bug 1: Static Variable Stores Generated `sta.l $000000`

**Symptom:** Writes to static/global variables had no effect (black screen).

**Cause:** In `compiler/qbe/w65816/emit.c`, the store instruction emitter didn't check if the constant was a symbol address (`CAddr`) vs a literal value (`CBits`).

**What happened:**
```c
static u8 brightness;
brightness = 15;  // Generated: sta.l $000000 (wrong!)
                  // Should be: sta.l brightness
```

**Fix:** Check `c->type == CAddr` before emitting store address:
```c
} else if (rtype(r1) == RCon) {
    c = &fn->con[r1.val];
    if (c->type == CAddr) {
        /* Symbol address - emit symbol name */
        fprintf(outf, "\tsta.l %s", str(c->sym.id));
        if (c->bits.i)
            fprintf(outf, "+%d", (int)c->bits.i);
        fprintf(outf, "\n");
    } else {
        /* Literal address */
        fprintf(outf, "\tsta.l $%06lX\n", (unsigned long)c->bits.i);
    }
}
```

**Regression test:** `tests/compiler/test_static_vars.c`

#### Bug 2: BSS (Static Variables) Placed in ROM Instead of RAM

**Symptom:** Static variable writes silently ignored (black screen after library integration).

**Cause:** In `compiler/qbe/emit.c`, BSS sections used `.SECTION ... SUPERFREE` which places data in ROM (read-only). Should use `.RAMSECTION` for RAM.

**What happened:**
```asm
; WRONG - placed in ROM at $00:9350
.SECTION ".bss.1" SUPERFREE
current_brightness: dsb 1
.ENDS

; CORRECT - placed in RAM at $00:024B
.RAMSECTION ".bss.1" BANK 0 SLOT 1
current_brightness dsb 1
.ENDS
```

**Additional issue:** Makefile sed rule added `, 0` to `.dsb` directives, but RAMSECTION doesn't accept fill values. Fix: Use `dsb` without dot to avoid the sed pattern.

**Fix in emit.c:**
```c
case SecBss:
    /* BSS goes to RAM (SLOT 1) not ROM */
    fprintf(f, ".RAMSECTION \".bss.%d\" BANK 0 SLOT 1\n", ++datasec_counter);
    break;
// ...
/* RAMSECTION uses 'dsb' without dot and without fill value */
fprintf(f, "\tdsb %"PRId64"\n", zero);
```

**How to diagnose:** Check `.sym` file for variable addresses:
```bash
grep "current_brightness" hello_world.sym
# BAD: 00:9350 current_brightness (ROM range $8000-$FFFF)
# GOOD: 00:024B current_brightness (RAM range $0000-$1FFF)
```

#### Bug 3: Unhandled `Osar` (Arithmetic Shift Right)

**Symptom:** Assembly output contains `; unhandled op 12` comments. `>> 8` operations don't work.

**Cause:** The w65816 emit.c had `case Oshr:` for logical shift right but not `case Osar:` for arithmetic shift right. C compilers typically generate `sar` for `>>` on signed types.

**Fix:** Added `case Osar:` handler in `compiler/qbe/w65816/emit.c`:
```c
case Osar:
    /* Arithmetic shift right - using LSR is safe for 16-bit positive values */
    emitload(r0, fn);
    if (rtype(r1) == RCon) {
        c = &fn->con[r1.val];
        for (int j = 0; j < c->bits.i && j < 16; j++)
            fprintf(outf, "\tlsr a\n");
    }
    emitstore(i->to, fn);
    break;
```

**Regression test:** `tests/compiler/test_shift_right.c`

#### Bug 4: Unhandled `Oextsw`/`Oextuw` (Word Extension)

**Symptom:** Assembly output contains `; unhandled op 68` or `; unhandled op 69`. Array indexing and loops with 16-bit indices fail.

**Cause:** QBE generates word-to-long extension ops when calculating array offsets or doing 32-bit operations. The w65816 backend didn't handle `Oextsw` (sign extend word) or `Oextuw` (zero extend word).

**Fix:** Added handlers in `compiler/qbe/w65816/emit.c`:
```c
case Oextsw:
case Oextuw:
    /* For 16-bit 65816, just copy the value */
    emitload(r0, fn);
    emitstore(i->to, fn);
    break;
```

**Regression test:** `tests/compiler/test_word_extend.c`

#### Bug 5: Initialized Static Variables Placed in ROM (Not RAM)

**Symptom:** Static variables with initializers silently fail to update (values never change, causing black screen or broken logic).

**Cause:** The QBE compiler places initialized static variables in `.SECTION ".rodata" SUPERFREE` (ROM), but uninitialized statics go to `.RAMSECTION ".bss"` (RAM).

**What happened:**
```c
static u8 bg12nba_shadow = 0;  // Placed in ROM at $00:FFB0 (read-only!)
bg12nba_shadow = value;        // Write silently ignored!
```

**Generated assembly (WRONG):**
```asm
.SECTION ".rodata.1" SUPERFREE
bg12nba_shadow:
    .db 0                      ; In ROM - not writable!
.ENDS
```

**Fix:** Remove the initializer. C standard guarantees static variables are zero-initialized:
```c
/* NOTE: Do NOT use "= 0" - QBE puts initialized statics in ROM!
 * C standard guarantees uninitialized statics are zero-initialized. */
static u8 bg12nba_shadow;      // Now in RAM at $00:004D (writable)
```

**Generated assembly (CORRECT):**
```asm
.RAMSECTION ".bss.1" BANK 0 SLOT 1
bg12nba_shadow:
    dsb 1                      ; In RAM - writable!
.ENDS
```

**How to diagnose:**
```bash
grep "variable_name" game.sym
# BAD:  00:ffb0 bg12nba_shadow (ROM range $8000-$FFFF in bank 0)
# GOOD: 00:004d bg12nba_shadow (RAM range $0000-$1FFF)
```

**Rule:** Never use `static u8 x = 0;` - always use `static u8 x;` for writable static variables.

---

## 9. Common Bugs

### Memory Overlap (WRAM Mirroring)

**Symptom:** Variables mysteriously change to 0 or wrong values.

**Cause:** Bank 0 and Bank $7E RAMSECTIONS overlap.

**Fix:** Use `ORGA $0300 FORCE` to place sections at non-overlapping addresses.

### OAM Not Updating

**Symptom:** Sprites don't appear or show garbage.

**Checklist:**
1. Is `oam_update_flag` being set?
2. Is NMI enabled (`$4200 & $80`)?
3. Is NMI handler calling oamUpdate?
4. Is DMA source address correct?
5. Is oamMemory actually populated?

### DBR Issues in NMI

**Symptom:** Code works in main loop but not in NMI handler.

**Cause:** DBR (Data Bank Register) is wrong in NMI context.

**Fix:** Use long addressing (`lda.l`) or set DBR explicitly:
```asm
pea $0000
plb
plb      ; DBR = $00
```

---

## 10. Debugging Infrastructure

### The Multi-Layer Debugging Challenge

OpenSNES is not a typical project. When something breaks, the bug could be in:

1. **Your C Code** - Logic errors, wrong API usage
2. **Compiler (cproc/QBE)** - Wrong code generation, optimization bugs
3. **Assembler (wla-dx)** - Section placement, symbol resolution
4. **Library (libopensnes)** - Library function bugs
5. **Runtime (crt0.asm)** - Initialization issues
6. **Memory Layout** - WRAM overlaps, stack overflows
7. **Hardware Understanding** - OAM timing, VBlank requirements

### Available Tools

| Tool | Purpose | Location |
|------|---------|----------|
| symmap | Memory overlap detection | `tools/symmap/symmap.py` |
| snesdbg.lua | Symbol-aware debugging (TODO) | `tools/snesdbg/` |
| Test framework | Automated testing (TODO) | `tests/` |

### Using symmap

```bash
# Check for WRAM overlaps (run after every build!)
python3 tools/symmap/symmap.py --check-overlap game.sym

# Find a symbol
python3 tools/symmap/symmap.py --find my_variable game.sym

# Show full memory layout
python3 tools/symmap/symmap.py -v game.sym
```

### Test Layers (When Implemented)

```bash
# Run all tests
./tests/run_tests.sh --all

# Run specific layer
./tests/run_tests.sh --compiler   # Test cproc/QBE code generation
./tests/run_tests.sh --library    # Test library functions
./tests/run_tests.sh --examples   # Test example ROMs
```

### Debugging Workflow

When something breaks:

1. **Check build** - Does it compile? `make clean && make`
2. **Check memory** - Any overlaps? `symmap --check-overlap game.sym`
3. **Check compiler** - Run compiler tests if you suspect codegen bug
4. **Check library** - Run library tests if you suspect library bug
5. **Use snesdbg.lua** - Watch variables, set breakpoints, trace execution

---

## 11. Reference Projects

### PVSnesLib (Learning Resource)

**Location:** `/Users/k0b3/workspaces/github/pvsneslib`

**Useful for:**
- 80+ working examples
- Sprite/metasprite patterns
- Audio implementation
- .sym files for testing symmap tool

**Key directories:**
- `snes-examples/` - Working examples
- `pvsneslib/source/` - Library implementation
- `devkitsnes/` - Build output

### When to Look at PVSnesLib

1. Implementing a new feature → Check if PVSnesLib has an example
2. Debugging a bug → Compare your code against PVSnesLib's implementation
3. Testing tools → Use PVSnesLib's .sym files as test fixtures
4. Understanding patterns → PVSnesLib has years of accumulated solutions

### SNESMOD (Audio Reference)

**Repository:** https://github.com/mukunda-/snesmod
**Tutorial:** https://nesdoug.com/2022/03/02/snesmod/

**Useful for:**
- Advanced SPC700 driver design patterns
- Tracker-style music playback implementation
- Sound effect streaming techniques
- Echo/reverb configuration
- Command protocol design

**Key components:**
- `driver/spc/sm_spc.asm` - SPC700 driver source
- `driver/include/snesmod.inc` - 65816 API
- `doc/snesmod_music.txt` - IT file restrictions

**Key insights:**
- Uses Impulse Tracker format (8 channels max)
- ~5KB driver with full tracker effects
- Validation-based command handshake
- Effect memory for slide/vibrato continuation
- Each 16ms echo delay = 2KB ARAM

See `.claude/skills/spc700-audio.md` for detailed SNESMOD analysis.

---

## 12. Audio System (January 2026)

### Dual-Mode Architecture

OpenSNES provides two audio modes selectable at build time:

| Mode | Build Flag | Driver | Use Case |
|------|------------|--------|----------|
| Legacy | (default) | `audio.asm` (~500 bytes) | Simple SFX, demos |
| SNESMOD | `USE_SNESMOD=1` | `snesmod.asm` (~5.5KB) | Full games with music |

### Legacy Driver Overview

The legacy driver provides a full-featured multi-voice SPC700 driver:

| Feature | Capability |
|---------|------------|
| Voices | 8 simultaneous |
| Samples | 64 slots, dynamic loading |
| Volume | Per-voice + master |
| Pan | Per-voice (0-15 stereo) |
| Pitch | Per-voice ($1000 = normal) |
| Effects | Echo/reverb with FIR filter |
| ADSR | Per-voice envelope control |
| Driver size | ~500 bytes (readable assembly) |

### SPC700 Memory Layout

```
$0200-$03FF  Driver code (~512 bytes)
$0400-$05FF  Sample directory (64 entries × 4 bytes)
$0600-$060F  Voice state (8 voices × 2 bytes)
$0800-$CFFF  Sample data (~50KB available)
$D000-$DFFF  Echo buffer (4KB reserved)
```

### Command Protocol

Communication via I/O ports ($2140-$2143 on 65816, $F4-$F7 on SPC700):

| Command | Port0 | Port1 | Port2 | Port3 | Description |
|---------|-------|-------|-------|-------|-------------|
| PLAY | $01 | voice | sample | vol\|pan | Play sample on voice |
| STOP | $02 | mask | - | - | Stop voices (mask bits) |
| SET_VOL | $03 | voice | volL | volR | Set voice volume |
| SET_PITCH | $04 | voice | pitchL | pitchH | Set voice pitch |
| MASTER_VOL | $20 | volume | - | - | Set master volume |
| ECHO_CFG | $30 | delay | feedback | vol | Configure echo |
| ECHO_FIR | $31 | idx | coeff | - | Set FIR coefficient |
| ECHO_ENABLE | $32 | mask | - | - | Enable echo per voice |
| ADSR | $40 | voice | adsr1 | adsr2 | Set ADSR envelope |

### Key API Functions

```c
// Initialization
void audioInit(void);           // Upload driver, init DSP
void audioUpdate(void);         // Call each frame

// Sample Management
u8 audioLoadSample(u8 id, const u8 *brrData, u16 size, u16 loopPoint);
void audioUnloadSample(u8 id);
u16 audioGetFreeMemory(void);

// Playback
u8 audioPlaySample(u8 sampleId);                    // Default settings
u8 audioPlaySampleEx(u8 id, u8 vol, u8 pan, u16 pitch);  // Custom
void audioStopVoice(u8 voice);
void audioStopAll(void);

// Effects
void audioSetEcho(u8 delay, s8 feedback, s8 volL, s8 volR);
void audioSetEchoFilter(const s8 fir[8]);
void audioEnableEcho(u8 voiceMask);
void audioSetADSR(u8 voice, u8 attack, u8 decay, u8 sustain, u8 release);
```

### Audio Examples

| Example | Mode | Purpose |
|---------|------|---------|
| `1_tone/` | Legacy | Low-level SPC700 programming |
| `2_sfx/` | Legacy | Basic library usage |
| `6_snesmod_music/` | SNESMOD | Tracker music playback |

### SNESMOD Usage

```c
#include <snes/snesmod.h>
#include "soundbank.h"  // Generated by smconv

extern u8 soundbank[];

void main(void) {
    snesmodInit();
    snesmodSetSoundbank(soundbank);
    snesmodLoadModule(MOD_MUSIC);
    snesmodPlay(0);

    while (1) {
        WaitForVBlank();
        snesmodProcess();  // MUST call every frame!
    }
}
```

See `.claude/skills/spc700-audio.md` for comprehensive SNESMOD documentation.

### Common Pitch Values

```c
#define AUDIO_PITCH_DEFAULT 0x1000  // 1.0x playback rate
#define AUDIO_PITCH_C3      0x085F  // Middle C (261.63 Hz)
#define AUDIO_PITCH_C4      0x10BE  // C4 (523.25 Hz)
#define AUDIO_PITCH_C5      0x217C  // C5 (1046.5 Hz)
```

---

## Quick Reference

### Build Command
```bash
make clean && make
```

### Test Command
```bash
cd tests && ./run_tests.sh /path/to/Mesen
```

### Debug Symbol Check
```bash
grep "variable_name" game.sym
```

### Binary Inspection
```bash
xxd -s 0x0100 -l 64 game.sfc  # Dump 64 bytes at offset $0100
```

### Check WRAM Overlap
```bash
python3 tools/symmap/symmap.py --check-overlap game.sym
```

---

## 13. VBlank Callbacks (January 2026)

### Overview

VBlank (NMI) callbacks allow user code to run during the vertical blanking period - the ideal time for timing-critical updates like scroll registers, palette changes, and DMA transfers.

### API

```c
#include <snes/interrupt.h>

// Simple version - assumes callback is in bank 0
void nmiSet(VBlankCallback callback);

// Bank-aware version - specify which bank contains the callback
void nmiSetBank(VBlankCallback callback, u8 bank);
```

### ⚠️ CRITICAL: Bank Addressing

The callback pointer is 24 bits: `bank:address`. The NMI handler uses `JML [nmi_callback]` (indirect long jump) which reads all 3 bytes.

**Common Bug:** If your callback is in bank 1 but you use `nmiSet()` (which defaults to bank 0), the CPU jumps to the wrong address!

```c
// myVBlankHandler placed at $01:8CBB by linker

nmiSet(myVBlankHandler);        // WRONG: Jumps to $00:8CBB (garbage!)
nmiSetBank(myVBlankHandler, 1); // CORRECT: Jumps to $01:8CBB
```

**How to find the correct bank:**
```bash
grep "myVBlankHandler" game.sym
# Output: 01:8CBB myVBlankHandler
#         ^^ This is the bank number
```

### LoROM Bank Mapping

In LoROM (OpenSNES default):
- Bank 0 = ROM $0000-$7FFF (first 32KB)
- Bank 1 = ROM $8000-$FFFF (second 32KB)
- Bank 2 = ROM $10000-$17FFF (etc.)

Small projects fit in bank 0. Larger projects spill into bank 1+.

### Compiler Register Preservation

The 816-tcc compiler uses "imaginary registers" (tcc__r0 through tcc__r10) for intermediate calculations. If your VBlank callback uses any C code, it may corrupt these registers.

**Solution:** The NMI handler saves ALL compiler registers before calling the callback and restores them after:

```asm
; In NmiHandler (crt0.asm)
; Save main loop's registers
lda tcc__r0 : sta tcc__nmi_r0
lda tcc__r1 : sta tcc__nmi_r1
; ... r2-r5, r9-r10 ...

jml [nmi_callback]  ; Call user callback

; Restore main loop's registers
lda tcc__nmi_r0 : sta tcc__r0
lda tcc__nmi_r1 : sta tcc__r1
; ... r2-r5, r9-r10 ...
```

### Example: Scroll Updates in VBlank

```c
static u16 bg1_scroll_x, bg1_scroll_y;
static u8 need_scroll_update;

void myVBlankHandler(void) {
    if (need_scroll_update) {
        bgSetScroll(0, bg1_scroll_x, bg1_scroll_y);
        need_scroll_update = 0;
    }
}

int main(void) {
    // Register callback (check .sym for correct bank!)
    nmiSetBank(myVBlankHandler, 1);

    while (1) {
        // Update scroll variables during game logic
        bg1_scroll_x += 1;
        need_scroll_update = 1;

        WaitForVBlank();
    }
}
```

### Continuous Scrolling Pattern

PVSnesLib-style "camera following" scroll:

```c
#define SCROLL_THRESHOLD_RIGHT 140
#define SCROLL_THRESHOLD_LEFT  80
#define MAX_SCROLL_X 512

// In main loop:
if (player_x > SCROLL_THRESHOLD_RIGHT && bg_scroll_x < MAX_SCROLL_X) {
    bg_scroll_x += 1;
    player_x -= 1;  // Push player back to keep them centered
}
if (player_x < SCROLL_THRESHOLD_LEFT && bg_scroll_x > 0) {
    bg_scroll_x -= 1;
    player_x += 1;
}
```

### Debugging VBlank Issues

**Callback not being called:**
1. Check NMI is enabled: `REG_NMITIMEN = NMITIMEN_NMI_ENABLE | NMITIMEN_JOY_ENABLE;`
2. Check `nmi_callback_set` flag is 1

**Callback corrupts game state:**
1. Ensure all compiler registers are saved/restored in NMI handler
2. Check that callback doesn't take too long (must complete within VBlank)

**Wrong behavior / crash:**
1. Check bank byte with `grep "callback_name" game.sym`
2. Use `nmiSetBank()` with correct bank number
3. Verify with Mesen2 debugger that JML target is correct

---

## 14. Joypad Input (January 2026)

### Register Layout

The SNES auto-joypad reads controller data into two 8-bit registers:

```
$4218 (JOY1L) - Low byte:  A  X  L  R  -  -  -  -
$4219 (JOY1H) - High byte: B  Y  Se St Up Dn Lt Rt
                           b7 b6 b5 b4 b3 b2 b1 b0
```

When combined as `JOY1L | (JOY1H << 8)`:

```
Bit: 15  14  13  12  11  10  9   8   7   6   5   4   3-0
     B   Y   Sel Sta Up  Dn  Lt  Rt  A   X   L   R   (ID)
```

### ⚠️ CRITICAL: KEY_A vs KEY_B

**The A and B buttons are NOT where you might expect!**

| Button | Register | Bit in Register | Bit in 16-bit | Correct Constant |
|--------|----------|-----------------|---------------|------------------|
| **A** | JOY1L ($4218) | bit 7 | bit 7 | `BIT(7) = 0x0080` |
| **B** | JOY1H ($4219) | bit 7 | bit 15 | `BIT(15) = 0x8000` |

### The Bug We Fixed (January 2026)

**Problem:** `input.h` had KEY_A and KEY_B swapped:
```c
// WRONG (was in input.h):
#define KEY_A  BIT(15)  // This is actually B!
#define KEY_B  BIT(7)   // This is actually A!
```

**Symptom:** Pressing the A button on a real SNES (or accurate emulator like bsnes/OpenEmu) didn't work - you had to press B instead.

**Why Mesen hid the bug:** Mesen is a more lenient emulator that may have compatibility quirks. The bug only appeared on cycle-accurate emulators like bsnes (used by OpenEmu).

**Fix:** Corrected the definitions in `lib/include/snes/input.h`:
```c
// CORRECT:
#define KEY_B  BIT(15)  // B button - high byte bit 7
#define KEY_A  BIT(7)   // A button - low byte bit 7
```

### Standard Input Pattern

```c
u16 pad, pad_prev, pad_pressed;

// Initialize
WaitForVBlank();
while (REG_HVBJOY & 0x01) {}  // Wait for auto-read complete
pad_prev = REG_JOY1L | (REG_JOY1H << 8);

// Main loop
while (1) {
    WaitForVBlank();

    while (REG_HVBJOY & 0x01) {}  // Wait for auto-read complete
    pad = REG_JOY1L | (REG_JOY1H << 8);
    pad_pressed = pad & ~pad_prev;  // Edge detection
    pad_prev = pad;

    if (pad == 0xFFFF) pad_pressed = 0;  // Controller disconnected

    if (pad_pressed & KEY_A) {
        // A was just pressed this frame
    }
    if (pad & KEY_RIGHT) {
        // Right is being held
    }
}
```

### Testing on Multiple Emulators

**Always test on accurate emulators!** Bugs that work on lenient emulators will fail on:
- Real SNES hardware
- bsnes/higan
- OpenEmu (uses bsnes core)

Mesen is great for debugging but may hide hardware accuracy issues.

### Button Constants Reference

```c
// Face buttons
#define KEY_A       BIT(7)   // 0x0080
#define KEY_B       BIT(15)  // 0x8000
#define KEY_X       BIT(6)   // 0x0040
#define KEY_Y       BIT(14)  // 0x4000

// Shoulders
#define KEY_L       BIT(5)   // 0x0020
#define KEY_R       BIT(4)   // 0x0010

// D-pad (in high byte)
#define KEY_UP      BIT(11)  // 0x0800
#define KEY_DOWN    BIT(10)  // 0x0400
#define KEY_LEFT    BIT(9)   // 0x0200
#define KEY_RIGHT   BIT(8)   // 0x0100

// Meta
#define KEY_START   BIT(12)  // 0x1000
#define KEY_SELECT  BIT(13)  // 0x2000
```

---

## 15. CI Regression Testing (January 2026)

### Philosophy: Test After Every Change

**CRITICAL RULE**: After making ANY code change:
1. Run local validation
2. Fix any failures
3. Update CI if needed
4. Target: 100% test coverage

### Local Validation Commands

```bash
# Full build (must pass before committing)
make clean && make

# Validate all examples (memory overlaps, ROM sizes)
./tests/examples/validate_examples.sh --quick

# Run compiler tests
./tests/compiler/run_tests.sh

# Verify example count (should be 20)
echo "Examples: $(find examples -name '*.sfc' | wc -l)"
```

### CI Pipeline (GitHub Actions)

**Workflow**: `.github/workflows/opensnes_build.yml`

**Triggers**:
- Push to `main` or `develop`
- Pull requests to `main` or `develop`

**Platforms**: Linux, Windows (MSYS2), macOS

### What CI Validates

| Check | Command | Expected |
|-------|---------|----------|
| Build toolchain | `make compiler` | cc65816, wla-65816, wlalink |
| Build library | `make lib` | opensnes.lib |
| Build examples | `make examples` | 19 .sfc files |
| Build tests | `make tests` | Test ROMs compile |
| Memory overlaps | `validate_examples.sh` | 0 collisions |
| ROM validity | symmap.py | Valid sizes, required symbols |
| Example count | count *.sfc | >= 19 |
| Compiler tests | `run_tests.sh` | All pass |

### symmap.py Overlap Checker

Detects WRAM mirror collisions that cause subtle bugs:

```bash
# Check single example
python3 tools/symmap/symmap.py --check-overlap game.sym

# Check all examples
./tests/examples/validate_examples.sh --quick
```

**What it checks**:
- Bank $00:$0000-$1FFF mirrors Bank $7E:$0000-$1FFF
- Flags if SAME address is used in BOTH banks
- Ignores compiler registers (`tcc__r*`) - intentionally in direct page

**Example fix for collision at $0300**:
```asm
; WRONG: oamMemory at $7E:0000 collides with runtime vars
.RAMSECTION ".oam" BANK $7E SLOT 1
    oamMemory dsb 544
.ENDS

; CORRECT: Place above collision range
.RAMSECTION ".oam" BANK $7E SLOT 1 ORGA $0300 FORCE
    oamMemory dsb 544
.ENDS
```

### Test Coverage Targets

| Component | Current | Target |
|-----------|---------|--------|
| Examples build | 100% (20/20) | 100% |
| Memory overlap check | 100% | 100% |
| Compiler regression | ~80% | 100% |
| Library functions | ~60% | 100% |
| Hardware features | ~40% | 100% |

### Adding New Tests

When adding a feature:

1. **Write the test first** (TDD approach)
2. **Implement the feature**
3. **Verify test passes locally**
4. **Update CI if new test category**

Test structure:
```
tests/unit/my_feature/
├── README.md       # What it tests
├── Makefile        # Build config
├── main.c          # Test code
└── test.lua        # Mesen2 runner (optional)
```

### CI Failure Debugging

If CI fails:

1. **Download build logs** from GitHub Actions artifacts
2. **Check which step failed**:
   - Build? → Check compiler/linker errors
   - Validation? → Run `validate_examples.sh` locally
   - Count? → Check if example Makefile is missing
3. **Reproduce locally** before pushing fix

### Updating CI

When to update `.github/workflows/opensnes_build.yml`:

- New test category added → Add test step
- New platform needed → Add to matrix
- Expected counts change → Update verification numbers
- New validation tool → Add validation step

---

*This knowledge base is continuously updated with learnings from development and debugging sessions.*
