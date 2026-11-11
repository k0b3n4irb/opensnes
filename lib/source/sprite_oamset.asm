;==============================================================================
; OpenSNES - the sprite setters, in assembly
;==============================================================================
; oamSet(), oamSetX(), oamSetY(), oamSetXY() and oamSetSize(): the calls a
; game makes for every sprite on every frame.
;
; A sprite is four bytes of the low table (x, y, tile, attributes) and two
; bits of the high table (bit 8 of x, size), four sprites to a byte. The two
; bits are reached through oam_ext_tab, one row per sprite: the mask that
; sets its x bit, the mask that clears it, and the index of its byte. Until
; 2026-10-09 the mask was built by a shift loop in oamSet() and by compiled C
; in the others; devtools/libbench measured oamSetXY() at 3.6 times the cost
; of PVSnesLib's.
;
; Calling convention (cc65816): arguments pushed left to right, 2 bytes each
; here, the last one at 4,s. No register is saved; A is 16-bit on return.
; tcc__r9 is used as scratch by oamSet() only.
;
; The OAM shadow and its two flags are plain RAM below $2000, reached here
; through the data bank (`.w`) as compiled C reaches its globals: bank $00
; at run time, $7E inside the object engine's callbacks, both of which map
; it.
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

; One row per sprite: +0 the mask that sets bit 8 of x, +1 the mask that
; clears it, +2 the index of its byte in the high table, +3 zero (so that a
; 16-bit read of +2 is the index). The size bit is the x bit shifted left.
.SECTION ".rodata.oam_ext_tab" SEMISUPERFREE BANKS 7-1
oam_ext_tab:
    .db $01, $FE, 0, 0
    .db $04, $FB, 0, 0
    .db $10, $EF, 0, 0
    .db $40, $BF, 0, 0
    .db $01, $FE, 1, 0
    .db $04, $FB, 1, 0
    .db $10, $EF, 1, 0
    .db $40, $BF, 1, 0
    .db $01, $FE, 2, 0
    .db $04, $FB, 2, 0
    .db $10, $EF, 2, 0
    .db $40, $BF, 2, 0
    .db $01, $FE, 3, 0
    .db $04, $FB, 3, 0
    .db $10, $EF, 3, 0
    .db $40, $BF, 3, 0
    .db $01, $FE, 4, 0
    .db $04, $FB, 4, 0
    .db $10, $EF, 4, 0
    .db $40, $BF, 4, 0
    .db $01, $FE, 5, 0
    .db $04, $FB, 5, 0
    .db $10, $EF, 5, 0
    .db $40, $BF, 5, 0
    .db $01, $FE, 6, 0
    .db $04, $FB, 6, 0
    .db $10, $EF, 6, 0
    .db $40, $BF, 6, 0
    .db $01, $FE, 7, 0
    .db $04, $FB, 7, 0
    .db $10, $EF, 7, 0
    .db $40, $BF, 7, 0
    .db $01, $FE, 8, 0
    .db $04, $FB, 8, 0
    .db $10, $EF, 8, 0
    .db $40, $BF, 8, 0
    .db $01, $FE, 9, 0
    .db $04, $FB, 9, 0
    .db $10, $EF, 9, 0
    .db $40, $BF, 9, 0
    .db $01, $FE, 10, 0
    .db $04, $FB, 10, 0
    .db $10, $EF, 10, 0
    .db $40, $BF, 10, 0
    .db $01, $FE, 11, 0
    .db $04, $FB, 11, 0
    .db $10, $EF, 11, 0
    .db $40, $BF, 11, 0
    .db $01, $FE, 12, 0
    .db $04, $FB, 12, 0
    .db $10, $EF, 12, 0
    .db $40, $BF, 12, 0
    .db $01, $FE, 13, 0
    .db $04, $FB, 13, 0
    .db $10, $EF, 13, 0
    .db $40, $BF, 13, 0
    .db $01, $FE, 14, 0
    .db $04, $FB, 14, 0
    .db $10, $EF, 14, 0
    .db $40, $BF, 14, 0
    .db $01, $FE, 15, 0
    .db $04, $FB, 15, 0
    .db $10, $EF, 15, 0
    .db $40, $BF, 15, 0
    .db $01, $FE, 16, 0
    .db $04, $FB, 16, 0
    .db $10, $EF, 16, 0
    .db $40, $BF, 16, 0
    .db $01, $FE, 17, 0
    .db $04, $FB, 17, 0
    .db $10, $EF, 17, 0
    .db $40, $BF, 17, 0
    .db $01, $FE, 18, 0
    .db $04, $FB, 18, 0
    .db $10, $EF, 18, 0
    .db $40, $BF, 18, 0
    .db $01, $FE, 19, 0
    .db $04, $FB, 19, 0
    .db $10, $EF, 19, 0
    .db $40, $BF, 19, 0
    .db $01, $FE, 20, 0
    .db $04, $FB, 20, 0
    .db $10, $EF, 20, 0
    .db $40, $BF, 20, 0
    .db $01, $FE, 21, 0
    .db $04, $FB, 21, 0
    .db $10, $EF, 21, 0
    .db $40, $BF, 21, 0
    .db $01, $FE, 22, 0
    .db $04, $FB, 22, 0
    .db $10, $EF, 22, 0
    .db $40, $BF, 22, 0
    .db $01, $FE, 23, 0
    .db $04, $FB, 23, 0
    .db $10, $EF, 23, 0
    .db $40, $BF, 23, 0
    .db $01, $FE, 24, 0
    .db $04, $FB, 24, 0
    .db $10, $EF, 24, 0
    .db $40, $BF, 24, 0
    .db $01, $FE, 25, 0
    .db $04, $FB, 25, 0
    .db $10, $EF, 25, 0
    .db $40, $BF, 25, 0
    .db $01, $FE, 26, 0
    .db $04, $FB, 26, 0
    .db $10, $EF, 26, 0
    .db $40, $BF, 26, 0
    .db $01, $FE, 27, 0
    .db $04, $FB, 27, 0
    .db $10, $EF, 27, 0
    .db $40, $BF, 27, 0
    .db $01, $FE, 28, 0
    .db $04, $FB, 28, 0
    .db $10, $EF, 28, 0
    .db $40, $BF, 28, 0
    .db $01, $FE, 29, 0
    .db $04, $FB, 29, 0
    .db $10, $EF, 29, 0
    .db $40, $BF, 29, 0
    .db $01, $FE, 30, 0
    .db $04, $FB, 30, 0
    .db $10, $EF, 30, 0
    .db $40, $BF, 30, 0
    .db $01, $FE, 31, 0
    .db $04, $FB, 31, 0
    .db $10, $EF, 31, 0
    .db $40, $BF, 31, 0
