; Library memory map for compiling library object files
; Must match the ROM's LoROM configuration for linking compatibility

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (LoROM)
    DEFAULTSLOT 0
    SLOT 0 $8000            ; ROM mapped at $8000-$FFFF
    SLOT 1 $0000            ; RAM at $0000-$1FFF (DP/Stack)
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM)
.ROMBANKS 2                 ; Match ROM configuration
