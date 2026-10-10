;==============================================================================
; OpenSNES - oamPlaceWorld(): a batch of world-space sprites in one call
;==============================================================================
; A scrolling game keeps its sprites in world coordinates and, every frame,
; turns each into an OAM entry: subtract the camera, decide whether it is on
; screen, write x / y / tile / attributes and the ninth x bit, or hide it.
; In compiled C that loop cost about 6,000 master cycles a sprite on a real
; project (issue #165); it is the same loop for every scrolling game, so it
; is here, once, in assembly.
;
; void oamPlaceWorld(const OamWorldBatch *batch, u16 cam_x, u16 cam_y)
;
; Calling convention (cc65816): arguments pushed left to right; with the php
; below, cam_y is at 5,s, cam_x at 7,s, batch at 9,s (bank at 11,s).
;
; OamWorldBatch (sprite.h; the offsets are asserted there):
;    0 x        const s16 *   world x of each sprite's top-left corner
;    4 y        const s16 *   world y
;    8 tile     const u8 *    tile number, low byte
;   12 attr     const u8 *    attribute byte (vhoopppc)
;   16 visible  u8 *          out: 1 placed, 0 hidden; may be 0
;   20 first_id u8            OAM id of the first sprite
;   21 count    u8
;   22 size     u8            width and height in pixels
;   24 order    const u8 *    optional: slot i shows sprite order[i]; 0 = slot i
;                             shows sprite i
;
; Every tcc__r* is the callee's to use (compiler/ABI.md):
;   r0 x pointer, stepped by one a sprite so that [r0],y with Y = i reads
;   word i; r1 the same for y; r2 tile; r3 attr; r4 visible; r5 the
;   sprite's byte of the high table; r5h count; r9 batch; r10 size; r10h
;   the right limit. The bottom limit and the "visible wanted" flag are in
;   tcc__lf (below).
;   The two camera arguments are rewritten in place as camera - size.
;   Fourteen bytes of tcc__lf (the block the compiler gives to values that
;   do not live across a call: a callee may use it, see compiler/ABI.md):
;   the two values above and, with `order`, the order pointer, the slot,
;   the sprite it shows, and the count as given (what an order byte must
;   stay under). r0 and r1 are
;   then not stepped, the words being read at Y = sprite * 2.
;
; The OAM shadow and its flags are plain RAM below $2000, reached through
; the data bank (see sprite_oamset.asm).
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

.DEFINE OW_ORDER  tcc__lf+0     ; 4 bytes
.DEFINE OW_SLOT   tcc__lf+4
.DEFINE OW_SPRITE tcc__lf+6
.DEFINE OW_GIVEN  tcc__lf+8
.DEFINE OW_BOTTOM tcc__lf+10    ; both paths: the bottom limit, and
.DEFINE OW_FLAGS  tcc__lf+12    ; whether the caller wants visible[]

.SECTION ".text.oamPlaceWorld" SUPERFREE

oamPlaceWorld:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    lda 9,s             ; batch
    sta.b tcc__r9
    lda 11,s            ; batch (bank)
    sta.b tcc__r9+2

    ; the five pointers: 20 bytes of the batch into r0..r4 (spelled out: a
    ; loop costs half as much again, on every call)
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
    lda [tcc__r9],y
    sta.b tcc__r0+16
    ldy.w #18
    lda [tcc__r9],y
    sta.b tcc__r0+18

    ldy.w #20
    lda [tcc__r9],y     ; first_id, count
    pha
    and.w #$00FF
    cmp.w #128
    bcc +
    pla                 ; no sprite of that number
    plp
    rtl
+   sta.b tcc__r10      ; (first_id, for a moment)
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
+
    ldy.w #22
    lda [tcc__r9],y     ; size
    and.w #$00FF
    sta.b tcc__r10
    clc
    adc.w #256
    sta.b tcc__r10+2    ; x + size must be under this
    lda.b tcc__r10
    clc
    adc.w #224
    sta.b OW_BOTTOM     ; y + size must be under this
    lda.b tcc__r4
    ora.b tcc__r4+1     ; all 24 bits of `visible`
    beq +
    lda.w #1
+   sta.b OW_FLAGS      ; 1: the caller wants the flags
    ; The camera, less the size: world - that = screen position + size, the
    ; value the two limits are for. Kept where the arguments were.
    lda 7,s             ; cam_x
    sec
    sbc.b tcc__r10
    sta 7,s
    lda 5,s             ; cam_y
    sec
    sbc.b tcc__r10
    sta 5,s

    ldy.w #24
    lda [tcc__r9],y     ; order: all 24 bits (0: none)
    iny
    ora [tcc__r9],y
    beq +
    jmp @ordered
+
    ldy.w #0
@sprite:
    ; On screen, even partly, when 0 < position + size < limit + size: one
    ; unsigned compare a side. y first: its 16-bit store spills into the
    ; tile byte, which is written after it.
    lda [tcc__r1],y     ; world y
    sec
    sbc 5,s             ; cam_y - size
    beq @hidden
    cmp.b OW_BOTTOM
    bcs @hidden
    sec
    sbc.b tcc__r10      ; y on screen: the OAM byte as it is (docs/hardware/OAM.md)
    sta.w oamMemory+1,x

    lda [tcc__r0],y     ; world x
    sec
    sbc 7,s             ; cam_x - size
    beq @hidden
    cmp.b tcc__r10+2
    bcs @hidden
    sec
    sbc.b tcc__r10      ; x on screen, -size < x < 256: negative = left of the screen
    sep #$20            ; (leaves N as the subtraction set it)
    .ACCU 8
    sta.w oamMemory,x
    bmi @x_left
    lda.l oam_ext_tab+1,x
    and (tcc__r5)
    bra @x_done
@x_left:
    lda.l oam_ext_tab,x
    ora (tcc__r5)
@x_done:
    sta (tcc__r5)
    lda [tcc__r2],y     ; tile
    sta.w oamMemory+2,x
    lda [tcc__r3],y     ; attr
    sta.w oamMemory+3,x
    lda.b OW_FLAGS
    beq @next8
    sta [tcc__r4],y     ; visible[i] = 1
    bra @next8

@hidden:
    .ACCU 16
    ; as oamHide(): x = 257 (low byte 1, ninth bit set), y = 240
    lda.w #$F001
    sta.w oamMemory,x
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora (tcc__r5)
    sta (tcc__r5)
    lda.b OW_FLAGS
    beq @next8
    lda #0
    sta [tcc__r4],y     ; visible[i] = 0

@next8:
    rep #$20
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
    inc.b tcc__r5       ; four sprites to a byte of the high table
+   iny
    cpy.b tcc__r5+2
    bcs +
    jmp @sprite
+
@done:
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

    ;--------------------------------------------------------------------------
    ; With `order`: slot i shows sprite order[i]. The game's arrays stay
    ; indexed by the sprite and never move; a depth sort permutes `order`
    ; alone. The same tests and the same writes as above, the sprite's data
    ; read at its own index. visible[] is indexed by the sprite too: "is
    ; sprite n on screen" is the question the game asks. An order byte that
    ; is not under `count` hides the slot and touches no array.
    ;--------------------------------------------------------------------------
@ordered:
    .ACCU 16
    ldy.w #24
    lda [tcc__r9],y
    sta.b OW_ORDER
    ldy.w #26
    lda [tcc__r9],y
    sta.b OW_ORDER+2
    ldy.w #21
    lda [tcc__r9],y
    and.w #$00FF
    sta.b OW_GIVEN      ; count as given: the length of the caller's arrays
    stz.b OW_SLOT
@oslot:
    ldy.b OW_SLOT
    lda [OW_ORDER],y     ; order[slot] (and the byte after it)
    and.w #$00FF
    cmp.b OW_GIVEN
    bcs @obad
    sta.b OW_SPRITE     ; the sprite this slot shows
    asl a
    tay                 ; its word in x[] and y[]

    lda [tcc__r1],y     ; world y
    sec
    sbc 5,s             ; cam_y - size
    beq @ohidden
    cmp.b OW_BOTTOM
    bcs @ohidden
    sec
    sbc.b tcc__r10
    sta.w oamMemory+1,x

    lda [tcc__r0],y     ; world x
    sec
    sbc 7,s             ; cam_x - size
    beq @ohidden
    cmp.b tcc__r10+2
    bcs @ohidden
    sec
    sbc.b tcc__r10
    sep #$20
    .ACCU 8
    sta.w oamMemory,x
    bmi @ox_left
    lda.l oam_ext_tab+1,x
    and (tcc__r5)
    bra @ox_done
@ox_left:
    lda.l oam_ext_tab,x
    ora (tcc__r5)
@ox_done:
    sta (tcc__r5)
    ldy.b OW_SPRITE     ; the sprite's byte in tile[], attr[], visible[]
    lda [tcc__r2],y     ; tile
    sta.w oamMemory+2,x
    lda [tcc__r3],y     ; attr
    sta.w oamMemory+3,x
    lda.b OW_FLAGS
    beq @onext8
    sta [tcc__r4],y     ; visible[sprite] = 1
    bra @onext8

@ohidden:
    .ACCU 16
    lda.w #$F001        ; as oamHide(): x = 257, y = 240
    sta.w oamMemory,x
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora (tcc__r5)
    sta (tcc__r5)
    lda.b OW_FLAGS
    beq @onext8
    ldy.b OW_SPRITE
    lda #0
    sta [tcc__r4],y     ; visible[sprite] = 0
    bra @onext8

@obad:
    .ACCU 16
    lda.w #$F001        ; no such sprite: the slot is hidden, no array is touched
    sta.w oamMemory,x
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora (tcc__r5)
    sta (tcc__r5)

@onext8:
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
+   inc.b OW_SLOT
    lda.b OW_SLOT
    cmp.b tcc__r5+2
    bcs +
    jmp @oslot
+   jmp @done

.ENDS
