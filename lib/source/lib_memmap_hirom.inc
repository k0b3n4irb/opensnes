; Library memory map for compiling library object files
; HiROM configuration (64KB banks)

.MEMORYMAP
    SLOTSIZE $10000         ; 64KB per slot (HiROM)
    DEFAULTSLOT 0
    SLOT 0 $0000 $10000     ; ROM at $0000-$FFFF (64KB per bank)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $10000         ; 64KB banks (HiROM)
.ROMBANKS 4                 ; Match ROM configuration (256KB)
