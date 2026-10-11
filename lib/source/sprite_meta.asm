;==============================================================================
; OpenSNES - the metasprite draw, in assembly
;==============================================================================
; oamDrawMetasprite(): one frame of a multi-piece sprite, piece by piece,
; into the OAM shadow. Until 2026-10-12 it was compiled C that called
; oamSet() and oamSetSize() for every piece (about 7,000 master cycles a
; piece on examples/sprites/metasprite: the library's heaviest function in
; the corpus profile, 2.6 % of its work). The loop below writes the four
; bytes and the two high-table bits of a piece itself.
;
; What it keeps from the C version, exactly: a frame ends at dx = -128 or
; when the id reaches 128; a piece whose x is outside -63..255 or whose y is
; outside -63..239 is skipped WITHOUT an id; a piece's palette field (bits
; 1-3 of attr) is the style's basePalette when it is 0; the tile is baseTile
; + the piece's tile and its bit 8 goes to the attribute's bit 0; a flip
; mirrors the piece inside the style's box (width - pieceSize - dx) and
; toggles its own flip bit; the highest id written goes to oam_max_id and
; oam_update_flag is set; the return value is the next free id.
;
; Calling convention (cc65816): arguments pushed left to right, 2 bytes for
; an integer and 4 for a pointer, the last one at 4,s. tcc__r0, tcc__r9 and
; the tcc__lf block are scratch for a callee (compiler/ABI.md).
;
; The OAM shadow and its two flags are plain RAM below $2000, reached here
; through the data bank (`.w`) as compiled C reaches its globals: bank $00
; at run time, $7E inside the object engine's callbacks, both of which map
; it. The frame and the style are read through their full 24-bit pointers.
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

.DEFINE MS_ID    tcc__lf+0      ; the next id to write
.DEFINE MS_BASE  tcc__lf+2      ; style->baseTile
.DEFINE MS_PAL   tcc__lf+4      ; style->basePalette, and +1 style->size
.DEFINE MS_SIZE  tcc__lf+5
.DEFINE MS_MIRX  tcc__lf+6      ; width - pieceSize (read only when flipping)
.DEFINE MS_MIRY  tcc__lf+8      ; height - pieceSize
.DEFINE MS_FLIP  tcc__lf+10     ; flip & $C0, high byte 0
.DEFINE MS_X     tcc__lf+12     ; the origin
.DEFINE MS_Y     tcc__lf+14
.DEFINE MS_SX    tcc__lf+16     ; the piece: dx, then x + dx
.DEFINE MS_SY    tcc__lf+18
.DEFINE MS_TILE  tcc__lf+20
.DEFINE MS_ITEM  tcc__lf+22     ; offset of the piece in the frame
.DEFINE MS_ATTR  tcc__lf+24     ; the attribute byte being composed
.DEFINE MS_MASK  tcc__lf+25     ; the piece's x bit in the high table
.DEFINE MS_HB    tcc__lf+26     ; the piece's high-table byte being composed

;------------------------------------------------------------------------------
; u16 oamDrawMetasprite(u16 startId, s16 x, s16 y, const MetaspriteItem *frame,
;                       const MetaspriteStyle *style, u8 flip)
;   18,s startId   16,s x   14,s y   10,s frame (4)   6,s style (4)   4,s flip
;------------------------------------------------------------------------------
.SECTION ".text.oamDrawMetasprite" SUPERFREE

oamDrawMetasprite:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 10,s            ; frame
    sta.b tcc__r9
    lda 12,s            ; frame (bank)
    sta.b tcc__r9+2
    lda 6,s             ; style
    sta.b tcc__r0
    lda 8,s             ; style (bank)
    sta.b tcc__r0+2
    lda 18,s            ; startId
    sta.b MS_ID
    lda 16,s            ; x
    sta.b MS_X
    lda 14,s            ; y
    sta.b MS_Y
    lda [tcc__r0]       ; baseTile
    sta.b MS_BASE
    ldy #2
    lda [tcc__r0],y     ; basePalette, size
    sta.b MS_PAL
    lda 4,s             ; flip
    and #$00C0
    sta.b MS_FLIP
    beq @loop_start
    ; the mirror of a piece: (extent - pieceSize) - d, computed once
    ldy #4
    lda [tcc__r0],y     ; pieceSize, width
    sta.b MS_MIRX
    ldy #6
    lda [tcc__r0],y     ; height, pad
    and #$00FF
    sta.b MS_MIRY
    lda.b MS_MIRX
    and #$00FF
    sta.b MS_SX         ; pieceSize, for the two subtractions
    lda.b MS_MIRX+1     ; width, height
    and #$00FF
    sec
    sbc.b MS_SX
    sta.b MS_MIRX
    lda.b MS_MIRY
    sec
    sbc.b MS_SX
    sta.b MS_MIRY
