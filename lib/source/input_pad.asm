;==============================================================================
; OpenSNES - padReleased() in assembly
;==============================================================================
; The buttons that were down on the previous frame and are not now:
; pad_keysold[pad] & ~pad_keys[pad], 0 for a pad number above 4. padHeld()
; and padPressed() are macros over one word each (input.h); this one reads
; two, so it stays a function, and the compiled C cost twice PVSnesLib's
; (devtools/libbench, 2026-10-09).
;
; Calling convention (cc65816): one 2-byte argument at 4,s; result in A,
; 16-bit on return.
;==============================================================================

.ifdef SA1
.include "memmap_sa1.inc"
.else
.ifdef HIROM
.include "memmap_hirom.inc"
.else
.include "memmap.inc"
.endif
.endif

;------------------------------------------------------------------------------
; u16 padReleased(u8 pad)
;   4,s pad
;------------------------------------------------------------------------------
.SECTION ".text.padReleased" SUPERFREE

padReleased:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 4,s             ; pad
    and.w #$00FF
    cmp.w #5
    bcs @none
    asl a
    tax
    lda.l pad_keys,x
    eor.w #$FFFF
    and.l pad_keysold,x
    rtl
@none:
    lda.w #0
    rtl

.ENDS
