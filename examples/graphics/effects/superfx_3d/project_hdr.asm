;==============================================================================
; OpenSNES ROM Header Template - SuperFX (GSU) Mode
;==============================================================================
;
; SuperFX cartridge: LoROM mapping with GSU RISC coprocessor (10.74/21.47 MHz).
;
; SUPERFX 3D CUBE is replaced by sed. Other values come from project_config.inc.
;
; Memory Map (SuperFX):
;   - LoROM (32KB ROM banks at $8000-$FFFF)
;   - Banks $00-$3F: ROM ($8000-$FFFF), WRAM mirror ($0000-$1FFF)
;   - $3000-$303F: GSU registers (SNES CPU side)
;   - $3100-$32FF: GSU code cache (read-only from SNES side)
;   - Banks $70-$71: Cartridge SRAM (32-64KB, shared with GSU)
;   - Slot 0: ROM ($8000-$FFFF, 32KB)
;   - Slot 1: WRAM ($0000-$1FFF, 8KB for DP/Stack)
;
; CRITICAL SUPERFX NOTES:
;   - The GSU CANNOT access WRAM ($7E), PPU ($2100), APU ($2140).
;   - ROM bus is EXCLUSIVE: when GSU runs, SNES CPU cannot read ROM.
;   - GSU writes pixel data to cartridge SRAM via PLOT instruction.
;   - SNES CPU must DMA from SRAM ($70:xxxx) to VRAM during VBlank.
;
;==============================================================================

;------------------------------------------------------------------------------
; Memory Configuration - SuperFX (LoROM + cartridge SRAM)
;------------------------------------------------------------------------------

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (LoROM)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM)
.ROMBANKS 8                 ; 256KB ROM

;------------------------------------------------------------------------------
; SNES Header (located at $00:FFB0-FFDF)
;------------------------------------------------------------------------------

.include "project_config.inc"

.SNESHEADER
    ID "OPEN"               ; Developer ID (4 chars)
    NAME "SUPERFX 3D CUBE"     ; Game title (21 chars, pad with spaces)
.ifdef FASTROM
    FASTROM
.else
    SLOWROM
.endif
    LOROM                   ; LoROM addressing
    CARTRIDGETYPE CARTRIDGETYPE  ; $14=ROM+SuperFX+SRAM
    ROMSIZE ROMSIZE_VAL     ; ROM size (1024 << N bytes)
    SRAMSIZE SRAMSIZE_VAL   ; Cartridge SRAM ($05=32KB)
    COUNTRY $01             ; North America (NTSC)
    LICENSEECODE $00        ; Unlicensed
    VERSION $00             ; Version 1.0
.ENDSNES

;------------------------------------------------------------------------------
; Native Mode Interrupt Vectors ($00:FFE0-FFEF)
;------------------------------------------------------------------------------

.SNESNATIVEVECTOR
    COP EmptyHandler
    BRK EmptyHandler
    ABORT EmptyHandler
    NMI NmiHandler
    IRQ IrqHandler
.ENDNATIVEVECTOR

;------------------------------------------------------------------------------
; Emulation Mode Interrupt Vectors ($00:FFF0-FFFF)
;------------------------------------------------------------------------------

.SNESEMUVECTOR
    COP EmptyHandler
    ABORT EmptyHandler
    NMI EmptyHandler
    RESET Start
    IRQBRK EmptyHandler
.ENDEMUVECTOR

;------------------------------------------------------------------------------
; Minimal Interrupt Handler
;------------------------------------------------------------------------------

.SECTION ".empty_handler" SEMIFREE
EmptyHandler:
    sep #$20
    lda $4210           ; Read RDNMI to acknowledge NMI
    rti
.ENDS
