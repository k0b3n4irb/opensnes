; Memory map for standalone C compilation units (SA-1)
; Must match the memory map in hdr_sa1.asm
; SA-1 uses LoROM-based addressing with I-RAM at $3000-$37FF

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (LoROM-based)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM-based)
.ROMBANKS 8                 ; Match ROM configuration

; Suppress WLA-DX "No .ASCIITABLE defined" warning for .ASC directives
.ASCIITABLE
MAP ' ' TO '~' = ' '
.ENDA
