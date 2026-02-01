# 65816 Assembly Programming Guide

> Based on "Programming the 65816" by David Eyes and Ron Lichty (1986)
>
> Adapted for OpenSNES with SNES-specific annotations

This guide covers 65816 assembly programming fundamentals. For SNES hardware details (PPU, DMA, registers), see `SNES_HARDWARE_REFERENCE.md`.

> **SNES Note**: Sections marked with this callout highlight where SNES behavior differs from generic 65816.

---

## Table of Contents

1. [The 65x Family Overview](#1-the-65x-family-overview)
2. [Number Systems and Data Representation](#2-number-systems-and-data-representation)
3. [65816 Architecture](#3-65816-architecture)
4. [Addressing Modes](#4-addressing-modes)
5. [Instruction Set](#5-instruction-set)
6. [Programming Techniques](#6-programming-techniques)
7. [Interrupts and System Control](#7-interrupts-and-system-control)
8. [SNES-Specific Patterns](#8-snes-specific-patterns)
9. [Code Examples](#9-code-examples)
10. [Cycle Counting](#10-cycle-counting)

---

## 1. The 65x Family Overview

### 1.1 Family Tree

| Processor | Year | Bits | Address Space | Key Features |
|-----------|------|------|---------------|--------------|
| **6502** | 1975 | 8-bit | 64 KB | Original design, simple and fast |
| **65C02** | 1983 | 8-bit | 64 KB | CMOS, new instructions, bug fixes |
| **65802** | 1985 | 8/16-bit | 64 KB | 65816 in 6502 pin-compatible package |
| **65816** | 1985 | 8/16-bit | 16 MB | Full 24-bit addressing |

> **SNES Note**: The SNES uses a **Ricoh 5A22**, which is a 65816 variant running at 3.58 MHz (fast) / 2.68 MHz (slow) / 1.79 MHz (extra slow) depending on memory region accessed.

### 1.2 Design Philosophy

The 65x processors emphasize:
- **Pipelining**: Overlapping fetch and execute operations
- **Minimal clock cycles**: Most operations complete in 2-6 cycles
- **Orthogonal addressing**: Many modes available for most instructions
- **Direct page**: Fast access to 256 bytes (relocatable on 65816)
- **Stack operations**: Hardware stack support

---

## 2. Number Systems and Data Representation

### 2.1 Data Sizes

| Term | Size | Range (Unsigned) | Range (Signed) |
|------|------|------------------|----------------|
| Byte | 8 bits | 0 to 255 | -128 to +127 |
| Word | 16 bits | 0 to 65,535 | -32,768 to +32,767 |
| Long | 24 bits | 0 to 16,777,215 | -8,388,608 to +8,388,607 |

### 2.2 Memory Order (Little-Endian)

The 65816 stores multi-byte values with the **low byte first**:

```
Value $1234 stored at address $1000:
  $1000: $34  (low byte)
  $1001: $12  (high byte)

Value $123456 stored at address $2000:
  $2000: $56  (low byte)
  $2001: $34  (middle byte)
  $2002: $12  (high byte / bank)
```

### 2.3 Signed Numbers (Two's Complement)

```
Positive: 0xxxxxxx (bit 7 = 0)
Negative: 1xxxxxxx (bit 7 = 1)

To negate: Invert all bits, add 1
  +5 = 00000101
  -5 = 11111011
```

### 2.4 BCD (Binary Coded Decimal)

Each nibble represents one decimal digit (0-9):
```
Decimal 42 = BCD $42 = 0100 0010
Valid BCD bytes: $00-$99
```

> **SNES Note**: BCD mode (D flag) works but is rarely used in games. The SNES has no hardware multiply/divide that uses BCD.

---

## 3. 65816 Architecture

### 3.1 Two Modes of Operation

| Feature | Emulation Mode (E=1) | Native Mode (E=0) |
|---------|---------------------|-------------------|
| Registers | 8-bit (6502 compatible) | 8 or 16-bit selectable |
| Stack | Page 1 only ($0100-$01FF) | Anywhere in Bank 0 |
| Direct Page | Zero Page ($0000-$00FF) | Relocatable |
| Address Space | 64 KB | 16 MB |
| Vectors | $FFFx (emulation) | $FFEx (native) |

> **SNES Note**: SNES programs run in **Native Mode**. The CPU starts in Emulation Mode after reset; `crt0.asm` switches to Native Mode immediately.

### 3.2 Register Set (Native Mode)

```
┌─────────────────────────────────────────────────────────────┐
│  Accumulator C (A + B)      16 bits (or 8-bit A)            │
├─────────────────────────────────────────────────────────────┤
│  Index Register X           16 bits (or 8-bit)              │
├─────────────────────────────────────────────────────────────┤
│  Index Register Y           16 bits (or 8-bit)              │
├─────────────────────────────────────────────────────────────┤
│  Stack Pointer (S)          16 bits                         │
├─────────────────────────────────────────────────────────────┤
│  Direct Page (D)            16 bits (base address)          │
├─────────────────────────────────────────────────────────────┤
│  Program Counter (PC)       16 bits                         │
├─────────────────────────────────────────────────────────────┤
│  Program Bank (PB/K)        8 bits (extends PC to 24-bit)   │
├─────────────────────────────────────────────────────────────┤
│  Data Bank (DB/B)           8 bits (default data bank)      │
├─────────────────────────────────────────────────────────────┤
│  Processor Status (P)       8 bits: N V M X D I Z C         │
│                             + Emulation flag (E) hidden     │
└─────────────────────────────────────────────────────────────┘
```

### 3.3 Status Register Flags (Native Mode)

| Bit | Flag | Description |
|-----|------|-------------|
| 7 | N | Negative - Set if result has bit 7 set |
| 6 | V | Overflow - Signed arithmetic overflow |
| 5 | M | Memory/Accumulator select (0=16-bit, 1=8-bit) |
| 4 | X | Index register select (0=16-bit, 1=8-bit) |
| 3 | D | Decimal mode for ADC/SBC |
| 2 | I | IRQ Disable - When set, masks IRQ interrupts |
| 1 | Z | Zero - Set if result is zero |
| 0 | C | Carry - Carry/borrow for arithmetic |
| - | E | Emulation (hidden, accessed via XCE) |

### 3.4 Switching Modes

```asm
; Switch to Native Mode (do this at startup)
        CLC             ; Clear carry
        XCE             ; Exchange Carry and Emulation

; Switch to Emulation Mode (rarely needed)
        SEC             ; Set carry
        XCE             ; Exchange Carry and Emulation

; Set 16-bit Accumulator and Index
        REP     #$30    ; Reset bits 4 and 5 (M and X)

; Set 8-bit Accumulator and Index
        SEP     #$30    ; Set bits 4 and 5 (M and X)

; Set 16-bit Accumulator only
        REP     #$20    ; Reset M flag

; Set 8-bit Accumulator, 16-bit Index
        SEP     #$20    ; Set M flag
        REP     #$10    ; Reset X flag
```

> **SNES Note**: OpenSNES library functions expect **8-bit A, 16-bit X/Y** (`SEP #$20; REP #$10`). Always restore this state after your code.

### 3.5 Direct Page Register

The Direct Page register (D) relocates the "zero page" anywhere in Bank 0:

```
Effective Address = D + offset

If D = $0000: Direct page address $23 = memory $0023
If D = $2100: Direct page address $23 = memory $2123 (PPU registers!)
If D = $4300: Direct page address $00 = memory $4300 (DMA registers!)
```

**Optimization**: Setting D to a page boundary (low byte = $00) saves one cycle per direct page access.

> **SNES Note**: Setting D to `$2100` or `$4300` allows fast access to PPU or DMA registers using direct page addressing (2 bytes, faster than absolute).

### 3.6 Bank Registers

**Program Bank (PB/K):**
- Extends 16-bit PC to 24-bit address
- Changed by: JML, JSL, RTL, interrupts
- Accessed by: PHK

**Data Bank (DB/B):**
- Default bank for absolute addressing (LDA $1234 reads from DB:$1234)
- Changed by: PLB
- Accessed by: PHB

> **SNES Note**:
> - Program Bank is typically $00-$3F (LoROM) or $00-$7D (HiROM) for ROM code
> - Data Bank is often set to $7E for RAM access or $00 for ROM data
> - Bank $7E-$7F is always WRAM (128 KB)

### 3.7 24-Bit Addressing

```
Full address = Bank : Offset
             = BB   : OOOO

SNES Examples:
  $00:8000 = ROM start (LoROM)
  $7E:0000 = WRAM start (128 KB)
  $7E:2000 = WRAM (safe area, no mirror conflicts)
  $7F:0000 = WRAM upper 64 KB
```

---

## 4. Addressing Modes

### 4.1 Complete Addressing Mode Table

| # | Mode | Syntax | Bytes | Description |
|---|------|--------|-------|-------------|
| 1 | Implied | TAX | 1 | Operation on registers |
| 2 | Accumulator | ASL A | 1 | Operation on A register |
| 3 | Immediate | LDA #$12 | 2-3 | Constant value |
| 4 | Absolute | LDA $1234 | 3 | DB:$1234 |
| 5 | Absolute Long | LDA $123456 | 4 | Full 24-bit address |
| 6 | Direct Page | LDA $12 | 2 | $00:(D+$12) |
| 7 | Absolute,X | LDA $1234,X | 3 | DB:($1234+X) |
| 8 | Absolute Long,X | LDA $123456,X | 4 | $123456+X |
| 9 | Absolute,Y | LDA $1234,Y | 3 | DB:($1234+Y) |
| 10 | Direct Page,X | LDA $12,X | 2 | $00:(D+$12+X) |
| 11 | Direct Page,Y | LDX $12,Y | 2 | $00:(D+$12+Y) |
| 12 | Indirect | LDA ($12) | 2 | DB:[D+$12] |
| 13 | Indirect Long | LDA [$12] | 2 | 24-bit from [D+$12] |
| 14 | Indexed Indirect | LDA ($12,X) | 2 | DB:[D+$12+X] |
| 15 | Indirect Indexed | LDA ($12),Y | 2 | DB:[D+$12]+Y |
| 16 | Indirect Long,Y | LDA [$12],Y | 2 | [24-bit]+Y |
| 17 | Stack Relative | LDA $12,S | 2 | $00:(S+$12) |
| 18 | Stack Relative Indirect,Y | LDA ($12,S),Y | 2 | DB:[S+$12]+Y |
| 19 | Block Move | MVN src,dst | 3 | Block memory move (see section 9.3) |

### 4.2 Addressing Mode Diagrams

**Direct Page (fast for variables):**
```
Instruction: LDA $12
Effective Address = $00:(D + $12)
If D=$0000: reads $000012
If D=$2100: reads $002112 (PPU register!)
```

**Absolute (normal memory access):**
```
Instruction: LDA $1234
Effective Address = DB:$1234
If DB=$7E: reads $7E1234 (WRAM)
If DB=$00: reads $001234 (may be ROM or hardware)
```

**Indirect Long (for 24-bit pointers):**
```
Instruction: LDA [$12]
Pointer at D+$12 (3 bytes, little-endian)
Reads from full 24-bit address in pointer
```

**Stack Relative (for local variables):**
```
Instruction: LDA $03,S
Effective Address = $00:(S + $03)
Great for function parameters and locals
```

### 4.3 Immediate Mode Sizes

The size of immediate operands depends on the M/X flags:

| Instruction | m=1 (8-bit A) | m=0 (16-bit A) |
|-------------|---------------|----------------|
| LDA #imm | 2 bytes | 3 bytes |
| ADC #imm | 2 bytes | 3 bytes |
| CMP #imm | 2 bytes | 3 bytes |

| Instruction | x=1 (8-bit X/Y) | x=0 (16-bit X/Y) |
|-------------|-----------------|------------------|
| LDX #imm | 2 bytes | 3 bytes |
| LDY #imm | 2 bytes | 3 bytes |
| CPX #imm | 2 bytes | 3 bytes |

> **SNES Note**: Assemblers need to know the current register size. WLA-DX uses `.ACCU 8/16` and `.INDEX 8/16` directives, or you can force size with `LDA.B #$12` (8-bit) or `LDA.W #$1234` (16-bit).

---

## 5. Instruction Set

### 5.1 Data Transfer Instructions

#### Load and Store
| Mnemonic | Description | Flags |
|----------|-------------|-------|
| LDA | Load Accumulator | N, Z |
| LDX | Load X Register | N, Z |
| LDY | Load Y Register | N, Z |
| STA | Store Accumulator | - |
| STX | Store X Register | - |
| STY | Store Y Register | - |
| STZ | Store Zero | - |

#### Register Transfers
| Mnemonic | Description | Flags |
|----------|-------------|-------|
| TAX | Transfer A to X | N, Z |
| TAY | Transfer A to Y | N, Z |
| TXA | Transfer X to A | N, Z |
| TYA | Transfer Y to A | N, Z |
| TSX | Transfer S to X | N, Z |
| TXS | Transfer X to S | - |
| TXY | Transfer X to Y | N, Z |
| TYX | Transfer Y to X | N, Z |
| TCD | Transfer 16-bit A to D | N, Z |
| TDC | Transfer D to 16-bit A | N, Z |
| TCS | Transfer 16-bit A to S | - |
| TSC | Transfer S to 16-bit A | N, Z |
| XBA | Exchange B and A (swap bytes of 16-bit A) | N, Z |

#### Stack Operations
| Mnemonic | Description | Flags |
|----------|-------------|-------|
| PHA | Push Accumulator | - |
| PHX | Push X Register | - |
| PHY | Push Y Register | - |
| PHP | Push Processor Status | - |
| PHB | Push Data Bank | - |
| PHD | Push Direct Page | - |
| PHK | Push Program Bank | - |
| PLA | Pull Accumulator | N, Z |
| PLX | Pull X Register | N, Z |
| PLY | Pull Y Register | N, Z |
| PLP | Pull Processor Status | All |
| PLB | Pull Data Bank | N, Z |
| PLD | Pull Direct Page | N, Z |
| PEA | Push Effective Absolute | - |
| PEI | Push Effective Indirect | - |
| PER | Push Effective Relative | - |

#### Block Move
| Mnemonic | Description |
|----------|-------------|
| MVN | Move Block Negative (increment addresses) |
| MVP | Move Block Positive (decrement addresses) |

### 5.2 Arithmetic Instructions

| Mnemonic | Description | Flags |
|----------|-------------|-------|
| ADC | Add with Carry | N, V, Z, C |
| SBC | Subtract with Carry | N, V, Z, C |
| INC | Increment Memory | N, Z |
| INX | Increment X | N, Z |
| INY | Increment Y | N, Z |
| INA / INC A | Increment A | N, Z |
| DEC | Decrement Memory | N, Z |
| DEX | Decrement X | N, Z |
| DEY | Decrement Y | N, Z |
| DEA / DEC A | Decrement A | N, Z |
| CMP | Compare A with Memory | N, Z, C |
| CPX | Compare X with Memory | N, Z, C |
| CPY | Compare Y with Memory | N, Z, C |

**Carry behavior:**
- ADC always adds carry: `A = A + M + C`
- SBC always subtracts borrow: `A = A - M - (1-C)`
- Clear carry before addition: `CLC`
- Set carry before subtraction: `SEC`

### 5.3 Logic Instructions

| Mnemonic | Description | Flags |
|----------|-------------|-------|
| AND | Logical AND | N, Z |
| ORA | Logical OR | N, Z |
| EOR | Exclusive OR | N, Z |
| BIT | Bit Test | N, V, Z |
| TRB | Test and Reset Bits | Z |
| TSB | Test and Set Bits | Z |

### 5.4 Shift and Rotate

| Mnemonic | Description | Flags |
|----------|-------------|-------|
| ASL | Arithmetic Shift Left | N, Z, C |
| LSR | Logical Shift Right | N, Z, C |
| ROL | Rotate Left through Carry | N, Z, C |
| ROR | Rotate Right through Carry | N, Z, C |

```
ASL:  C <- [76543210] <- 0     (multiply by 2)
LSR:  0 -> [76543210] -> C     (divide by 2, unsigned)
ROL:  C <- [76543210] <- C     (9-bit rotate left)
ROR:  C -> [76543210] -> C     (9-bit rotate right)
```

### 5.5 Branch Instructions

| Mnemonic | Condition | Description |
|----------|-----------|-------------|
| BCC / BLT | C = 0 | Branch if Carry Clear / Less Than (unsigned) |
| BCS / BGE | C = 1 | Branch if Carry Set / Greater or Equal (unsigned) |
| BEQ | Z = 1 | Branch if Equal (Zero) |
| BNE | Z = 0 | Branch if Not Equal |
| BMI | N = 1 | Branch if Minus (negative) |
| BPL | N = 0 | Branch if Plus (positive or zero) |
| BVC | V = 0 | Branch if Overflow Clear |
| BVS | V = 1 | Branch if Overflow Set |
| BRA | always | Branch Always (short) |
| BRL | always | Branch Always Long |

**Branch Range:**
- Short branches (BCC, BEQ, etc.): -128 to +127 bytes
- BRL: -32768 to +32767 bytes

### 5.6 Jump and Call Instructions

| Mnemonic | Description |
|----------|-------------|
| JMP addr | Jump to 16-bit address (same bank) |
| JMP (addr) | Jump indirect |
| JMP (addr,X) | Jump indexed indirect |
| JML addr / JMP long | Jump to 24-bit address |
| JML [addr] | Jump indirect long |
| JSR addr | Jump to Subroutine (same bank) |
| JSL addr | Jump to Subroutine Long (24-bit) |
| RTS | Return from Subroutine |
| RTL | Return from Subroutine Long |
| RTI | Return from Interrupt |

### 5.7 Status Register Instructions

| Mnemonic | Description |
|----------|-------------|
| CLC | Clear Carry |
| SEC | Set Carry |
| CLD | Clear Decimal Mode |
| SED | Set Decimal Mode |
| CLI | Clear Interrupt Disable (enable IRQ) |
| SEI | Set Interrupt Disable (disable IRQ) |
| CLV | Clear Overflow |
| REP #imm | Reset Processor Status Bits |
| SEP #imm | Set Processor Status Bits |
| XCE | Exchange Carry and Emulation |

### 5.8 Miscellaneous

| Mnemonic | Description |
|----------|-------------|
| NOP | No Operation (1 byte, 2 cycles) |
| WDM | Reserved (2-byte NOP) |
| BRK | Software Interrupt |
| COP | Co-Processor Enable |
| WAI | Wait for Interrupt |
| STP | Stop the Clock |

> **SNES Note**:
> - **WAI** is useful for waiting for VBlank (NMI) - more efficient than polling
> - **STP** halts the CPU until reset - don't use in games!
> - **COP** and **WDM** are effectively unused on SNES

---

## 6. Programming Techniques

### 6.1 16-Bit Arithmetic

```asm
; 16-bit Addition: RESULT = NUM1 + NUM2
        REP     #$20        ; 16-bit accumulator
        LDA     NUM1
        CLC
        ADC     NUM2
        STA     RESULT
        SEP     #$20        ; Back to 8-bit

; 32-bit Addition (two 16-bit halves)
        REP     #$20
        LDA     NUM1_LO
        CLC
        ADC     NUM2_LO
        STA     RESULT_LO
        LDA     NUM1_HI
        ADC     NUM2_HI     ; Carry propagates from low addition
        STA     RESULT_HI
        SEP     #$20
```

### 6.2 Multiplication by Constants

```asm
; Multiply by 2 (shift left)
        ASL     A

; Multiply by 4
        ASL     A
        ASL     A

; Multiply by 10 (x*8 + x*2)
        ASL     A           ; x*2
        STA     TEMP
        ASL     A           ; x*4
        ASL     A           ; x*8
        CLC
        ADC     TEMP        ; x*8 + x*2 = x*10
```

> **SNES Note**: The SNES has hardware multiply/divide registers at $4202-$4206. For non-constant multiplies, use those instead of shift-add loops.

### 6.3 Loop Constructs

```asm
; Count down (more efficient - BNE after DEX/DEY is free)
        LDX     #100
LOOP:
        ; ... loop body ...
        DEX
        BNE     LOOP

; Count up
        LDX     #0
LOOP:
        ; ... loop body ...
        INX
        CPX     #100
        BNE     LOOP

; Process array with 16-bit index
        REP     #$10        ; 16-bit index
        LDY     #0
LOOP:
        LDA     ARRAY,Y
        ; ... process ...
        INY
        INY                 ; +2 for 16-bit elements
        CPY     #ARRAY_SIZE
        BNE     LOOP
        SEP     #$10
```

### 6.4 Subroutine Patterns

```asm
; Simple subroutine (no local variables)
MY_ROUTINE:
        PHA                 ; Save A if needed
        ; ... do work ...
        PLA
        RTS

; Subroutine with stack-based locals
; Allocate 4 bytes of local storage
LOCAL1 = 1
LOCAL2 = 3
MY_ROUTINE:
        PHD                 ; Save direct page
        TSC                 ; Get stack pointer
        SEC
        SBC     #4          ; Reserve 4 bytes
        TCS
        TCD                 ; D = stack frame (locals at $01, $03)

        ; Use LOCAL1 and LOCAL2 as direct page addresses
        LDA     #$1234
        STA     LOCAL1

        ; Cleanup
        TSC
        CLC
        ADC     #4
        TCS
        PLD
        RTS
```

### 6.5 Table Lookups

```asm
; Byte table lookup (8-bit index)
        LDX     INDEX
        LDA     TABLE,X

; Word table lookup (16-bit values, 8-bit index)
        LDA     INDEX
        ASL     A           ; Multiply by 2
        TAX
        REP     #$20
        LDA     TABLE,X
        SEP     #$20

; Jump table
        LDA     INDEX
        ASL     A
        TAX
        JMP     (JUMP_TABLE,X)

JUMP_TABLE:
        .DW     CASE_0
        .DW     CASE_1
        .DW     CASE_2
```

---

## 7. Interrupts and System Control

### 7.1 SNES Interrupt Vectors

| Type | Native Vector | Emulation Vector | Usage |
|------|---------------|------------------|-------|
| RESET | - | $00FFFC | Power on / Reset button |
| NMI | $00FFEA | $00FFFA | VBlank (every frame) |
| IRQ | $00FFEE | $00FFFE | H/V timer, external |
| BRK | $00FFE6 | $00FFFE | Software breakpoint |
| COP | $00FFE4 | $00FFF4 | Unused on SNES |
| ABORT | $00FFE8 | $00FFF8 | Unused on SNES |

> **SNES Note**: These vectors are in the ROM header area. WLA-DX's `.SNESNATIVEVECTOR` and `.SNESEMUVECTOR` directives set them. The ROM mapper (LoROM/HiROM) affects physical ROM addresses.

### 7.2 Interrupt Sequence

When an interrupt occurs, the CPU:
1. Push PB (native mode only)
2. Push PC high byte
3. Push PC low byte
4. Push P (processor status)
5. Set I flag (disable IRQ)
6. Clear D flag (binary mode)
7. Load PC from vector
8. Set PB to 0 (native mode)

### 7.3 NMI Handler Template (SNES VBlank)

```asm
NMI_HANDLER:
        ; Save all registers
        REP     #$30        ; 16-bit A and X/Y
        PHA
        PHX
        PHY
        PHB
        PHD

        ; Set up known state
        SEP     #$20        ; 8-bit A
        LDA     #$00
        PHA
        PLB                 ; Data bank = $00

        REP     #$20        ; 16-bit A
        LDA     #$0000
        TCD                 ; Direct page = $0000

        SEP     #$20        ; 8-bit A

        ; Acknowledge NMI by reading $4210
        LDA     $4210

        ; ===== VBlank work here =====
        ; - DMA transfers to VRAM/CGRAM/OAM
        ; - Update scroll registers
        ; - Update sound
        ; =============================

        ; Restore all registers
        REP     #$30
        PLD
        PLB
        PLY
        PLX
        PLA

        RTI
```

> **SNES Note**: Reading $4210 (RDNMI) clears the NMI flag. While not strictly required for the current NMI to complete, failing to read it can cause issues detecting subsequent VBlanks. Best practice: always read $4210 early in your NMI handler.

### 7.4 Waiting for VBlank

```asm
; Method 1: Poll the NMI flag (simple but wastes CPU)
WAIT_NMI:
        LDA     $4212       ; HVBJOY
        AND     #$80        ; Check VBlank flag
        BEQ     WAIT_NMI
        RTS

; Method 2: Use WAI instruction (efficient)
; Requires NMI to be enabled
WAIT_NMI:
        WAI                 ; CPU halts until interrupt
        RTS

; Method 3: Use a VBlank counter (set by NMI handler)
WAIT_VBLANK:
        LDA     VBLANK_COUNT
-       CMP     VBLANK_COUNT    ; WLA-DX anonymous label: - and +
        BEQ     -               ; Branch back to previous -
        RTS
```

> **Note**: The `-` and `+` are WLA-DX anonymous labels. `-` branches backward to the previous `-` label, `+` branches forward to the next `+` label.

---

## 8. SNES-Specific Patterns

### 8.1 Register Access via Direct Page

```asm
; Fast PPU register access
        REP     #$20
        LDA     #$2100
        TCD                 ; D = $2100
        SEP     #$20

        ; Now direct page addresses = PPU registers
        ; $00 = $2100 (INIDISP)
        ; $01 = $2101 (OBSEL)
        ; $05 = $2105 (BGMODE)
        ; etc.

        LDA     #$0F
        STA     $00         ; Write to $2100 (turn on screen)

        LDA     #$01
        STA     $05         ; Write to $2105 (set BG mode 1)
```

### 8.2 DMA Transfer Setup

```asm
; Copy data to VRAM using DMA channel 0
        LDA     #$80
        STA     $2115       ; VRAM: increment by 1 word after high byte write

        REP     #$20
        LDA     #VRAM_DEST
        STA     $2116       ; VRAM address (word address)

        LDA     #.LOWORD(SOURCE_DATA)
        STA     $4302       ; Source address (low)
        SEP     #$20
        LDA     #.BANKBYTE(SOURCE_DATA)
        STA     $4304       ; Source bank

        REP     #$20
        LDA     #DATA_SIZE
        STA     $4305       ; Size
        SEP     #$20

        LDA     #$01        ; Mode: 2 bytes to $2118/$2119
        STA     $4300
        LDA     #$18        ; Destination: $2118 (VRAM data)
        STA     $4301

        LDA     #$01        ; Enable DMA channel 0
        STA     $420B
```

### 8.3 Reading Joypad Input

```asm
; Wait for auto-joypad read to complete
-       LDA     $4212
        AND     #$01
        BNE     -

        ; Read joypad 1 (16-bit value)
        REP     #$20
        LDA     $4218       ; JOY1L/JOY1H
        STA     JOY1_CURRENT

        ; Calculate newly pressed buttons
        EOR     JOY1_PREVIOUS
        AND     JOY1_CURRENT
        STA     JOY1_PRESSED

        LDA     JOY1_CURRENT
        STA     JOY1_PREVIOUS
        SEP     #$20
```

> **SNES Note**: Joypad bits: `B Y Sl St U D L R | A X L R 0 0 0 0`

### 8.4 Bank Switching for Data Access

```asm
; Access data in a different bank
        PHB                 ; Save current data bank

        LDA     #$01        ; Bank $01
        PHA
        PLB                 ; Set data bank

        LDA     $8000       ; Reads from $01:8000

        PLB                 ; Restore data bank

; Or use long addressing (no bank change needed)
        LDA     $018000     ; Reads from $01:8000 directly
```

---

## 9. Code Examples

### 9.1 Memory Fill

```asm
; Fill COUNT bytes at ADDR with VALUE (8-bit version, simple)
MEMFILL:
        SEP     #$20        ; 8-bit A
        REP     #$10        ; 16-bit X
        LDA     VALUE
        LDX     COUNT
        BEQ     DONE
LOOP:
        DEX
        STA     ADDR,X
        BNE     LOOP
DONE:
        RTS

; Fill COUNT bytes at ADDR with VALUE (16-bit optimized)
; COUNT must be even, ADDR must be word-aligned
MEMFILL_FAST:
        REP     #$30        ; 16-bit A and X
        LDA     VALUE       ; 16-bit value (low byte duplicated)
        LDX     COUNT
        BEQ     +
-       DEX
        DEX
        STA     ADDR,X
        BNE     -
+       SEP     #$30
        RTS
```

### 9.2 Memory Copy

```asm
; Copy COUNT bytes from SRC to DST
; For large copies, use MVN instead
MEMCOPY:
        REP     #$30
        LDY     #0
LOOP:
        LDA     (SRC),Y
        STA     (DST),Y
        INY
        INY
        CPY     COUNT
        BCC     LOOP
        SEP     #$30
        RTS
```

### 9.3 Block Move (65816 Native)

```asm
; Move 4096 bytes from $7E2000 to $7F0000
        REP     #$30            ; 16-bit A and X/Y
        LDA     #4096-1         ; Count - 1 (in C register)
        LDX     #$2000          ; Source offset
        LDY     #$0000          ; Dest offset
        MVN     $7E,$7F         ; WLA-DX syntax: source_bank, dest_bank
        SEP     #$30
```

> **WARNING - Assembler syntax varies!**
> - **WLA-DX**: `MVN source_bank, dest_bank` (as shown above)
> - **ca65**: `MVN dest_bank, source_bank` (opposite order!)
> - **Opcode encoding**: Always dest first, then source in the bytes
>
> Always verify with your assembler's documentation!

> **SNES Note**: MVN/MVP are fast for RAM-to-RAM copies but tie up the CPU entirely. For VRAM/CGRAM/OAM transfers, use DMA instead (DMA is faster and doesn't block interrupts).

### 9.4 16-Bit Signed Comparison

```asm
; Compare two signed 16-bit numbers
; Returns: C=1 if A >= value, C=0 if A < value
; Also sets N based on sign of (A - value)
SIGNED_CMP:
        SEC
        SBC     COMPARE_VALUE
        BVC     +
        EOR     #$8000      ; Fix sign if overflow
+       BMI     LESS_THAN
        ; A >= value
        SEC
        RTS
LESS_THAN:
        ; A < value
        CLC
        RTS
```

---

## 10. Cycle Counting

### 10.1 Base Cycle Times

| Addressing Mode | Base Cycles |
|-----------------|-------------|
| Implied (TAX, INX) | 2 |
| Immediate | 2 |
| Direct Page | 3 |
| Absolute | 4 |
| Absolute Long | 5 |
| Direct Page,X/Y | 4 |
| Absolute,X/Y | 4 |
| (Direct Page) | 5 |
| (Direct Page),Y | 5 |
| [Direct Page] | 6 |
| Stack Relative | 4 |

### 10.2 Additional Cycles

| Condition | Extra Cycles |
|-----------|--------------|
| 16-bit accumulator (m=0) | +1 for memory ops |
| 16-bit index (x=0) | +1 for index ops |
| Direct page not page-aligned (D & $FF != 0) | +1 |
| Branch taken | +1 |
| Branch crosses page (emulation mode) | +1 |
| Index crosses page boundary | +1 |

### 10.3 VBlank Timing Budget

> **SNES Note**: VBlank timing:
> - NTSC: 262 total scanlines, VBlank is ~37 scanlines (225-261)
> - PAL: 312 total scanlines, VBlank is ~87 scanlines (225-311)
> - Each scanline: 1364 master cycles
>
> **Usable VBlank time:**
> - NTSC: ~37 × 1364 = ~50,468 master cycles ≈ **2,500-3,000 CPU cycles**
> - PAL: ~87 × 1364 = ~118,668 master cycles ≈ **6,000-7,000 CPU cycles**
>
> **DMA transfer speed:** ~8 master cycles per byte (CPU halted during DMA)
>
> **Safe DMA budget:** ~4 KB (NTSC) to leave time for CPU work (scroll updates, OAM, etc.)

---

## Appendix: WLA-DX Assembler Directives

Common directives used in OpenSNES:

| Directive | Description |
|-----------|-------------|
| `.ACCU 8` / `.ACCU 16` | Tell assembler current A size |
| `.INDEX 8` / `.INDEX 16` | Tell assembler current X/Y size |
| `.DB` / `.BYTE` | Define byte(s) |
| `.DW` / `.WORD` | Define word(s) (16-bit) |
| `.DL` / `.LONG` | Define long (24-bit) |
| `.DSB count` | Define storage block (reserve bytes) |
| `.EQU` / `=` | Equate symbol to value |
| `.SECTION "name" SEMIFREE` | Begin relocatable section |
| `.ENDS` | End section |
| `.BANK n` | Set current bank |
| `.ORG addr` | Set origin within bank |

---

*Based on "Programming the 65816" by David Eyes and Ron Lichty (1986). Adapted for SNES development with OpenSNES.*
