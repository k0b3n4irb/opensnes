; Library memory map for compiling library object files
; SA-1 configuration (LoROM-based, 32KB banks)
; Same bank layout as LoROM — SA-1 differences are in the ROM header,
; I-RAM/BW-RAM access, and the SA-1 coprocessor registers.

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
