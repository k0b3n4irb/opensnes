;==============================================================================
; OpenSNES - bgSetScroll(), bgSetScrollX(), bgSetScrollY() in assembly
;==============================================================================
; The three write the scroll shadow (bg_scroll_x / bg_scroll_y, crt0) and set
; the layer's bit of bg_scroll_dirty; the NMI handler copies dirty layers to
; the PPU. In C until 2026-10-09: `1 << bg` is a shift loop on this CPU, and
; devtools/libbench measured the call above PVSnesLib's.
;
; A layer number of 4 or more is ignored (the C version wrote past the
; arrays).
;
; Calling convention (cc65816): arguments pushed left to right, 2 bytes each,
; the last one at 4,s. A is 16-bit on return.
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

.SECTION ".rodata.bg_scroll_bit" SEMISUPERFREE BANKS 7-1
bg_scroll_bit:
    .db $01, $02, $04, $08
.ENDS

;------------------------------------------------------------------------------
; void bgSetScroll(u8 bg, u16 x, u16 y)
;   8,s bg   6,s x   4,s y
;------------------------------------------------------------------------------
.SECTION ".text.bgSetScroll" SUPERFREE

bgSetScroll:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 8,s             ; bg
    and.w #$00FF
    cmp.w #4
    bcs @out
    tay
    asl a
    tax
    lda 6,s             ; x
    sta.l bg_scroll_x,x
    lda 4,s             ; y
    sta.l bg_scroll_y,x
    tyx
    sep #$20
    .ACCU 8
    lda.l bg_scroll_bit,x
    ora.l bg_scroll_dirty
    sta.l bg_scroll_dirty
    rep #$20
    .ACCU 16
@out:
    rtl

.ENDS

;------------------------------------------------------------------------------
; void bgSetScrollX(u8 bg, u16 x)
;   6,s bg   4,s x
;------------------------------------------------------------------------------
.SECTION ".text.bgSetScrollX" SUPERFREE

bgSetScrollX:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s             ; bg
    and.w #$00FF
    cmp.w #4
    bcs @out
    tay
    asl a
    tax
    lda 4,s             ; x
    sta.l bg_scroll_x,x
    tyx
    sep #$20
    .ACCU 8
    lda.l bg_scroll_bit,x
    ora.l bg_scroll_dirty
    sta.l bg_scroll_dirty
    rep #$20
    .ACCU 16
@out:
    rtl

.ENDS

;------------------------------------------------------------------------------
; void bgSetScrollY(u8 bg, u16 y)
;   6,s bg   4,s y
;------------------------------------------------------------------------------
.SECTION ".text.bgSetScrollY" SUPERFREE

bgSetScrollY:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s             ; bg
    and.w #$00FF
    cmp.w #4
    bcs @out
    tay
    asl a
    tax
    lda 4,s             ; y
    sta.l bg_scroll_y,x
    tyx
    sep #$20
    .ACCU 8
    lda.l bg_scroll_bit,x
    ora.l bg_scroll_dirty
    sta.l bg_scroll_dirty
    rep #$20
    .ACCU 16
@out:
    rtl

.ENDS