.ENDS

;------------------------------------------------------------------------------
; void oamSet(u16 id, u16 x, u16 y, u16 tile, u16 palette, u16 priority, u16 flags)
;   16,s id   14,s x   12,s y   10,s tile   8,s palette   6,s priority   4,s flags
;------------------------------------------------------------------------------
.SECTION ".text.oamSet" SUPERFREE

oamSet:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 16,s             ; id
    cmp.w #128
    bcs @none           ; not a sprite: nothing is written
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    lda.l oam_ext_tab+2,x
    tay                 ; Y = id >> 2: the sprite's byte of the high table
    sep #$20
    .ACCU 8

    lda 10,s            ; tile low byte
    sta.l oamMemory+2,x

    ; attributes, vhoopppc: v and h from flags, oo priority, ppp palette,
    ; c bit 8 of the tile
    lda 6,s             ; priority
    and #$03
    asl a
    asl a
    asl a
    sta.b tcc__r9
    lda 8,s             ; palette
    and #$07
    ora.b tcc__r9
    asl a               ; (priority << 4) | (palette << 1)
    sta.b tcc__r9
    lda 11,s            ; tile high byte
    and #$01
    ora.b tcc__r9
    sta.b tcc__r9
    lda 4,s             ; flags
    and #$C0
    ora.b tcc__r9
    sta.l oamMemory+3,x

    lda 14,s            ; x low byte
    sta.l oamMemory,x
    ; y is stored as given: a sprite at OAM y = 0 begins on the first visible
    ; line (snesdev-wiki, Sprites / OAM: "a sprite with Y=0 will appear to
    ; begin on the first visible line"; corpus 9dd075095fd0d965). The
    ; library stored y - 1 from 2026-04-27 to 2026-10-09, which drew every
    ; sprite one line too high — one line above a background scrolled to the
    ; same y since bgSetScroll() got its own, correct, - 1 (2026-09-12).
    lda 12,s            ; y low byte
    sta.l oamMemory+1,x

    lda 15,s             ; x high byte
    lsr a               ; carry = bit 8 of x
    bcs @x_high
    lda.l oam_ext_tab+1,x
    sta.w oam_update_flag ; any non-zero value: a mask is never 0
    and.w oamMemory+512,y
@x_done:
    sta.w oamMemory+512,y

    lda 16,s             ; id (low byte: it is under 128)
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id     ; highest sprite written: the NMI sends up to it
+   rep #$20
    .ACCU 16
@none:
    rtl
@x_high:
    .ACCU 8
    lda.l oam_ext_tab,x
    sta.w oam_update_flag
    ora.w oamMemory+512,y
    bra @x_done

.ENDS

;------------------------------------------------------------------------------
; void oamHide(u16 id)
;   4,s id
;
; Park one sprite: x low 1, y 240 (one word), bit 8 of x set; tile,
; attributes and size bit untouched. Highest id -> oam_max_id, flag set.
;------------------------------------------------------------------------------
.SECTION ".text.oamHide" SUPERFREE

oamHide:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 4,s             ; id
    cmp.w #128
    bcc +
    rtl                 ; not a sprite: nothing is written
+   asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    lda.w #$F001        ; y 240 over x 1
    sta.w oamMemory,x
    lda.l oam_ext_tab+2,x
    tay                 ; Y = the sprite's byte of the high table
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x ; the mask that sets bit 8
    sta.w oam_update_flag ; any non-zero value: a mask is never 0
    ora.w oamMemory+512,y
    sta.w oamMemory+512,y
    lda 4,s             ; id (low byte: it is under 128)
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id
+   rep #$20
    .ACCU 16
    rtl

.ENDS

;------------------------------------------------------------------------------
; void oamHideRange(u16 first, u16 count)
;   6,s first   4,s count
;
; oamHide() for count sprites from first: x low 1, y 240, bit 8 of x set,
; tile, attributes and size bit untouched. Clamped at 128. Highest id written
; -> oam_max_id, oam_update_flag set.
;
; Both tables are written by straight runs of stores entered part way, so
; that a sprite costs its stores and nothing else: the low table one word a
; sprite (y 240 over x 1, `sta abs,x`, 40 mclk), the high table one word for
; eight sprites (`ora #$5555`), with a mask at the two edge words. Until
; 2026-11-11 a loop with a compare a sprite and a byte of the high table at a
; time: 307 mclk a sprite for 32, 274 for 128 (devtools/libbench, rows
; hiderange, hide128).
; tcc__r9: end (exclusive), tcc__r9+2: the end word, tcc__r10 (4 bytes): the
; two edge masks, tcc__r0: scratch.
;------------------------------------------------------------------------------

.DEFINE OHR_SMALL 3               ; under this many sprites, the loop of oamHide()'s stores

.SECTION ".text.oamHideRange" SUPERFREE

oamHideRange:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s             ; first
    cmp.w #128
    bcc +
    rtl                 ; not a sprite
+   lda 4,s             ; count
    bne +
    rtl                 ; nothing to hide
+   clc
    adc 6,s             ; end = first + count
    cmp.w #129
    bcc +
    lda.w #128
+   sta.b tcc__r9       ; end
    sec
    sbc 6,s             ; first: n = end - first
    cmp.w #OHR_SMALL
    bcs _ohr_big
    lda 6,s             ; first
    tay                 ; Y = id: a short run, sprite by sprite
_ohr_one:
    tya
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    lda.w #$F001        ; y 240 over x 1
    sta.w oamMemory,x
    lda.l oam_ext_tab+2,x
    sta.b tcc__r10      ; the index of its byte in the high table
    lda.l oam_ext_tab,x ; low byte: the mask that sets bit 8
    ldx.b tcc__r10
    sep #$20
    .ACCU 8
    ora.w oamMemory+512,x
    sta.w oamMemory+512,x
    rep #$20
    .ACCU 16
    iny
    cpy.b tcc__r9
    bcc _ohr_one
    brl _ohr_done

_ohr_big:
    ; the low table: a run of 64 `sta dp,x` (2 bytes, 38 mclk on SlowROM
    ; against 46 for `sta abs,x`) with the direct page on the shadow, entered
    ; part way so that only the stores wanted run. Sprites 64 and on of the
    ; range go first, with the direct page 256 bytes further.
    tay                 ; Y = n (3 to 128)
    lda 6,s             ; first
    asl a
    asl a
    tax                 ; X = first * 4
    phd
    cpy.w #65
    bcc _ohr_lo1
    lda.w #oamMemory + 256
    tcd
    pea _ohr_lo1 - 1    ; where the run returns
    tya
    eor.w #$FFFF
    sec
    adc.w #128          ; 64 - (n - 64)
    asl a               ; 2 bytes a store (carry clear: under 128)
    adc.w #_ohr_run - 1 ; rts adds 1
    pha
    lda.w #$F001        ; y 240 over x 1
    rts                 ; into _ohr_run, same bank
_ohr_lo1:
    lda.w #oamMemory
    tcd
    pea _ohr_lo2 - 1
    tya
    cmp.w #65
    bcc +
    lda.w #64
+   eor.w #$FFFF
    sec
    adc.w #64           ; 64 - min(n, 64)
    asl a
    adc.w #_ohr_run - 1
    pha
    lda.w #$F001
    rts
_ohr_lo2:
    pld

    ; the high table, a word (eight sprites) at a time
    lda 6,s             ; first
    and.w #7
    asl a
    tax
    lda.l oam_hide_head,x
    sta.b tcc__r10      ; mask of the start word
    lda.b tcc__r9
    dec a
    and.w #7
    asl a
    tax
    lda.l oam_hide_tail,x
    sta.b tcc__r10+2    ; mask of the end word
    lda.b tcc__r9
    dec a
    lsr a
    lsr a
    and.w #$FFFE
    sta.b tcc__r9+2     ; byte of the end word
    lda 6,s             ; first
    lsr a
    lsr a
    and.w #$FFFE
    tax                 ; X = byte of the start word
    cpx.b tcc__r9+2
    bne +
    lda.b tcc__r10      ; one word: both edges
    and.b tcc__r10+2
    ora.w oamMemory+512,x
    sta.w oamMemory+512,x
    brl _ohr_done
+   lda.b tcc__r10
    ora.w oamMemory+512,x
    sta.w oamMemory+512,x
    inx
    inx
    ; the m whole words between (0 to 14): enter the 14 copies at 14 - m
    txa
    eor.w #$FFFF
    sec
    adc.b tcc__r9+2     ; 2 * m
    bne +
    brl _ohr_tail
+   lsr a
    eor.w #$FFFF
    sec
    adc.w #14           ; 14 - m
    sta.b tcc__r0
    asl a
    asl a
    asl a
    adc.b tcc__r0       ; (14 - m) * 9, a copy is 9 bytes (carry clear)
    adc.w #_ohr_mid - 1
    pha
    rts                 ; into _ohr_mid, same bank
_ohr_mid:
    .REPEAT 14 INDEX OHM
    lda.w oamMemory + 512 + (13 - OHM) * 2,x
    ora.w #$5555
    sta.w oamMemory + 512 + (13 - OHM) * 2,x
    .ENDR
    ldx.b tcc__r9+2     ; the end word
_ohr_tail:
    lda.b tcc__r10+2
    ora.w oamMemory+512,x
    sta.w oamMemory+512,x
_ohr_done:
    sep #$20
    .ACCU 8
    lda.b tcc__r9
    dec a               ; highest id written
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id
+   lda.b #1
    sta.w oam_update_flag
    rep #$20
    .ACCU 16
    rtl

; The masks of the two edge words, in this section (read long): a table in a
; section of its own (SEMISUPERFREE) is kept by `wlalink -d` in every ROM
; that links this file, called or not.
oam_hide_head:                  ; the x bits of sprites first & 7 to 7 of a word
    .dw $5555, $5554, $5550, $5540, $5500, $5400, $5000, $4000
oam_hide_tail:                  ; the x bits of sprites 0 to (end - 1) & 7
    .dw $0001, $0005, $0015, $0055, $0155, $0555, $1555, $5555

; 64 stores of A at sprites 63 down to 0 from D + X; entered part way
_ohr_run:
    .REPEAT 64 INDEX OHR
    sta.b 252 - OHR * 4,x
    .ENDR
    rts

.ENDS

;------------------------------------------------------------------------------
; void oamSetX(u16 id, u16 x)
;   6,s id   4,s x
;------------------------------------------------------------------------------
.SECTION ".text.oamSetX" SUPERFREE

oamSetX:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s             ; id
    cmp.w #128
    bcs @none           ; not a sprite: nothing is written
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    lda.l oam_ext_tab+2,x
    tay                 ; Y = id >> 2: the sprite's byte of the high table
    sep #$20
    .ACCU 8
    lda 4,s             ; x low byte
    sta.l oamMemory,x
    lda 5,s             ; x high byte
    lsr a               ; carry = bit 8 of x
    bcs @x_high
    lda.l oam_ext_tab+1,x
    sta.w oam_update_flag ; any non-zero value: a mask is never 0
    and.w oamMemory+512,y
@x_done:
    sta.w oamMemory+512,y
    lda 6,s             ; id (low byte: it is under 128)
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id     ; highest sprite written: the NMI sends up to it
+   rep #$20
    .ACCU 16
@none:
    rtl
@x_high:
    .ACCU 8
    lda.l oam_ext_tab,x
    sta.w oam_update_flag
    ora.w oamMemory+512,y
    bra @x_done

.ENDS

;------------------------------------------------------------------------------
; void oamSetY(u16 id, u16 y)
;   6,s id   4,s y
;------------------------------------------------------------------------------
.SECTION ".text.oamSetY" SUPERFREE

oamSetY:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s             ; id
    cmp.w #128
    bcs @none           ; not a sprite: nothing is written
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    sep #$20
    .ACCU 8
    lda 4,s             ; y low byte
    sta.l oamMemory+1,x
    lda #$01
    sta.w oam_update_flag
    lda 6,s             ; id (low byte: it is under 128)
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id     ; highest sprite written: the NMI sends up to it
+   rep #$20
    .ACCU 16
@none:
    rtl

.ENDS

;------------------------------------------------------------------------------
; void oamSetXY(u16 id, u16 x, u16 y)
;   8,s id   6,s x   4,s y
;------------------------------------------------------------------------------
.SECTION ".text.oamSetXY" SUPERFREE

oamSetXY:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 8,s             ; id
    cmp.w #128
    bcs @none           ; not a sprite: nothing is written
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    lda.l oam_ext_tab+2,x
    tay                 ; Y = id >> 2: the sprite's byte of the high table
    sep #$20
    .ACCU 8
    lda 6,s             ; x low byte
    sta.l oamMemory,x
    lda 4,s             ; y low byte
    sta.l oamMemory+1,x
    lda 7,s             ; x high byte
    lsr a               ; carry = bit 8 of x
    bcs @x_high
    lda.l oam_ext_tab+1,x
    sta.w oam_update_flag ; any non-zero value: a mask is never 0
    and.w oamMemory+512,y
@x_done:
    sta.w oamMemory+512,y
    lda 8,s             ; id (low byte: it is under 128)
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id     ; highest sprite written: the NMI sends up to it
+   rep #$20
    .ACCU 16
@none:
    rtl
@x_high:
    .ACCU 8
    lda.l oam_ext_tab,x
    sta.w oam_update_flag
    ora.w oamMemory+512,y
    bra @x_done

.ENDS

;------------------------------------------------------------------------------
; void oamSetSize(u16 id, u16 large)
;   6,s id   4,s large
;------------------------------------------------------------------------------
.SECTION ".text.oamSetSize" SUPERFREE

oamSetSize:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s             ; id
    cmp.w #128
    bcs @none           ; not a sprite: nothing is written
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    lda.l oam_ext_tab+2,x
    tay                 ; Y = id >> 2: the sprite's byte of the high table
    lda 4,s             ; large
    sep #$20            ; (leaves Z as the 16-bit load set it)
    .ACCU 8
    beq +
    lda.l oam_ext_tab,x
    asl a               ; the size bit is the x bit's neighbour
    ora.w oamMemory+512,y
    bra ++
+   lda.l oam_ext_tab,x
    asl a
    eor #$FF
    and.w oamMemory+512,y
++  sta.w oamMemory+512,y
    lda #$01
    sta.w oam_update_flag
    lda 6,s             ; id (low byte: it is under 128)
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id     ; highest sprite written: the NMI sends up to it
+   rep #$20
    .ACCU 16
@none:
    rtl

.ENDS
