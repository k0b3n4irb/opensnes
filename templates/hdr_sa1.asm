;==============================================================================
; OpenSNES ROM Header Template - SA-1 Mode
;==============================================================================
;
; SA-1 cartridge: LoROM-based mapping with SA-1 coprocessor (10.74 MHz 65c816).
;
; __ROM_NAME__ is replaced by sed. Other values come from project_config.inc.
;
; Memory Map (SA-1):
;   - LoROM-based (32KB ROM banks at $8000-$FFFF)
;   - Banks $00-$3F: ROM ($8000-$FFFF) via Super MMC
;   - $3000-$37FF: I-RAM (2KB, shared between SNES CPU and SA-1)
;   - $2200-$23FF: SA-1 registers
;   - Banks $40-$5F: BW-RAM (256KB, battery-backed)
;   - Slot 0: ROM ($8000-$FFFF, 32KB)
;   - Slot 1: WRAM ($0000-$1FFF, 8KB for DP/Stack)
;   - Slot 2: I-RAM ($3000-$37FF, 2KB shared)
;
; CRITICAL SA-1 NOTE:
;   The SA-1 CPU CANNOT access main WRAM ($7E), PPU ($2100), APU ($2140),
;   or standard CPU I/O ($4200). It can only access ROM, I-RAM, and BW-RAM.
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

;------------------------------------------------------------------------------
; Memory Configuration - SA-1 (LoROM + I-RAM + BW-RAM)
;------------------------------------------------------------------------------

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (LoROM-based)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM-based)
.ROMBANKS 8                 ; 256KB ROM

;------------------------------------------------------------------------------
; SNES Header (located at $00:FFB0-FFDF)
;------------------------------------------------------------------------------
; SA-1 requires map mode $23 at $FFD5. WLA-DX's LOROM directive sets $20.
; We use LOROM for correct address resolution, then patch $FFD5 to $23.
;------------------------------------------------------------------------------

.include "project_config.inc"

.SNESHEADER
    ID "OPEN"               ; Developer ID (4 chars)
    NAME "__ROM_NAME__"     ; Game title (21 chars, pad with spaces)
.ifdef FASTROM
    FASTROM                 ; Fast ROM access (register $420D = $01)
.else
    SLOWROM                 ; 2.68MHz ROM access
.endif
    LOROM                   ; LoROM address resolution (SA-1 is LoROM-based)
    CARTRIDGETYPE CARTRIDGETYPE  ; $35=ROM+SA-1+RAM+Battery
    ROMSIZE ROMSIZE_VAL     ; ROM size (1024 << N bytes)
    SRAMSIZE SRAMSIZE_VAL   ; BW-RAM size ($05=32KB, $08=256KB)
    COUNTRY $01             ; North America (NTSC)
    LICENSEECODE $00        ; Unlicensed
    VERSION $00             ; Version 1.0
.ENDSNES

;------------------------------------------------------------------------------
; SA-1 map mode byte ($FFD5) patch
;------------------------------------------------------------------------------
; WLA-DX LOROM sets $FFD5 to $20/$30. SA-1 needs $23/$33.
; Cannot patch via FORCE section (conflicts with .SNESHEADER).
; The build system patches the ROM binary after linking (common.mk).
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
; Native Mode Interrupt Vectors ($00:FFE0-FFEF)
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

.SNESEMUVECTOR
    COP EmptyHandler        ; Coprocessor interrupt
    ABORT EmptyHandler      ; Abort (not used on SNES)
    NMI EmptyHandler        ; VBlank (shouldn't fire in emu mode)
    RESET Start             ; Power-on/reset entry point
    IRQBRK EmptyHandler     ; IRQ or BRK
.ENDEMUVECTOR

;------------------------------------------------------------------------------
; Minimal Interrupt Handler
;------------------------------------------------------------------------------

.SECTION ".empty_handler" SEMIFREE
EmptyHandler:
    sep #$20            ; 8-bit A
    lda $4210           ; Read RDNMI to acknowledge NMI
    rti
.ENDS
