; far_bank7f fixture: C objects in WRAM bank $7F, declared by hand-written
; assembly (the library places nothing there) and reached from C as
; `extern FAR`. SLOT 3 is the whole 64 KB of the bank (templates/memmap.inc).

; Two objects one after the other, then a third of other element widths. No
; ORGA: the linker places them; main.c proves they do not overlap and that
; f7_w ends well above $8000 (index 0x4200 is a 16-bit offset past $8000).
.RAMSECTION "f7_a" BANK $7F SLOT 3
    f7_w    dsw $4400           ; 34 816 bytes
.ENDS

.RAMSECTION "f7_b" BANK $7F SLOT 3
    f7_v    dsw 16
.ENDS

.RAMSECTION "f7_c" BANK $7F SLOT 3
    f7_s8   dsb 1
    f7_s16  dsw 1
    f7_s32  dsb 4
    f7_u8   dsb 300             ; indices past 256 bytes
    f7_u32  dsb 400             ; 100 words of 32 bits
.ENDS

; The same offset in the two WRAM banks, for the cell that reads a $7E object
; and a $7F one in one function: a bank byte shared between the two reads the
; other bank's cell, which holds another value.
.RAMSECTION "e7_alias" BANK $7E SLOT 2 ORGA $2000 FORCE
    e7_x    dsw 8
.ENDS

.RAMSECTION "f7_alias" BANK $7F SLOT 3 ORGA $2000 FORCE
    f7_x    dsw 8
.ENDS
