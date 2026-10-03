; Memory map for GSU (SuperFX) code assembly
; Used by wla-superfx to assemble GSU programs into flat binaries.
;
; Two ways to assemble a program (make/common.mk, GSU_BANK):
;
; - GSU_BANK unset (default): the program is assembled at $0000 and placed
;   by the 65816 link wherever it fits. Its labels are OFFSETS in the
;   binary, so it must be position-independent: relative branches, LOOP
;   through R13 = R15, no absolute jump, no table in ROM.
; - GSU_BANK = n: the program is assembled at $8000 and the 65816 link puts
;   it at the start of ROM bank n (GSU_SECTION, templates/assets.inc), which
;   is where the GSU sees it too (LoROM view, banks $00-$3F). Its labels are
;   then real addresses: `IWT R15, #label` jumps there, `IWT R14, #table`
;   with `ROMB` = GSU_BANK reads a table of the program.

.MEMORYMAP
    SLOTSIZE $8000
    DEFAULTSLOT 0
.ifdef GSU_BANK
    SLOT 0 START $8000 SIZE $8000
.else
    SLOT 0 START $0000 SIZE $8000
.endif
.ENDME

.ROMBANKSIZE $8000
.ROMBANKS 1
