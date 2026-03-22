; Memory map for GSU (SuperFX) code assembly
; Used by wla-superfx to assemble GSU programs into flat binaries.
; The GSU sees ROM linearly from $0000 within each bank.

.MEMORYMAP
    SLOTSIZE $8000
    DEFAULTSLOT 0
    SLOT 0 START $0000 SIZE $8000
.ENDME

.ROMBANKSIZE $8000
.ROMBANKS 1
