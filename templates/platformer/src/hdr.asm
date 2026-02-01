;==============================================================================
; OpenSNES ROM Header
;==============================================================================
;
; This file defines the SNES ROM header for LoROM configuration.
;
; Attribution:
;   Based on PVSnesLib header format by Alekmaul
;
;==============================================================================

;------------------------------------------------------------------------------
; Memory Map Configuration
;------------------------------------------------------------------------------

.MEMORYMAP
    SLOTSIZE $8000
    DEFAULTSLOT 0
    SLOT 0 $8000
.ENDME

.ROMBANKSIZE $8000

.ROMBANKS 8

;------------------------------------------------------------------------------
; SNES Header (LoROM at $00:FFB0)
;------------------------------------------------------------------------------

.SNESHEADER
    ID "SNES"

    NAME "PLATFORMER TEMPLATE "  ; 21 characters, space-padded

    SLOWROM
    LOROM

    CARTRIDGETYPE $00           ; ROM only
    ROMSIZE $08                 ; 256KB
    SRAMSIZE $00                ; No SRAM
    COUNTRY $01                 ; North America
    LICENSEECODE $00
    VERSION $00
.ENDSNES

;------------------------------------------------------------------------------
; Interrupt Vectors
;------------------------------------------------------------------------------

.SNATIVEVECTOR
    COP     EmptyHandler
    BRK     EmptyHandler
    ABORT   EmptyHandler
    NMI     VBlankHandler
    IRQ     EmptyHandler
.ENDNATIVEVECTOR

.SEMUVECTOR
    COP     EmptyHandler
    ABORT   EmptyHandler
    NMI     EmptyHandler
    RESET   ResetHandler
    IRQ     EmptyHandler
.ENDEMUVECTOR

;------------------------------------------------------------------------------
; Handlers (defined in crt0.asm)
;------------------------------------------------------------------------------

.BANK 0
.ORG 0

.SECTION ".vectors" SEMIFREE

EmptyHandler:
    rti

.ENDS
