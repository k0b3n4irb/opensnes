;==============================================================================
; OpenSNES ROM Header Template - HiROM Mode
;==============================================================================
;
; This file defines the SNES ROM header and interrupt vectors for HiROM mode.
;
; The ROM name placeholder __ROM_NAME__ is replaced by the build system.
; ROM name must be EXACTLY 21 characters (pad with spaces).
;
; Memory Map (HiROM):
;   - ROM banks are 64KB ($0000-$FFFF)
;   - Bank $40-$7D: Full 64KB ROM access
;   - Bank $00-$3F: Only $8000-$FFFF mirrors ROM (lower half is RAM/I/O)
;   - Slot 0: ROM at $0000-$FFFF (64KB, but code must be at $8000+)
;   - Slot 1: Work RAM at $0000-$1FFF (8KB for DP/Stack)
;
; CRITICAL HiROM NOTE:
;   Code/data accessed via bank $00 MUST be at $8000-$FFFF.
;   We use .ORG $8000 to place all code in the accessible mirror range.
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

;------------------------------------------------------------------------------
; Memory Configuration - HiROM (64KB banks)
;------------------------------------------------------------------------------

.MEMORYMAP
    ; HiROM uses full 64KB banks. Code must be placed at $8000+ to be
    ; accessible via the bank $00 mirror ($00:8000-FFFF -> $40:8000-FFFF).
    SLOTSIZE $10000         ; 64KB per slot (full HiROM bank)
    DEFAULTSLOT 0
    SLOT 0 $0000 $10000     ; ROM at $0000-$FFFF (64KB per bank)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $10000         ; 64KB per bank (HiROM standard)
.ROMBANKS 4                 ; 256KB ROM (4 x 64KB banks)

;------------------------------------------------------------------------------
; SNES Header (located at $00:FFB0-FFDF in HiROM)
;------------------------------------------------------------------------------

.SNESHEADER
    ID "OPEN"               ; Developer ID (4 chars)
    NAME "__ROM_NAME__"     ; Game title (21 chars, pad with spaces)
    SLOWROM                 ; 2.68MHz ROM access
    HIROM                   ; HiROM memory mapping
    CARTRIDGETYPE __CARTRIDGETYPE__  ; $21=ROM, $23=ROM+SRAM
    ROMSIZE __ROMSIZE__     ; ROM size (1024 << N bytes)
    SRAMSIZE __SRAMSIZE__   ; $00=None, $03=8KB
    COUNTRY $01             ; North America (NTSC)
    LICENSEECODE $00        ; Unlicensed
    VERSION $00             ; Version 1.0
.ENDSNES

;------------------------------------------------------------------------------
; Native Mode Interrupt Vectors ($00:FFE0-FFEF)
;------------------------------------------------------------------------------
; These are used when the CPU is in 65816 native mode (E=0).
;------------------------------------------------------------------------------

.SNESNATIVEVECTOR
    COP EmptyHandler        ; Coprocessor interrupt
    BRK EmptyHandler        ; Break instruction
    ABORT EmptyHandler      ; Abort (not used on SNES)
    NMI NmiHandler          ; VBlank interrupt
    IRQ IrqHandler          ; Hardware IRQ (H/V timer, etc.)
.ENDNATIVEVECTOR

;------------------------------------------------------------------------------
; Emulation Mode Interrupt Vectors ($00:FFF0-FFFF)
;------------------------------------------------------------------------------
; These are used when the CPU is in 6502 emulation mode (E=1).
; RESET always starts in emulation mode.
;------------------------------------------------------------------------------

.SNESEMUVECTOR
    COP EmptyHandler        ; Coprocessor interrupt
    ABORT EmptyHandler      ; Abort (not used on SNES)
    NMI EmptyHandler        ; VBlank (shouldn't fire in emu mode)
    RESET Start             ; Power-on/reset entry point
    IRQBRK EmptyHandler     ; IRQ or BRK
.ENDEMUVECTOR

;------------------------------------------------------------------------------
; Reserve $0000-$7FFF (not accessible via bank $00 mirror)
;------------------------------------------------------------------------------
; In HiROM mode, bank $00:$0000-$7FFF is RAM/I/O, NOT ROM.
; Only bank $00:$8000-$FFFF mirrors ROM (from bank $40:$8000-$FFFF).
; This section reserves the inaccessible range to force all code to $8000+.
;------------------------------------------------------------------------------

.BANK 0
.ORG 0
.SECTION ".hirom_reserved" FORCE
    .DSB $8000, $00     ; Fill $0000-$7FFF with zeros (reserved, inaccessible)
.ENDS

;------------------------------------------------------------------------------
; Minimal Interrupt Handler (for unused vectors)
;------------------------------------------------------------------------------
; CRITICAL: Must be at $8000+ to be accessible from bank $00 vectors!
; Bank $00 addresses $0000-$7FFF are system RAM/I/O, NOT ROM.
; Only $8000-$FFFF mirrors to ROM in bank $00 (mirrors bank $40:$8000-$FFFF).
;------------------------------------------------------------------------------

; Now at $8000 (after reserved section)
.SECTION ".empty_handler" SEMIFREE
EmptyHandler:
    sep #$20            ; 8-bit A
    lda $4210           ; Read RDNMI to acknowledge NMI
    rti
.ENDS
