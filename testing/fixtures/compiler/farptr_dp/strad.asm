; An object across a bank, as only assembly can define one: 16 bytes from
; $7E:FFF8 to $7F:0007 (the RAMSECTION covers its first 8 bytes, the rest is
; bank $7F). Its first half is `tail`'s second half (main.c): the linker lets
; a FORCE section of slot 3 sit on a slot-2 object, which is what the
; fixture wants (the values are known). C declares it `extern FAR u8
; strad[16]`: no compiler may step a pointer to it on 16 bits.
.RAMSECTION "farptr_strad" BANK $7E SLOT 3 ORGA $FFF8 FORCE
strad: dsb 8
.ENDS
