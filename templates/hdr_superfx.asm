;==============================================================================
; OpenSNES ROM Header Template - SuperFX (GSU) Mode
;==============================================================================
;
; SuperFX cartridge: LoROM mapping with GSU RISC coprocessor (10.74/21.47 MHz).
;
; __ROM_NAME__ is replaced by sed. Other values come from project_config.inc.
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
.IFDEF ROM_BANKS_VAL
.ROMBANKS ROM_BANKS_VAL     ; project knob (make ROM_BANKS=…)
.ELSE
.ROMBANKS 8
.ENDIF

;------------------------------------------------------------------------------
; SNES Header (located at $00:FFB0-FFDF)
;------------------------------------------------------------------------------

.include "project_config.inc"

;------------------------------------------------------------------------------
; Extended Header ($FFB0-$FFBF) — fill with $FF like Star Fox
;------------------------------------------------------------------------------
; Star Fox has all $FF in the extended header area (= no extended header).
; snes9x may misdetect if this area contains non-$FF values.
;------------------------------------------------------------------------------
.BANK 0 SLOT 0
; Extended header ($FFB0-$FFBF), recognised when the licensee code at $FFDA
; is $33 (snesdev-wiki "ROM header / Expanded cartridge header"; fullsnes
; "Extended Header"). A Super FX cart declares its Game Pak RAM HERE, at
; $FFBD (1 KB << n), and leaves $FFD8 at $00 — both arbiters agree. Until
; 2026-09-24 this block was sixteen $FF bytes and $FFDA was $00: no
; expansion RAM declared, no extended header at all.
.ORG $7FB0
.SECTION ".extended_header" FORCE
.db "OS"                    ; $FFB0 maker code (2 ASCII)
.db "GSU0"                  ; $FFB2 game code (4 ASCII)
.db $00,$00,$00,$00,$00,$00 ; $FFB6 reserved
.db $00                     ; $FFBC expansion FLASH size (none)
.db GSU_RAM_SIZE_VAL        ; $FFBD expansion RAM size: 1 KB << n (make GSU_RAM_KB=…)
.db $00                     ; $FFBE special version
.db $00                     ; $FFBF chipset sub-type
.ENDS

.SNESHEADER
    NAME "__ROM_NAME__"     ; Game title (21 chars, pad with spaces)
.ifdef FASTROM
    FASTROM
.else
    SLOWROM
.endif
    LOROM                   ; LoROM addressing
    CARTRIDGETYPE CARTRIDGETYPE  ; $13=ROM+GSU (Star Fox compatible)
    ROMSIZE ROMSIZE_VAL     ; ROM size (1024 << N bytes)
    SRAMSIZE $00            ; $00 here: Game Pak RAM is declared at $FFBD (see above)
    COUNTRY $01             ; North America (NTSC)
    LICENSEECODE $33        ; $33 = "extended header present" (not a licensee)
    VERSION $00             ; Version 1.0
.ENDSNES


;------------------------------------------------------------------------------
; Native Mode Interrupt Vectors ($00:FFE0-FFEF)
;------------------------------------------------------------------------------

.SNESNATIVEVECTOR
    ; Super FX: every native vector is a WRAM address — the GSU answers a
    ; vector fetch with exactly these values while it owns the ROM (manual
    ; Book II §5.4.1, table 2-5-1), and crt0 installs a `JML handler` at
    ; each of them at boot (.gsu_vectors, from gsu_vector_stubs below).
    ; Same path, GSU idle or busy.
    COP $0104
    BRK $0100
    ABORT $0100
    NMI $0108
    IRQ $010C
.ENDNATIVEVECTOR

; The four WRAM vector stubs as bytes — `JML` ($5C) + 24-bit handler — in
; the order of crt0's .gsu_vectors: BRK/ABORT, COP, NMI, IRQ. crt0 copies
; the 16 bytes to $0100 at boot.
.SECTION ".gsu_vector_stubs" SEMIFREE BANK 0
gsu_vector_stubs:
    .db $5C
    .dw EmptyHandler
    .db :EmptyHandler
    .db $5C
    .dw EmptyHandler
    .db :EmptyHandler
    .db $5C
    .dw gsu_nmi_wram        ; phase B: the WRAM NMI, which falls through to
    .db :gsu_nmi_wram       ; NmiHandler whenever the GSU is idle
    .db $5C
    .dw IrqHandler
    .db :IrqHandler
.ENDS

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
    .ACCU 8
    lda $4210           ; Read RDNMI to acknowledge NMI
    rti
.ENDS
