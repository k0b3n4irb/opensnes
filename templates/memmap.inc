; Memory map for standalone C compilation units (LoROM)
; Must match the memory map in hdr.asm

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (default)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB) — also the window for Game Pak RAM:
                            ; a Super FX variable is `.RAMSECTION "x" BANK $70 SLOT 3`
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM)
; ROM size is a project knob since 2026-09-24: make passes -D ROM_BANKS_VAL and
; -D ASSET_BANKS_VAL (make/common.mk, ROM_BANKS); the prebuilt library keeps
; the default. wlalink takes the largest bank count it is given.
.IFDEF ROM_BANKS_VAL
.ROMBANKS ROM_BANKS_VAL
.ELSE
.ROMBANKS 8                 ; default; the header's ROMSIZE byte follows it
.ENDIF

; Bank search order for ASSET_SECTION payload: highest first, so bulk
; read-only data lands away from the code bank without anyone picking
; a number by hand. Must name only banks this memory map declares.
.IFDEF ASSET_BANKS_VAL
.DEFINE ASSET_BANKS ASSET_BANKS_VAL
.ELSE
.DEFINE ASSET_BANKS "7-1"   ; highest asset bank first, bank $00 never
.ENDIF
; Suppress WLA-DX "No .ASCIITABLE defined" warning for .ASC directives
.ASCIITABLE
MAP ' ' TO '~' = ' '
.ENDA
