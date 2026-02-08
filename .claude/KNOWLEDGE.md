# SNES Development Knowledge Base

**Last Updated:** February 3, 2026

This file contains accumulated knowledge and gotchas from SNES development work with OpenSNES. Use it as a quick reference.

---

## Table of Contents

1. [Project Context](#1-project-context)
2. [Memory Map](#2-memory-map) (includes HiROM configuration)
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
16. [Example Architecture](#16-example-architecture-january-2026)
17. [VBlank Timing Constraints](#17-vblank-timing-constraints-january-2026)
18. [HDMA (Horizontal DMA)](#18-hdma-horizontal-dma-january-2026)
19. [Background Modes](#19-background-modes-january-2026)
20. [Color Format - BGR555](#20-color-format---bgr555-january-2026)
21. [VRAM Tilemap Overlap and DMA Timing](#21-vram-tilemap-overlap-and-dma-timing-january-2026)
22. [Bug Fix Verification Protocol](#22-bug-fix-verification-protocol-january-2026)

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

### ⚠️ MANDATORY: Consult Reference Documentation First

**Before developing ANY new feature, ALWAYS read these files:**

| Document | Purpose | When to Read |
|----------|---------|--------------|
| **SNES_HARDWARE_REFERENCE.md** | Complete SNES technical reference (CPU, PPU, APU, DMA, timing) | Understanding hardware behavior, register usage, timing constraints |
| **SNES_ROSETTA_STONE.md** | OpenSNES vs PVSnesLib comparison, SDK patterns, API mapping | Implementing SDK features, understanding existing patterns |

**Why this matters:**
1. **Avoid reinventing the wheel** - The reference docs contain solutions to common problems
2. **Understand hardware constraints** - SNES has strict timing, memory, and DMA limitations
3. **Follow existing patterns** - Consistency with PVSnesLib makes code easier to understand
4. **Prevent subtle bugs** - Many SNES bugs come from misunderstanding hardware behavior

**Quick reference workflow:**
```bash
# Before implementing a feature:
1. Read relevant sections in SNES_HARDWARE_REFERENCE.md
2. Check SNES_ROSETTA_STONE.md for OpenSNES patterns
3. Look at PVSnesLib examples for working implementations
4. Then start coding
```

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

### HiROM Memory Layout (February 2026)

```
Bank $00-$3F: System area (same as LoROM)
  $0000-$1FFF: Mirror of $7E:0000 (WRAM low 8KB)
  $2000-$7FFF: I/O registers (PPU, CPU, DMA, etc.)
  $8000-$FFFF: ROM mirror (maps to Bank $40:$8000-$FFFF) ⚠️ CRITICAL

Bank $40-$7D: HiROM program space (64KB per bank)
  $0000-$7FFF: ROM (NOT accessible via Bank $00 mirror!)
  $8000-$FFFF: ROM (mirrored to Bank $00:$8000-$FFFF)

Bank $7E-$7F: Work RAM (128KB total, same as LoROM)
```

### ⚠️ CRITICAL: HiROM WLA-DX Configuration

**The Problem We Fixed (February 2026):**

In HiROM mode, the RESET vector stores a 16-bit address (e.g., `$80A7`). The CPU jumps to `$00:80A7`, which mirrors `$40:80A7`. This means:
- Code at address `$80A7` must be at **file offset `$80A7`** (not `$00A7`)
- The first 32KB of each bank (`$0000-$7FFF`) is NOT accessible via bank `$00`

**Wrong approach (causes black screen):**
```asm
; Using 32KB slots like LoROM - BROKEN for HiROM!
.MEMORYMAP
    SLOTSIZE $8000
    SLOT 0 $8000 $8000    ; Code at file offset $0000, but CPU expects $8000!
.ENDME
.ROMBANKSIZE $8000
```

**Correct approach:**
```asm
; Use 64KB banks, reserve $0000-$7FFF
.MEMORYMAP
    SLOTSIZE $10000         ; 64KB per slot (full HiROM bank)
    SLOT 0 $0000 $10000     ; Full 64KB bank
    SLOT 1 $0000 $2000      ; Work RAM
.ENDME
.ROMBANKSIZE $10000         ; 64KB per bank

; Reserve $0000-$7FFF so all code goes to $8000+
.BANK 0
.ORG 0
.SECTION ".hirom_reserved" FORCE
    .DSB $8000, $00         ; Fill with zeros (inaccessible via bank $00)
.ENDS

; Now code sections will be placed at $8000+
```

**Key insight:** In HiROM, file offset equals SNES address within the bank. If RESET points to `$80A7`, the code must be at file offset `$80A7`.

### HiROM vs LoROM Summary

| Aspect | LoROM | HiROM |
|--------|-------|-------|
| Bank size | 32KB ($8000-$FFFF) | 64KB ($0000-$FFFF) |
| Bank $00 ROM access | $8000-$FFFF | $8000-$FFFF only (mirrors $40) |
| WLA-DX SLOTSIZE | $8000 | $10000 |
| WLA-DX SLOT 0 start | $8000 | $0000 (but reserve $0000-$7FFF) |
| Code placement | Automatic at $8000+ | Must force to $8000+ |

### ✅ HiROM Function Return Value Issue — RESOLVED (February 2026)

**Problem (now fixed):** C library functions that return values (like `padHeld()`, `padPressed()`) previously didn't work correctly in HiROM mode. The return value appeared corrupted.

**Root Cause:** Same function epilogue bug that affected all non-void functions — the `tsa` instruction in the epilogue overwrote the return value in the A register with the stack pointer. This was NOT HiROM-specific.

**Fix:** The `tax`/`txa` pair in emit.c preserves A across the stack adjustment in the function epilogue. This fix resolved the issue for both LoROM and HiROM modes.

**Verified:** hirom_demo tested in Mesen2 — `padHeld(0)` returns correct values, A button changes background color as expected.

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

**Full documentation:** `.claude/WLA-DX.md` (local copy) or https://wla-dx.readthedocs.io/en/latest/

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

### Compile-Time Data Generation

WLA-DX provides powerful directives for generating data at compile time.

#### Trigonometric Data (`.DWSIN`, `.DBSIN`, `.DWCOS`, `.DBCOS`)

Generate sine/cosine wave tables without manual calculation:

```asm
; Syntax: .DWSIN start_angle, additional_count, increment, amplitude, offset
; Generates (additional_count + 1) 16-bit words

; 16-entry sine wave with amplitude 32
wave_table:
    .DWSIN 0, 15, 22.5, 32, 0
    ; Produces: 0, 12, 23, 29, 32, 29, 23, 12, 0, -12, -23, -29, -32, -29, -23, -12

; 8-bit version for smaller tables
.DBSIN 0, 31, 11.25, 127, 128    ; 32 bytes, centered at 128
```

**Parameters:**
- `start_angle`: Starting angle in degrees (0-359.999, can be float)
- `additional_count`: Number of additional values (total = count + 1)
- `increment`: Angle step per value (can be float)
- `amplitude`: Multiplier for sin/cos result (-1 to 1)
- `offset`: Added to each result

**Use cases:** HDMA wave effects, audio envelopes, smooth movement curves

#### Repetition (`.REPT` / `.REPEAT`)

Generate repetitive data or code patterns:

```asm
; Basic repetition
.REPT 16
    .db $FF
.ENDR

; With INDEX counter (0 to count-1)
.REPT 28 INDEX i
    .db 8                           ; Scanline count
    .dw DataTable + ((i & 15) * 2)  ; Cyclic pointer
.ENDR

; With custom START and STEP
.REPT 10 START 100 STEP -10 INDEX cnt
    .db cnt    ; Produces: 100, 90, 80, 70, 60, 50, 40, 30, 20, 10
.ENDR
```

**Use cases:** HDMA tables, lookup tables, repetitive data structures

#### User-Defined Functions (`.FUNCTION`)

Create compile-time calculation functions:

```asm
; Define functions
.FUNCTION SCALE(val, factor) (val * factor)
.FUNCTION CLAMP_BYTE(x) (x < 0 ? 0 : (x > 255 ? 255 : x))
.FUNCTION VRAM_ADDR(row, col) ((row * 32 + col) * 2)

; Use in code
.dw SCALE(100, 3)           ; 300
.db CLAMP_BYTE(-50)         ; 0
.dw VRAM_ADDR(5, 10)        ; 0x014A
```

#### Built-In Functions

WLA-DX provides many useful built-in functions:

| Function | Description |
|----------|-------------|
| `sin(x)`, `cos(x)` | Trigonometry (radians) |
| `abs(x)` | Absolute value |
| `min(a, b)`, `max(a, b)` | Min/max of two values |
| `clamp(val, min, max)` | Clamp value to range |
| `floor(x)`, `ceil(x)` | Rounding |
| `pow(base, exp)` | Exponentiation |
| `lobyte(x)`, `hibyte(x)` | Extract low/high byte |
| `loword(x)`, `hiword(x)` | Extract low/high word |
| `bank(label)` | Get bank number of label |
| `defined(sym)` | Check if symbol exists |

```asm
; Examples
.db lobyte($1234)           ; $34
.db hibyte($1234)           ; $12
.dw loword($123456)         ; $3456
.db bank(my_data)           ; Bank number
```

### Macros

Define reusable code blocks with parameters:

```asm
; Simple macro
.MACRO SET_REG ARGS reg, value
    lda #value
    sta reg
.ENDM

; Usage
SET_REG $2100, $0F          ; lda #$0F / sta $2100

; Macro with numbered args
.MACRO LOAD_WORD
    lda #<\1
    sta \2
    lda #>\1
    sta \2+1
.ENDM

; Unique labels with \@
.MACRO WAIT_VBLANK
@loop\@:
    lda $4212
    bpl @loop\@
.ENDM
```

### Conditional Assembly

```asm
.DEFINE DEBUG 1

.IF DEBUG == 1
    ; Debug code included
    jsr PrintDebugInfo
.ENDIF

.IFDEF FEATURE_AUDIO
    .INCLUDE "audio.asm"
.ENDIF
```

### C-to-Assembly Calling Convention (February 2026)

When writing assembly functions called from C, correct stack offset calculation is critical.

#### Stack Layout for C Function Calls

When C calls an assembly function via `JSL`:

1. C code pushes parameter(s) onto stack (2 bytes per parameter)
2. `JSL` pushes 3-byte return address (bank + 16-bit address)
3. Assembly function typically saves registers with `PHP` (1 byte) and `PHX`/`PHY` (2 bytes each)

**Example: Function with one u8 parameter**

```asm
myFunction:
    php                 ; Push P (1 byte)
    phx                 ; Push X (2 bytes)
    rep #$30            ; 16-bit A and X

    ; Stack layout from current S:
    ;   S+1, S+2: saved X (2 bytes)
    ;   S+3:      saved P (1 byte)
    ;   S+4, S+5, S+6: return address (3 bytes from JSL)
    ;   S+7, S+8: parameter (2 bytes from C's pha)
    ;
    ; Total offset to parameter: 2 + 1 + 3 = 6, so param at S+7

    lda 7,s             ; Load parameter (CORRECT)
    and #$00FF          ; Mask to byte if u8

    ; ... function body ...

    plx
    plp
    rtl
```

#### The Bug We Fixed (February 2026)

**Problem:** `hdmaWaveSetAmplitude()` used `lda 8,s` instead of `lda 7,s`.

**Symptom:** RIGHT button didn't change wave amplitude, while LEFT worked (coincidentally, since going from 1→0 may have matched garbage value).

**What happened:**
```asm
; WRONG - off by one!
lda 8,s             ; Reads S+8 and S+9 (high byte of param + garbage)

; CORRECT
lda 7,s             ; Reads S+7 and S+8 (low and high byte of param)
```

#### Stack Offset Calculation Formula

```
Offset = (saved registers size) + (return address size)

Where:
- PHP = 1 byte
- PHX/PHY = 2 bytes each (in 16-bit mode)
- PHA = 2 bytes (in 16-bit mode)
- JSL return address = 3 bytes
- JSR return address = 2 bytes
```

**Common patterns:**

| Prologue | Offset to 1st param |
|----------|---------------------|
| `php` only | 1 + 3 = 4 → `lda 5,s` |
| `php` + `phx` | 1 + 2 + 3 = 6 → `lda 7,s` |
| `php` + `phx` + `phy` | 1 + 2 + 2 + 3 = 8 → `lda 9,s` |
| `php` + `pha` (save A) | 1 + 2 + 3 = 6 → `lda 7,s` |

#### Multiple Parameters

Parameters are pushed right-to-left by C, so on stack they appear left-to-right:

```c
// C call: myFunc(a, b, c)
// Stack after JSL: [c] [b] [a] [return addr]
```

```asm
myFunc:
    php
    phx
    rep #$30
    ; Offset 7 = first param (a)
    ; Offset 9 = second param (b)
    ; Offset 11 = third param (c)
    lda 7,s             ; a
    lda 9,s             ; b
    lda 11,s            ; c
```

#### Debugging Tips

1. **Count bytes carefully** - Off-by-one errors are silent and deadly
2. **Test all code paths** - Bug may only manifest for certain parameter values
3. **Check generated assembly** - See how C pushes the parameter with `pha`
4. **Use Mesen2 debugger** - Watch stack pointer and memory during call

---

## 5. OAM and Sprites

**See also:** `.claude/PVSNESLIB_SPRITES.md` for comprehensive sprite documentation.

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

### Dynamic Sprite System (January 2026)

**Key Data Structures:**
- `oamMemory[]` (544 bytes): Hardware OAM shadow buffer
- `oambuffer[]` (128 × 16 bytes): Dynamic sprite state (t_sprites)
- `oamQueueEntry[]` (128 × 6 bytes): VRAM upload queue

**t_sprites structure (16 bytes):**
```c
typedef struct {
    s16 oamx;           // 0-1: X position
    s16 oamy;           // 2-3: Y position
    u16 oamframeid;     // 4-5: Animation frame
    u8 oamattribute;    // 6: vhoopppc (flip, priority, palette)
    u8 oamrefresh;      // 7: Set to 1 to upload graphics
    u16 oamgraphics;    // 8-9: Graphics address
    u8 oamgfxbank;      // 10: Graphics bank
    u8 _pad;            // 11: Padding
    u16 _reserved[2];   // 12-15: Alignment
} t_sprites;
```

**Frame Update Sequence (CRITICAL ORDER):**
1. Update positions/animation in `oambuffer[]`
2. Set `oamrefresh = 1` for changed sprites
3. Call `oamDynamic*Draw()` for each sprite
4. Call `oamInitDynamicSpriteEndFrame()`
5. Call `WaitForVBlank()`
6. Call `oamVramQueueUpdate()`

**Lookup Tables:**
- `lkup16oamS[]`: Source offsets in sprite sheet (by frameid)
- `lkup16idT[]`/`lkup16idT0[]`: OAM tile numbers
- `lkup16idB[]`: VRAM destination blocks

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

### Critical Pattern: Sprite Coordinates MUST Use Global Struct (February 2026)

**Problem:** When using separate `static u16` variables for sprite X/Y coordinates, horizontal movement (LEFT/RIGHT) fails while vertical movement (UP/DOWN) works correctly.

**Symptoms:**
- D-pad UP/DOWN moves sprite correctly
- D-pad LEFT/RIGHT has no effect
- No compiler errors or warnings
- Assembly output looks correct at first glance

**Root Cause:** A compiler quirk causes different (broken) code generation for separate `static u16` variable access compared to struct member access. The exact mechanism is unclear, but the pattern is reproducible.

**Broken Pattern (DO NOT USE):**
```c
// BAD - causes LEFT/RIGHT movement to fail!
static u16 player_x;
static u16 player_y;

// In main loop:
if (pad & KEY_LEFT) {
    if (player_x > 8) player_x -= 2;  // This doesn't work!
}
if (pad & KEY_UP) {
    if (player_y > 32) player_y -= 2;  // This works fine
}
oamSetXY(0, player_x, player_y);
```

**Working Pattern (REQUIRED):**
```c
// GOOD - all directions work correctly
typedef struct {
    s16 x, y;    // Use s16 (signed), not u16
    // ... other fields
} Player;

Player player = {100, 100};  // Global struct with initializer

// In main loop:
if (pad & KEY_LEFT) {
    if (player.x > 8) player.x -= 2;  // Works!
}
if (pad & KEY_UP) {
    if (player.y > 32) player.y -= 2;  // Works!
}
oamSet(0, player.x, player.y, tile, pal, prio, flags);
```

**Key Differences:**
1. Use `s16` instead of `u16` for coordinates
2. Use a global struct instead of separate static variables
3. Use `oamSet()` instead of `oamSetXY()`
4. Initialize struct at declaration (global initializer)

**Affected Examples:**
- `12_continuous_scroll` - Fixed February 2026
- `21_animated_sprite` - Reference implementation (working pattern)

**How to Diagnose:**
1. Check if UP/DOWN works but LEFT/RIGHT doesn't
2. Look for `static u16 player_x;` pattern
3. Refactor to global struct with `s16` members

**Why This Matters:**
This bug is subtle and time-consuming to debug because:
- The C code looks correct
- The assembly output appears correct on inspection
- Only horizontal movement fails (partial failure)
- No error messages or warnings

**See Also:**
- `examples/graphics/21_animated_sprite/main.c` - Working reference
- `examples/graphics/12_continuous_scroll/main.c` - Fixed example with documentation

---

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

# Verify example count (should be 19)
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
| Examples build | 100% (19/19) | 100% |
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

## 16. Example Architecture (January 2026)

### Ideal Pattern: C + Library DMA

The recommended example architecture uses **C code with library functions** for graphics loading, keeping ASM files data-only:

**main.c** - Uses library DMA functions:
```c
#include <snes.h>

extern u8 tiles[], tiles_end[];
extern u8 tilemap[], tilemap_end[];
extern u8 palette[], palette_end[];

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    // Configure tilemap location
    bgSetMapPtr(0, 0x0000, SC_32x32);

    // Load tiles and palette via DMA
    bgInitTileSet(0, tiles, palette, 0,
                  tiles_end - tiles,
                  palette_end - palette,
                  BG_16COLORS, 0x4000);

    // Load tilemap via DMA
    dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);

    // Configure video mode
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1;
    bgSetScroll(0, 0, 0);

    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) { WaitForVBlank(); }
}
```

**data.asm** - Data definitions only (no DMA code):
```asm
.section ".rodata1" superfree
tiles: .incbin "res/tiles.pic"
tiles_end:
tilemap: .incbin "res/tiles.map"
tilemap_end:
palette: .incbin "res/tiles.pal"
palette_end:
.ends
```

**Makefile** - Include required library modules:
```makefile
LIB_MODULES := console sprite dma background
```

### Example Classification (January 2026)

| Example | Architecture | Notes |
|---------|--------------|-------|
| `5_mode1` | C + Library DMA | Ideal pattern |
| `6_simple_sprite` | C + Library DMA | Uses `oamInitGfxSet()` |
| `7_fading` | C + Library DMA | Ideal pattern |
| `11_mode5` | C + Library DMA | Hi-res mode |
| `4_scrolling` | C + Library DMA | 2 backgrounds |
| `12_continuous_scroll` | C + Library DMA | VBlank callback demo |
| `6_snesmod_music` | C + Library DMA | SNESMOD example |
| `7_snesmod_sfx` | C + Library DMA | SNESMOD example |
| `3_mode7` | C + ASM DMA | Mode 7 interleaved VRAM (library doesn't support) |
| `8_mode0` | C + ASM DMA | 4-BG parallax (educational for Mode 0) |
| `9_parallax` | C + ASM DMA | Parallax scrolling demo |
| `10_mode3` | C + ASM DMA | Split DMA for >32KB tiles |
| `2_animation` | C + ASM DMA | Sprite animation |
| `1_tone` | C + ASM | Low-level SPC700 demo |
| `2_sfx` | C + Library | Legacy audio |
| `2_custom_font` | Standalone | Font rendering demo |
| `1_calculator` | Standalone | No graphics, logic demo |
| `1_hello_world` | C + Library | Text console |
| `2_two_players` | C + Library | Input demo |

### When to Keep ASM

ASM DMA code is appropriate when:

1. **Mode 7**: Interleaved VRAM layout (tilemap in low bytes, characters in high bytes) - library doesn't support
2. **Split DMA**: Tile data >32KB requires multiple DMA transfers from different banks
3. **Timing-critical**: Parallax scrolling with per-scanline updates
4. **Educational**: Showing how hardware registers work at low level

### Converting Examples to Library DMA

To convert an example from ASM DMA to library DMA:

1. **Update main.c**:
   - Remove `extern void load_graphics(void);`
   - Add extern declarations for data: `extern u8 tiles[], tiles_end[];`
   - Replace `load_graphics()` with library calls:
     - `bgInitTileSet()` for tiles + palette
     - `dmaCopyVram()` for tilemap
     - `oamInitGfxSet()` for sprite graphics

2. **Update data.asm**:
   - Remove the `load_graphics:` function and all DMA code
   - Keep only `.incbin` directives with labels and `_end` labels

3. **Update Makefile**:
   - Add `background` to `LIB_MODULES` if using `bgInitTileSet()`, `bgSetMapPtr()`

4. **Build and validate**:
   ```bash
   make clean && make
   python3 tools/symmap/symmap.py --check-overlap game.sym
   ```

### Library DMA Functions Reference

| Function | Purpose | Notes |
|----------|---------|-------|
| `bgInitTileSet()` | Load tiles + palette, set gfx pointer | Handles 24-bit ROM addresses |
| `dmaCopyVram()` | Copy data to VRAM | For tilemaps, extra tiles |
| `dmaCopyCGram()` | Copy palette data | For additional palettes |
| `oamInitGfxSet()` | Load sprite tiles + palette | Also sets OBJSEL |
| `bgSetMapPtr()` | Configure tilemap address/size | Sets BGxSC register |
| `bgSetGfxPtr()` | Configure tile graphics address | Sets BG12NBA/BG34NBA |
| `bgSetScroll()` | Set scroll position | Shadow registers for write-only regs |

---

## 17. VBlank Timing Constraints (January 2026)

### Overview

VBlank is the only safe time to update VRAM, OAM, and CGRAM. Understanding timing limits is critical for stable games.

### VBlank Duration

| Region | VBlank Lines | VBlank Time | Safe DMA |
|--------|--------------|-------------|----------|
| NTSC | ~37 lines | ~2,273 µs | ~6KB |
| PAL | ~70 lines | ~4,300 µs | ~12KB |

**Practical recommendation:** Limit DMA to **~4KB per frame** to leave headroom for:
- OAM transfer (544 bytes)
- Scroll register updates
- Palette updates
- User VBlank callback code

### DMA Transfer Rates

```
DMA speed: 2.68 MB/second (Bus A to Bus B)
VBlank NTSC: ~2,273 µs available
Max theoretical: 2.68 × 2.273ms ≈ 6KB

But you need time for:
- NMI handler overhead (~50 cycles)
- OAM DMA (544 bytes → ~200 µs)
- Scroll/palette writes (~50 µs)
- User callback code (variable)

Safe budget: 4KB DMA + 544 OAM + misc = ~5KB total
```

### What Happens If You Exceed VBlank

If DMA isn't complete before rendering starts:
- **VRAM writes**: Corrupted graphics, visual glitches
- **OAM writes**: Sprites in wrong positions or invisible
- **CGRAM writes**: Wrong colors, color flashing

### Strategies for Large Transfers

**1. Spread across frames:**
```c
// Load 16KB tileset over 4 frames
static u8 load_phase = 0;
void loadTilesetAsync(void) {
    switch (load_phase) {
        case 0: dmaCopyVram(tiles,      0x0000, 4096); break;
        case 1: dmaCopyVram(tiles+4096, 0x0800, 4096); break;
        case 2: dmaCopyVram(tiles+8192, 0x1000, 4096); break;
        case 3: dmaCopyVram(tiles+12288,0x1800, 4096); break;
    }
    load_phase++;
}
```

**2. Load during forced blank:**
```c
// Disable rendering for large loads (screen goes black)
REG_INIDISP = INIDISP_FORCE_BLANK;
dmaCopyVram(large_tileset, 0x0000, 32768);  // 32KB - no time limit!
REG_INIDISP = INIDISP_BRIGHTNESS(15);
```

**3. Stream during gameplay:**
```c
// Update only changed tiles each frame
#define TILES_PER_FRAME 8  // 8 tiles × 32 bytes = 256 bytes
void streamTileUpdates(void) {
    if (dirty_tile_count > 0) {
        u8 count = (dirty_tile_count > TILES_PER_FRAME) ?
                   TILES_PER_FRAME : dirty_tile_count;
        // DMA only the changed tiles
        dmaCopyVram(dirty_tiles, dirty_vram_addr, count * 32);
        dirty_tile_count -= count;
    }
}
```

### Animated Sprite Considerations

From Wikibooks: "DMA isn't all that fast - you only have time to DMA up to 6kB during v-blank, and even less if you take into account scrolling and OAM updating. For practical purposes, about 4kB is recommended."

For sprite animation, consider:
- Pre-load all animation frames to VRAM at level start
- Use tile index changes (OAM only, no VRAM DMA needed)
- If you must stream: limit to 4KB/frame

---

## 18. HDMA (Horizontal DMA) (January 2026)

### Overview

HDMA transfers small amounts of data **during each H-blank** (between scanlines). This enables per-scanline effects impossible with regular DMA.

### HDMA vs Regular DMA

| Feature | Regular DMA | HDMA |
|---------|-------------|------|
| When | VBlank only | Every H-blank |
| Data per transfer | Up to 64KB | 1-4 bytes |
| Use case | Bulk loading | Per-scanline effects |
| Channels | 0-7 | 0-7 (shared with DMA) |

### HDMA Transfer Modes

| Mode | Bytes/Scanline | Pattern | Use Case |
|------|----------------|---------|----------|
| 0 | 1 byte | `a` | Single register (scroll, mosaic) |
| 1 | 2 bytes | `a, a+1` | Word to consecutive registers |
| 2 | 2 bytes | `a, a` | Two writes to same register |
| 3 | 4 bytes | `a, a, a+1, a+1` | Two words alternating |
| 4 | 4 bytes | `a, a+1, a, a+1` | Two words to consecutive |

### Common HDMA Effects

**1. Color Gradient (Sky/Water):**
```c
// Table: scanline count, then RGB values
static const u8 sky_gradient[] = {
    32,  0x00, 0x40,  // 32 lines of dark blue
    32,  0x00, 0x50,  // 32 lines of medium blue
    32,  0x00, 0x60,  // 32 lines of light blue
    0                 // End marker
};
// Target: CGRAM address $00 (backdrop color)
```

**2. Parallax Scrolling:**
```c
// Different scroll values per screen section
// CRITICAL: Must use REPEAT mode (bit 7 = 1) for scroll registers!
static const u8 parallax_table[] = {
    0xC0, 0x00, 0x00,   // Top 64 lines: no scroll ($80 | 64 = $C0)
    0xC0, 0x40, 0x00,   // Next 64: scroll 64px
    0xC0, 0x80, 0x00,   // Next 64: scroll 128px
    0xA0, 0xC0, 0x00,   // Bottom 32: scroll 192px ($80 | 32 = $A0)
    0
};
// Target: BG1HOFS ($210D)
```

### CRITICAL: Scroll Registers Require REPEAT Mode (February 2026)

**Problem:** HDMA to scroll registers (BG1HOFS, BG1VOFS, etc.) appears to do nothing when using direct mode (bit 7 = 0).

**Root Cause:** Scroll registers require the value to be written on **every scanline**, not just once at the start of each group. Direct mode writes once then skips; repeat mode writes every scanline.

**Solution:** Always use REPEAT mode (set bit 7) for scroll HDMA tables:

```asm
; WRONG - Direct mode (bit 7 = 0) - DOES NOT WORK for scroll!
.db 32, $20, $00     ; This won't produce any visible scroll effect

; CORRECT - Repeat mode (bit 7 = 1) - WORKS for scroll
.db $A0, $20, $00    ; $A0 = $80 | 32 = repeat for 32 lines
```

**Symptoms of this bug:**
- HDMA to brightness ($2100) works fine
- HDMA to scroll ($210D) shows no effect
- Vertical stripes remain perfectly straight instead of staircase pattern

**Why this happens:** Write-twice registers like BGxHOFS may need continuous writes to maintain their latched state, or the PPU may only sample scroll values when they're actively being written.

**3. Wavy Water Effect:**
```c
// Sine wave offset table (256 entries for smooth wave)
static u8 wave_table[256];
void initWaveTable(void) {
    for (int i = 0; i < 256; i++) {
        wave_table[i] = (fixSin(i) >> 5) + 128;
    }
}
// Animate by rotating table start each frame
```

**4. Window/Spotlight:**
```c
// Shrink window per scanline for spotlight
static const u8 spotlight[] = {
    1, 128, 128,  // Line 0: window at center (closed)
    1, 120, 136,  // Line 1: slightly open
    1, 112, 144,  // Line 2: more open
    // ... continues expanding then contracting
    0
};
// Target: WH0/WH1 ($2126/$2127)
```

### HDMA Registers

| Register | Purpose |
|----------|---------|
| $43x0 | HDMA control (same as DMA, bit 6 = HDMA enable) |
| $43x1 | B-bus destination |
| $43x2-3 | Table address (A-bus) |
| $43x4 | Table bank |
| $43x5-6 | Indirect address (for indirect mode) |
| $43x7 | Indirect bank |
| $43x8-9 | Current table address (read-only) |
| $43xA | Line counter (read-only) |
| $420C | HDMA enable (bit per channel) |

### HDMA Table Format

**Direct mode:**
```
[count] [data...]    ; count > 0: repeat data for 'count' lines
[count] [data...]    ; count = 0: end of table
[0x80|count] [data...] ; count | 0x80: continuous mode (new data each line)
```

**Indirect mode:**
```
[count] [pointer_lo] [pointer_hi]  ; Points to data elsewhere
```

### HDMA Limitations

1. **Channels shared with DMA** - Don't use same channel for both
2. **Limited bytes** - Only 1-4 bytes per scanline
3. **Table in ROM/WRAM** - Must be accessible during rendering
4. **CPU overhead** - Each active channel costs ~18 cycles/scanline

### Best Practices

```c
// Reserve channels: 0-5 for DMA, 6-7 for HDMA
#define HDMA_CHANNEL_GRADIENT 6
#define HDMA_CHANNEL_SCROLL   7

// Disable HDMA before modifying tables
REG_HDMAEN = 0;
updateGradientTable();
REG_HDMAEN = (1 << HDMA_CHANNEL_GRADIENT);
```

### HDMA Toggle Functions Must Be In Separate Assembly Section (February 2026)

**Problem:** When implementing runtime HDMA toggle (enable/disable via button), the display shows erratic animation even when toggle functions are never called.

**Root Cause:** Having multiple `sta.l $420C` instructions in the **same assembly section** as the HDMA table causes undefined behavior. This appears to be a WLA-DX linker quirk where code/data placement affects HDMA operation.

**Solution:** Place toggle functions in a **separate assembly file or section**:

```asm
;=== File: hdma_table.asm ===
.SECTION ".hdma_data" SUPERFREE
hdma_scroll_table:
    .db $A0, $00, $00
    ; ... table entries ...
    .db 0

setup_hdma:
    ; ... setup code with ONE sta.l $420C ...
    rtl
.ENDS

;=== File: hdma_toggle.asm (SEPARATE FILE!) ===
.SECTION ".hdma_toggle" SUPERFREE
do_hdma_toggle:
    ; ... toggle code with sta.l $420C ...
    rtl
.ENDS
```

**Symptoms of this bug:**
- Static HDMA effect works perfectly (setup only)
- Adding toggle functions causes animation/vibration on ROM open
- Animation happens even when toggle functions are never called
- Replacing `sta.l $420C` with dummy RAM writes makes it static again

### Input Handling with HDMA (February 2026)

**Problem:** `padPressed()` library function may not work reliably when combined with HDMA effects.

**Solution:** Use direct controller register reads with manual edge detection:

```c
u16 pad, pad_old = 0;

while (1) {
    WaitForVBlank();

    /* Wait for auto-joypad read to complete */
    while (REG_HVBJOY & 0x01) {}

    /* Read controller directly */
    pad = REG_JOY1L | (REG_JOY1H << 8);

    /* Edge detection for button press */
    if ((pad & KEY_A) && !(pad_old & KEY_A)) {
        do_hdma_toggle();
    }

    pad_old = pad;
}
```

This pattern matches the working input examples and ensures reliable button detection.

---

## 19. Background Modes (January 2026)

### Mode Overview

The SNES has 8 background modes (0-7). Mode is set via register `$2105` (BGMODE).

| Mode | BG1 | BG2 | BG3 | BG4 | Colors | Common Use |
|------|-----|-----|-----|-----|--------|------------|
| **0** | 2bpp | 2bpp | 2bpp | 2bpp | 4×4 = 16 | Status bars, 4-layer parallax |
| **1** | 4bpp | 4bpp | 2bpp | - | 16+16+4 | **Most common** (SMW, SF2) |
| **2** | 4bpp | 4bpp | OPT | - | 16+16 | Offset-per-tile scrolling |
| **3** | 8bpp | 4bpp | - | - | 256+16 | Photorealistic BG + UI |
| **4** | 8bpp | 2bpp | OPT | - | 256+4 | 256-color + simple overlay |
| **5** | 4bpp | 2bpp | - | - | 16+4 | Hi-res (512×224) |
| **6** | 4bpp | - | OPT | - | 16 | Hi-res + offset-per-tile |
| **7** | 8bpp | - | - | - | 256 | Rotation/scaling (F-Zero) |

### Mode 1 (Most Common)

```
BG1: 4bpp (16 colors) - Foreground/playfield
BG2: 4bpp (16 colors) - Background scenery
BG3: 2bpp (4 colors)  - Status bar/HUD overlay

Total: 36 colors from 256 palette
```

**Typical usage in platformers:**
- BG1: Interactive level tiles (player walks on)
- BG2: Parallax background (mountains, clouds)
- BG3: Score, lives, timer display

**Example setup:**
```c
setMode(BG_MODE1, 0);
REG_TM = TM_BG1 | TM_BG2 | TM_BG3;  // Enable all 3 layers
REG_BG3PRIO = 1;  // BG3 highest priority (in front)
```

### Mode 0 (4 Layers)

```
BG1-BG4: All 2bpp (4 colors each from different palettes)
Total: 16 colors (4 per layer)
```

**Use case:** Complex parallax with many layers
- Contra III uses Mode 0 for some stages
- Good for "behind the scenes" depth effects

### Mode 3 (256 Colors)

```
BG1: 8bpp (256 colors) - Full palette access
BG2: 4bpp (16 colors)  - Overlay/UI
```

**Use case:** Photo-realistic backgrounds, pre-rendered art

### Mode 7 (Rotation/Scaling)

```
BG1: 8bpp (256 colors), affine transformation
- Single 128×128 or 256×256 tile map
- Hardware rotation, scaling, perspective
- Used for: F-Zero, Mario Kart, Pilotwings
```

**Mode 7 VRAM layout (interleaved):**
```
$0000: Tilemap byte 0, Tile 0 byte 0
$0001: Tilemap byte 1, Tile 0 byte 1
$0002: Tilemap byte 2, Tile 0 byte 2
...
```

**Mode 7 registers:**
| Register | Purpose |
|----------|---------|
| $211A | Mode 7 settings (flip, wrap) |
| $211B-C | Matrix A (cos θ × scale) |
| $211D-E | Matrix B (sin θ × scale) |
| $211F-20 | Matrix C (-sin θ × scale) |
| $2121-22 | Matrix D (cos θ × scale) |
| $211F-20 | Center X |
| $2121-22 | Center Y |

### Hi-Res Modes (5, 6)

```
Resolution: 512×224 (vs normal 256×224)
- Pixels are half-width
- Text looks sharper
- Used for: RPG menus, visual novels
```

**Limitation:** 16+4 colors only, no Mode 7

### Tile Size

All modes support 8×8 or 16×16 tiles per layer (set in BGMODE register):

```c
// 8x8 tiles (default)
setMode(BG_MODE1, 0);

// 16x16 tiles for BG1
setMode(BG_MODE1, BG1_TILE_16x16);

// 16x16 for BG1 and BG2
setMode(BG_MODE1, BG1_TILE_16x16 | BG2_TILE_16x16);
```

### Priority System

Each BG has 2 priority levels (set in tilemap). Sprites also have 4 priority levels.

**Default front-to-back order (Mode 1):**
```
1. Sprites (priority 3)
2. BG1 tiles (priority 1)
3. Sprites (priority 2)
4. BG2 tiles (priority 1)
5. Sprites (priority 1)
6. BG1 tiles (priority 0)
7. Sprites (priority 0)
8. BG2 tiles (priority 0)
9. BG3 tiles (priority 1) - if BG3PRIO=1
10. BG3 tiles (priority 0)
```

---

## 20. Color Format - BGR555 (January 2026)

### SNES Color Format

The SNES uses **15-bit BGR555** color:

```
Bit:  15   14-10    9-5     4-0
      0    BBBBB   GGGGG   RRRRR

      (bit 15 is always 0)
```

Each channel (R, G, B) has **5 bits = 32 levels** (0-31).

### Total Colors

- **Palette capacity:** 256 entries (512 bytes in CGRAM)
- **Colors per entry:** 32,768 possible (2^15)
- **Max on-screen:** 256 (from palette) but up to 32,768 using color math

### Color Calculation

```c
// Create BGR555 color from RGB values (0-31 each)
#define RGB555(r, g, b) ((b << 10) | (g << 5) | r)

// Examples:
#define COLOR_BLACK   RGB555(0,  0,  0)   // 0x0000
#define COLOR_WHITE   RGB555(31, 31, 31)  // 0x7FFF
#define COLOR_RED     RGB555(31, 0,  0)   // 0x001F
#define COLOR_GREEN   RGB555(0,  31, 0)   // 0x03E0
#define COLOR_BLUE    RGB555(0,  0,  31)  // 0x7C00
#define COLOR_YELLOW  RGB555(31, 31, 0)   // 0x03FF
#define COLOR_CYAN    RGB555(0,  31, 31)  // 0x7FE0
#define COLOR_MAGENTA RGB555(31, 0,  31)  // 0x7C1F
```

### CGRAM Layout

```
$00-$1F:  BG palette 0 (32 bytes = 16 colors)
$20-$3F:  BG palette 1
$40-$5F:  BG palette 2
$60-$7F:  BG palette 3
$80-$9F:  BG palette 4
$A0-$BF:  BG palette 5
$C0-$DF:  BG palette 6
$E0-$FF:  BG palette 7
$100-$11F: Sprite palette 0
$120-$13F: Sprite palette 1
$140-$15F: Sprite palette 2
$160-$17F: Sprite palette 3
$180-$19F: Sprite palette 4
$1A0-$1BF: Sprite palette 5
$1C0-$1DF: Sprite palette 6
$1E0-$1FF: Sprite palette 7
```

**Note:** Color 0 of each BG palette is transparent (shows backdrop). Color 0 of sprite palettes is also transparent.

### Backdrop Color

Address `$00` in CGRAM is the **backdrop** - visible where no BG or sprite covers it.

```c
// Set backdrop to dark blue
REG_CGADD = 0;
REG_CGDATA = RGB555(0, 0, 16) & 0xFF;
REG_CGDATA = RGB555(0, 0, 16) >> 8;
```

### Color Math (Transparency Effects)

The SNES can blend colors mathematically for transparency:

| Operation | Effect |
|-----------|--------|
| Add | Brighten (fire, glow) |
| Subtract | Darken (shadows) |
| Add/2 | 50% transparency |

**Registers:**
- `$2130` (CGWSEL): Color math enable/window
- `$2131` (CGADSUB): Add/subtract select per layer
- `$2132` (COLDATA): Fixed color for math

```c
// 50% transparent BG2 over BG1
REG_CGWSEL = 0x02;  // Apply to BG2
REG_CGADSUB = 0x22; // Add, half, BG2
```

### Converting from 24-bit RGB

When converting from standard 24-bit RGB (0-255 per channel):

```c
// 24-bit to BGR555
u16 rgb24_to_bgr555(u8 r, u8 g, u8 b) {
    return ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
}

// Example: Orange (255, 128, 0) in 24-bit
// → (31, 16, 0) in 5-bit
// → BGR555: 0x0000 | 0x0200 | 0x001F = 0x021F
```

### Palette Organization Tips

1. **Color 0 = transparent** - Don't put important colors there
2. **Share palettes** - Enemies with similar colors can share
3. **Reserve backdrop** - Plan for it (usually sky/water color)
4. **Gradient optimization** - Place gradient colors consecutively for HDMA

```c
// Example: 16-color palette for grass tileset
const u16 grass_palette[] = {
    RGB555(0, 0, 0),    // 0: Transparent
    RGB555(8, 20, 4),   // 1: Dark grass
    RGB555(12, 24, 8),  // 2: Medium grass
    RGB555(16, 28, 12), // 3: Light grass
    RGB555(4, 8, 2),    // 4: Shadow
    RGB555(20, 16, 8),  // 5: Dirt dark
    RGB555(24, 20, 12), // 6: Dirt light
    RGB555(28, 24, 16), // 7: Sand
    // ... etc
};
```

---

## 21. VRAM Tilemap Overlap and DMA Timing (January 2026)

### The Problem

When multiple background tilemaps share overlapping VRAM regions, splitting DMA uploads across VBlanks causes **visual corruption** during the intermediate frame.

### Case Study: Breakout Pink Border Bug

**Symptom:** Pink border appeared during level transitions, then disappeared.

**VRAM Layout:**
```
BG1 tilemap: 0x0000-0x07FF (2KB)
BG3 tilemap: 0x0400-0x0BFF (2KB)
Overlap:     0x0400-0x07FF (1KB shared)
```

**Broken Code (Split DMAs):**
```c
WaitForVBlank();
dmaCopyCGram((u8 *)pal, 0, 512);
dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);  // Writes 0x0000-0x07FF
WaitForVBlank();  // <<< FRAME RENDERED WITH CORRUPT BG3!
dmaCopyVram((u8 *)backmap, 0x0400, 0x800);   // Writes 0x0400-0x0BFF
```

**What Happened:**
1. First VBlank: BG1 tilemap uploaded to 0x0000-0x07FF
2. Frame rendered: BG3 reads from 0x0400-0x0BFF
   - 0x0400-0x07FF contains BG1 blockmap data (wrong!)
   - 0x0800-0x0BFF contains old/garbage data
3. Pink showed through transparent border pixels from corrupt BG3

**Fixed Code (Atomic DMAs):**
```c
WaitForVBlank();
dmaCopyCGram((u8 *)pal, 0, 512);
dmaCopyVram((u8 *)blockmap, 0x0000, 0x800);
dmaCopyVram((u8 *)backmap, 0x0400, 0x800);  // All in same VBlank!
```

### The Rule

**When VRAM regions overlap, ALL overlapping DMAs must complete in the SAME VBlank.**

This is more important than staying under the "safe" 4KB DMA budget. PVSnesLib routinely does 4.5KB+ in a single VBlank for this reason.

### Why Overlap Exists

Overlapping tilemaps save VRAM by sharing unused portions:
- BG1 uses rows 0-15 (0x0000-0x03FF)
- BG3 uses rows 16-31 of BG1's space + its own rows 0-15 (0x0400-0x0BFF)
- Net savings: 1KB VRAM

This is intentional and works fine when DMAs are atomic.

### Detecting Overlap Issues

**Symptoms:**
- Brief color corruption during screen transitions
- Garbage appearing for 1 frame then disappearing
- Effects only visible during level loads or screen changes

**Debug Steps:**
1. Check VRAM layout in code/documentation
2. Look for `WaitForVBlank()` calls between related DMA operations
3. Compare against PVSnesLib reference - it does atomic DMAs

### General Principles

1. **Reference implementations matter more than theory** - PVSnesLib's DMA timing is battle-tested
2. **Don't over-optimize DMA budget** - Splitting for "safety" introduced the bug
3. **Transparency creates layer interactions** - Corrupt lower layers show through
4. **Trace exact frame sequence** when debugging visual glitches

---

## 22. Bug Fix Verification Protocol (January 2026)

### Lesson Learned

**Always verify a bug still exists before implementing a fix.**

We wasted significant time planning and analyzing a "for-loop compiler bug" that had already been fixed. The plan was based on analyzing a stale `.asm` file generated with an older compiler version.

### The Protocol

**Before implementing ANY bug fix:**

1. **Reproduce the bug with current code first**
   ```bash
   # Quick compile test for compiler bugs
   echo "test code" | cc -E -x c - | ./compiler/cproc/cproc-qbe | ./compiler/qbe/qbe -t w65816
   ```

2. **Don't trust old generated files** - Regenerate `.asm`, `.obj`, `.sym` files before analysis

3. **Check git history** for recent fixes to the affected code:
   ```bash
   git log --oneline -10 -- path/to/file.c
   ```

4. **Build and test the current code** before diving into implementation

### Red Flags That a Bug May Be Fixed

- The "buggy" file is old (check timestamps)
- Current builds work but old artifacts show problems
- Git history shows recent changes to related code
- Workarounds exist in code but might no longer be needed

### Time Cost

Skipping verification can waste hours on non-existent bugs. A 30-second test at the start saves significant time.

---

*This knowledge base is continuously updated with learnings from development and debugging sessions.*

---

## 23. CI Pipeline Masking Failures (February 2026)

### The Catastrophic Discovery

**The CI has been reporting "success" for failing builds since its creation.**

### Root Cause: Shell Pipe Exit Codes

```yaml
# This LOOKS correct but is BROKEN:
- name: Build tests
  run: |
    make tests 2>&1 | tee -a log.txt
```

**What actually happens:**
```
make tests    → exits with code 2 (FAILURE)
      ↓
tee log.txt   → exits with code 0 (SUCCESS)
      ↓
GitHub sees   → exit code 0 = step passed ✓
```

The pipe operator `|` returns the exit code of the LAST command, not the first. Since `tee` always succeeds, **every failure was hidden**.

### Impact

- Compiler bugs went undetected for months
- Memory corruption bugs existed but validation "passed"
- Examples that failed to build showed as "success"
- Windows "failed" only because MSYS2 behaves differently with pipes

### The Fix

**ALWAYS use `set -o pipefail` in shell scripts:**
```yaml
- name: Build tests
  run: |
    set -o pipefail  # ← MANDATORY
    make tests 2>&1 | tee -a log.txt
```

### Verification Rule

**NEVER trust CI green status.** Always run locally:
```bash
make clean && make && make tests && make examples
./tests/compiler/run_tests.sh
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

---

## 24. QBE Compiler Static Buffer Bug (February 2026)

### The Bug

Struct initializers with pointer members generated wrong assembly:

```c
u8 frames[] = {1, 2, 3};
Animation anim = { .frames = frames };  // pointer to frames

// GENERATED (WRONG):
.dl z+0      // ← "z" instead of "frames"!

// EXPECTED:
.dl frames+0
```

### Root Cause

In `compiler/qbe/emit.c`, reference names were stored as pointers to a static buffer (`tokval.str`) that gets overwritten by subsequent tokens:

```c
// BROKEN:
init_items[init_count].ref.name = d->u.ref.name;  // pointer to static buffer

// When parsing "l $frames, b 3, z 7":
// 1. Parse "$frames" → buffer = "frames"
// 2. Store POINTER to buffer
// 3. Parse "z" → buffer = "z" (OVERWRITES!)
// 4. Emit → outputs "z" instead of "frames"
```

### The Fix

Copy the string instead of storing a pointer:

```c
// FIXED:
size_t len = strlen(d->u.ref.name) + 1;
char *name_copy = emalloc(len);
memcpy(name_copy, d->u.ref.name, len);
init_items[init_count].ref.name = name_copy;
```

### Lesson: Static Buffers in Parsers

**NEVER store pointers to parser token buffers.** They are reused and will be overwritten. Always copy data you need to keep.

### Detection Pattern

If generated assembly contains single-letter symbols like `z`, `b`, `l`, `s`, `d` where variable names should be, suspect this class of bug.

---

## 25. WRAM Mirror Memory Corruption (February 2026)

### The Bug

Bank $00 compiler-generated variables collided with Bank $7E system buffers at the same addresses, causing silent memory corruption.

### The Hardware Reality

```
SNES Memory Map (first 8KB of WRAM):
  Bank $00: $0000-$1FFF  ←──┐
                            ├── SAME PHYSICAL 8KB RAM
  Bank $7E: $0000-$1FFF  ←──┘

Writing to $00:0300 ALSO writes to $7E:0300!
```

### What Was Happening

```
Bank $00 (C variables):          Bank $7E (system buffers):
  $0300: player_x                  $0300: oamMemory ← COLLISION!
  $0520: enemy_data                $0520: oambuffer ← COLLISION!
```

At runtime:
```c
player_x = 100;        // Writes to $00:0300
// This ALSO corrupts oamMemory[0] at $7E:0300!
// Sprites display at wrong positions
```

### The Fix

1. **Move Bank $7E buffers above mirror range ($2000+)**
   ```asm
   ; BEFORE (dangerous):
   .RAMSECTION ".buffer" BANK $7E SLOT 1 ORGA $0520 FORCE
   
   ; AFTER (safe):
   .RAMSECTION ".buffer" BANK $7E SLOT 2 ORGA $2000 FORCE
   ```

2. **Reserve collision zone in Bank $00**
   ```asm
   .RAMSECTION ".reserved_7e_mirror" BANK 0 SLOT 1 ORGA $0300 FORCE
       _reserved_7e_mirror dsb 544  ; Prevents linker from using this space
   .ENDS
   ```

3. **Update symmap.py to ignore reserved symbols**

### Detection

```bash
python3 tools/symmap/symmap.py --check-overlap game.sym
```

If it shows "COLLISION" with addresses in $0000-$1FFF range, you have this bug.

### Safe Memory Layout

```
Bank $00 (SLOT 1: $0000-$1FFF):
  $0000-$02FF  System variables, compiler variables (SAFE)
  $0300-$051F  RESERVED (mirrors oamMemory)
  $0520-$1FFF  Compiler variables continue (SAFE after reservation)

Bank $7E:
  $0300-$051F  oamMemory (in mirror range, but protected)
  $2000+       All other buffers (SAFE, above mirror range)
```

---

*This knowledge base is continuously updated with learnings from development and debugging sessions.*