@loop_start:
    ldy #0
@loop:
    sty.b MS_ITEM
    lda [tcc__r9],y     ; dx
    cmp #$FF80          ; metasprite_end
    bne +
    brl @done
+   lda.b MS_ID
    cmp #128
    bcc +
    brl @done           ; the table is full: stop, as the C loop did
+
    lda [tcc__r9],y     ; dx
    sta.b MS_SX
    iny
    iny
    lda [tcc__r9],y     ; dy
    sta.b MS_SY
    lda.b MS_FLIP
    beq @place
    bit #$0040          ; OBJ_FLIPX
    beq +
    lda.b MS_MIRX
    sec
    sbc.b MS_SX
    sta.b MS_SX
    lda.b MS_FLIP
+   bit #$0080          ; OBJ_FLIPY
    beq @place
    lda.b MS_MIRY
    sec
    sbc.b MS_SY
    sta.b MS_SY
@place:
    lda.b MS_X
    clc
    adc.b MS_SX
    sta.b MS_SX         ; x of the piece
    clc
    adc #63
    cmp #319
    bcc +
    brl @next           ; off screen: x outside -63..255
+    lda.b MS_Y
    clc
    adc.b MS_SY
    sta.b MS_SY         ; y of the piece
    clc
    adc #63
    cmp #303
    bcc +
    brl @next           ; off screen: y outside -63..239
+    iny
    iny
    lda [tcc__r9],y     ; tile
    clc
    adc.b MS_BASE
    sta.b MS_TILE
    iny
    iny
    lda.b MS_ID
    asl a
    asl a
    tax                 ; X = id * 4: the sprite's entry, and its row of oam_ext_tab
    sep #$20
    .ACCU 8
    ; attributes, vhoopppc: the piece's own bits 1-7 (flip, priority,
    ; palette), the style's palette when the piece names none, bit 8 of
    ; the tile, and the whole-sprite flip toggling the piece's
    lda [tcc__r9],y     ; attr
    and #$FE
    sta.b MS_ATTR
    and #$0E            ; the palette field
    bne +
    lda.b MS_PAL        ; basePalette (0-7)
    asl a
    ora.b MS_ATTR
    sta.b MS_ATTR
+   lda.b MS_TILE+1
    and #$01
    ora.b MS_ATTR
    eor.b MS_FLIP
    sta.l oamMemory+3,x
    lda.b MS_TILE
    sta.l oamMemory+2,x
    lda.b MS_SX
    sta.l oamMemory,x
    lda.b MS_SY
    sta.l oamMemory+1,x
    ; the high table: bit 8 of x and the size, both bits of the sprite
    ; cleared then set as needed
    rep #$20
    .ACCU 16
    lda.l oam_ext_tab+2,x
    tay                 ; Y = id >> 2: the sprite's byte of the high table
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x ; the x bit
    sta.b MS_MASK
    asl a
    ora.b MS_MASK       ; the x bit and the size bit
    eor #$FF
    and.w oamMemory+512,y
    sta.b MS_HB
    lda.b MS_SX+1
    lsr a               ; carry = bit 8 of x
    bcc +
    lda.b MS_MASK
    ora.b MS_HB
    sta.b MS_HB
+   lda.b MS_SIZE
    beq +
    lda.b MS_MASK
    asl a               ; the size bit is the x bit's neighbour
    ora.b MS_HB
    sta.b MS_HB
+   lda.b MS_HB
    sta.w oamMemory+512,y
    lda #$01
    sta.w oam_update_flag
    rep #$20
    .ACCU 16
    inc.b MS_ID
@next:
    lda.b MS_ITEM
    clc
    adc #8              ; sizeof(MetaspriteItem)
    tay
    brl @loop

@done:
    lda.b MS_ID
    cmp 18,s            ; startId: nothing drawn, nothing to record
    beq @ret
    dec a               ; the highest id written
    sep #$20
    .ACCU 8
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id    ; the NMI sends up to it
+   rep #$20
    .ACCU 16
@ret:
    lda.b MS_ID         ; the next free id
    rtl

.ENDS
