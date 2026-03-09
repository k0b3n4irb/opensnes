; Memory map for standalone C compilation units (HiROM)
; Must match the memory map in hdr_hirom.asm

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
.ROMBANKS 4                 ; Match ROM configuration
