; Library memory map for compiling library object files
; Must EXACTLY match the ROM's LoROM configuration in templates/common/hdr.asm

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (default)
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000      ; ROM mapped at $8000-$FFFF (32KB)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM)
.ROMBANKS 8                 ; Match ROM configuration (256KB for soundbanks)
