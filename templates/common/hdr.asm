;==============================================================================
; OpenSNES ROM Header Template
;==============================================================================
;
; This file defines the SNES ROM header and interrupt vectors.
;
; The ROM name placeholder __ROM_NAME__ is replaced by the build system.
; ROM name must be EXACTLY 21 characters (pad with spaces).
;
; Memory Map:
;   - LoROM mode (32KB banks)
;   - Slot 0: $8000-$FFFF (ROM)
;   - Slot 1: $0000-$1FFF (RAM for Direct Page/Stack)
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

;------------------------------------------------------------------------------
; Memory Configuration
;------------------------------------------------------------------------------

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (default)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks
.ROMBANKS 8                 ; 256KB ROM (for soundbanks)

;------------------------------------------------------------------------------
; SNES Header (located at $00:FFB0-FFDF)
;------------------------------------------------------------------------------

.SNESHEADER
    ID "OPEN"               ; Developer ID (4 chars)
    NAME "__ROM_NAME__"     ; Game title (21 chars, pad with spaces)
    SLOWROM                 ; 2.68MHz ROM access
    LOROM                   ; LoROM memory mapping
    CARTRIDGETYPE __CARTRIDGETYPE__  ; $00=ROM, $02=ROM+SRAM
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
; Minimal Interrupt Handler (for unused vectors)
;------------------------------------------------------------------------------
; CRITICAL: Must acknowledge NMI by reading $4210, otherwise NMI fires repeatedly
;------------------------------------------------------------------------------

.BANK 0
.ORG 0

.SECTION ".empty_handler" SEMIFREE
EmptyHandler:
    sep #$20            ; 8-bit A
    lda $4210           ; Read RDNMI to acknowledge NMI
    rti
.ENDS
