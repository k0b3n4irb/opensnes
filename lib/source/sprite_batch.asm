;==============================================================================
; OpenSNES - oamSetBatch(): N sprites from parallel arrays, in one call
;==============================================================================
; A game with many identical objects (bullets, particles, a swarm) writes the
; same four OAM bytes for each of them every frame. Called per object, that
; is a call (or the oamSetFast() macro) a sprite; here it is one loop over
; the game's arrays, with no call, no argument frame and no ninth-bit shift
; loop per sprite: the mask comes from oam_ext_tab, as in oamSetXY().
;
; void oamSetBatch(const OamBatch *batch)
;
; Calling convention (cc65816): one argument, 4 bytes; with the php below it
; is at 5,s (bank at 7,s).
;
; OamBatch (sprite.h; the offsets are asserted there):
;    0 x        const s16 *   screen x of each sprite, 0-511 as oamSetXY() takes it
;    4 y        const s16 *   screen y (the low byte is the OAM byte)
;    8 tile     const u8 *    tile number, low byte; 0: the tile bytes stay as they are
;   12 attr     const u8 *    attribute byte (vhoopppc); 0: they stay as they are
;   16 first_id u8            OAM id of the first sprite
;   17 count    u8
;
; Two loops. Neither tile nor attr: Y = 2 * sprite, nothing stepped, only
; x, y and the ninth x bit are written (the tile and attribute bytes are not
; touched). Otherwise: x and y are stepped by one a sprite so that [r0],y
; with Y = sprite reads word sprite (as sprite_world.asm), and tile / attr are
; read at Y and written when their pointer is not 0.
;
; Scratch (every tcc__r* is the callee's; only r0-r5, r9 and r10 exist): r0 x, r1 y,
; r2 tile, r3 attr, r4 tile wanted, r4h attr wanted, r5 the sprite's byte of the
; high table, r5h count (clamped), r9 batch, r10 2 * count.
;
; The OAM shadow and its flags are plain RAM below $2000, reached through the
; data bank (see sprite_oamset.asm).
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

.SECTION ".text.oamSetBatch" SUPERFREE

oamSetBatch:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    lda 5,s             ; batch
    sta.b tcc__r9
    lda 7,s             ; batch (bank)
    sta.b tcc__r9+2

    ; the four pointers: 16 bytes of the batch into r0..r3
    lda [tcc__r9]
    sta.b tcc__r0
    ldy.w #2
    lda [tcc__r9],y
    sta.b tcc__r0+2
    ldy.w #4
    lda [tcc__r9],y
    sta.b tcc__r0+4
    ldy.w #6
    lda [tcc__r9],y
    sta.b tcc__r0+6
    ldy.w #8
    lda [tcc__r9],y
    sta.b tcc__r0+8
    ldy.w #10
    lda [tcc__r9],y
    sta.b tcc__r0+10
    ldy.w #12
    lda [tcc__r9],y
    sta.b tcc__r0+12
    ldy.w #14
    lda [tcc__r9],y
    sta.b tcc__r0+14

    ldy.w #16
    lda [tcc__r9],y     ; first_id, count
    pha
    and.w #$00FF
    cmp.w #128
    bcc +
    pla                 ; no sprite of that number
    plp
    rtl
+   sta.b tcc__r10       ; (first_id, for a moment)
    asl a
    asl a
    tax                 ; X = first_id * 4: the entry, and its row of oam_ext_tab
    lda.b tcc__r10
    lsr a
    lsr a
    clc
    adc.w #oamMemory+512
    sta.b tcc__r5       ; the byte of the high table that holds this sprite
    pla
    xba
    and.w #$00FF        ; count
    clc
    adc.b tcc__r10
    cmp.w #129
    bcc +
    lda.w #128          ; the batch runs past sprite 127: stop there
+   sec
    sbc.b tcc__r10
    sta.b tcc__r5+2     ; count, clamped
    bne +
    plp                 ; nothing to place
    rtl
+   asl a
    sta.b tcc__r10       ; 2 * count: the end of Y in the first loop

    lda.b tcc__r2
    ora.b tcc__r2+1     ; all 24 bits of `tile`
    beq +
    lda.w #1
+   sta.b tcc__r4
    lda.b tcc__r3
    ora.b tcc__r3+1
    beq +
    lda.w #1
+   sta.b tcc__r4h
    ora.b tcc__r4
    bne @general

    ;--------------------------------------------------------------------------
    ; x, y and the ninth x bit only
    ;--------------------------------------------------------------------------
    ldy.w #0
@xy:
    .ACCU 16
    lda [tcc__r1],y     ; y
    sep #$20
    .ACCU 8
    sta.w oamMemory+1,x
    rep #$20
    .ACCU 16
    lda [tcc__r0],y     ; x
    sep #$20
    .ACCU 8
    sta.w oamMemory,x
    xba
    lsr a               ; carry = bit 8 of x
    bcs @xy_high
    lda.l oam_ext_tab+1,x
    and (tcc__r5)
    bra @xy_done
@xy_high:
    lda.l oam_ext_tab,x
    ora (tcc__r5)
@xy_done:
    sta (tcc__r5)
    rep #$20
    .ACCU 16
    inx
    inx
    inx
    inx
    txa
    and.w #$000C
    bne +
    inc.b tcc__r5       ; four sprites to a byte of the high table
+   iny
    iny
    cpy.b tcc__r10
    bcc @xy
    bra @done

    ;--------------------------------------------------------------------------
    ; x, y, the ninth x bit, and tile and / or attr
    ;--------------------------------------------------------------------------
@general:
    ldy.w #0
@gen:
    .ACCU 16
    lda [tcc__r1],y     ; y
    sep #$20
    .ACCU 8
    sta.w oamMemory+1,x
    rep #$20
    .ACCU 16
    lda [tcc__r0],y     ; x
    sep #$20
    .ACCU 8
    sta.w oamMemory,x
    xba
    lsr a
    bcs @gen_high
    lda.l oam_ext_tab+1,x
    and (tcc__r5)
    bra @gen_done
@gen_high:
    lda.l oam_ext_tab,x
    ora (tcc__r5)
@gen_done:
    sta (tcc__r5)
    lda.b tcc__r4
    beq +
    lda [tcc__r2],y
    sta.w oamMemory+2,x
+   lda.b tcc__r4h
    beq +
    lda [tcc__r3],y
    sta.w oamMemory+3,x
+   rep #$20
    .ACCU 16
    inc.b tcc__r0       ; one more byte: with Y, two a sprite
    inc.b tcc__r1
    inx
    inx
    inx
    inx
    txa
    and.w #$000C
    bne +
    inc.b tcc__r5
+   iny
    cpy.b tcc__r5+2
    bcc @gen

@done:
    .ACCU 16
    ; the table changed, up to the last sprite of the batch
    txa
    lsr a
    lsr a
    dec a
    sep #$20
    .ACCU 8
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id
+   lda #1
    sta.w oam_update_flag
    plp
    rtl

.ENDS
