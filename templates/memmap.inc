; Memory map for standalone C compilation units (LoROM)
; Must match the memory map in hdr.asm

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (default)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM)
.ROMBANKS 8                 ; Match ROM configuration

; Bank search order for ASSET_SECTION payload: highest first, so bulk
; read-only data lands away from the code bank without anyone picking
; a number by hand. Must name only banks this memory map declares.
.DEFINE ASSET_BANKS "7-1"

; Suppress WLA-DX "No .ASCIITABLE defined" warning for .ASC directives
.ASCIITABLE
MAP ' ' TO '~' = ' '
.ENDA
