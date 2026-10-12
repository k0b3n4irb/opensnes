;==============================================================================
; tile — build SNES tiles from pixels at run time (see snes/tile.h)
;==============================================================================
; void tileEncode2bpp(const u8 *pixels, u8 *tile)
; void tileEncode4bpp(const u8 *pixels, u8 *tile)
; void tileEncode8bpp(const u8 *pixels, u8 *tile)
;
; Stack at entry (php): 5,s = tile (4-byte pointer), 9,s = pixels (4-byte
; pointer), pushed left to right. The work frame is on the stack and
; becomes the direct page for the call (phd / tcd), so the function needs
; no global scratch and is reentrant.
;
; Per pixel, row by row: the byte is shifted right eight times, each bit
; rolled into its plane's accumulator (lsr / rol), so after a row of eight
; pixels accumulator n holds plane n's byte with pixel 0 in bit 7. Then the
; row's bytes go to tile[pair * 16 + row * 2 + {0, 1}] for each plane pair
; the depth has. About 47 000 master cycles a tile, whatever the depth (all
; eight planes are shifted every time); the C loop it replaces took ~310 000
; (luna profile on sprite_swarm, 2026-09-29), most of it in `0x80 >> col`.
;
; Frame (direct page while the call runs):
;   $00-$07  plane accumulators
;   $08-$0A  pixels (24-bit)       $0B-$0D  tile (24-bit)
;   $0E      plane pairs (1, 2, 4) $0F      pixels left in the row
;   $10-$11  pixel index           $12-$13  row * 2
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

.SECTION ".tile_encode" SEMIFREE
.ACCU 16
.INDEX 16

tileEncode2bpp:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #1
    bra _tile_encode

tileEncode4bpp:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #2
    bra _tile_encode

tileEncode8bpp:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #4

_tile_encode:
    .ACCU 16
    .INDEX 16
    tax                         ; X = plane pairs
    phd
    tsc
    sec
    sbc #20
    tcs                         ; 20-byte frame
    inc a
    tcd                         ; D = the frame's first byte
    ; arguments, D-relative: N,s is dp N-1; tile 5+2+20 = 27,s -> $1A,
    ; pixels 31,s -> $1E
    lda $1E
    sta $08                     ; pixels, low 16
    lda $20
    sta $0A                     ; pixels bank ($0B is overwritten next)
    lda $1A
    sta $0B                     ; tile, low 16
    lda $1C
    sta $0D                     ; tile bank ($0E is overwritten next)
    txa
    sta $0E                     ; pairs, and $0F = 0
    stz $10                     ; pixel index
    stz $12                     ; row * 2
    sep #$20
    .ACCU 8
    ldy #$0000

@row:
    lda #8
    sta $0F
@pixel:
    lda [$08],y
    lsr a
    rol $00
    lsr a
    rol $01
    lsr a
    rol $02
    lsr a
    rol $03
    lsr a
    rol $04
    lsr a
    rol $05
    lsr a
    rol $06
    lsr a
    rol $07
    iny
    dec $0F
    bne @pixel

    sty $10                     ; next row's first pixel
    ldy $12                     ; this row's offset in the first pair
    ldx $0E                     ; pairs ($0F is 0 here)
    lda $00
    sta [$0B],y
    iny
    lda $01
    sta [$0B],y
    dex
    beq @row_done
    jsr _tile_next_pair
    lda $02
    sta [$0B],y
    iny
    lda $03
    sta [$0B],y
    dex
    beq @row_done
    jsr _tile_next_pair
    lda $04
    sta [$0B],y
    iny
    lda $05
    sta [$0B],y
    jsr _tile_next_pair
    lda $06
    sta [$0B],y
    iny
    lda $07
    sta [$0B],y
@row_done:
    rep #$20
    .ACCU 16
    lda $12
    inc a
    inc a
    sta $12
    sep #$20
    .ACCU 8
    ldy $10
    cpy #64
    bne @row

    rep #$20
    .ACCU 16
    tsc
    clc
    adc #20
    tcs
    pld
    plp
    rtl

; Y = the hi byte's offset in this pair -> the lo byte's offset in the next
; (16 bytes per pair: +15)
_tile_next_pair:
    .ACCU 8
    rep #$20
    .ACCU 16
    tya
    clc
    adc #15
    tay
    sep #$20
    .ACCU 8
    rts
.ENDS

;==============================================================================
; void tileBlitGlyph2bpp(u8 FAR *tile, u8 FAR *right, const u8 FAR *glyph,
;                        u8 glyph_row, u8 tile_row, u8 rows, u8 shift)
;
; ORs `rows` rows of a 2 bpp glyph (16 bytes, the two planes interleaved by
; row as a SNES tile) into `tile`, from the glyph's row `glyph_row` onto the
; tile's row `tile_row`, each row shifted right by `shift` pixels (0-7); the
; bits that leave the tile on the right land in `right` (NULL: dropped). A
; glyph put at a pixel position touches up to four tiles: two calls, one per
; tile row, cover it. This call is measured in the libtests report and on
; devtools/sdkbench `farfill`.
;
; Stack at entry (php), pushed left to right: 5,s = shift, 7,s = rows,
; 9,s = tile_row, 11,s = glyph_row, 13,s = glyph (4-byte pointer), 17,s =
; right, 21,s = tile. A 12-byte frame on the stack becomes the direct page
; (phd / tcd), as tileEncode*: no global scratch, reentrant.
;
; Per row and plane: the plane byte is put in the high byte of A (16-bit),
; then shifted right `shift` times through a jump into a chain of seven
; `lsr a`: the high byte is the part that stays in the tile, the low byte
; the part that crosses into `right` — one shift gives both. A zero part is
; not written (an OR of zero costs two long indirect accesses for nothing).
;
; Frame (direct page while the call runs):
;   $00-$02  tile (24-bit, + tile_row * 2)    $03-$05  right (same offset)
;   $06-$08  glyph (24-bit, + glyph_row * 2)  $09      right present and shift != 0
;   $0A      rows left                        $0B      unused
; Arguments, D-relative (N,s is dp N-1, +2 for phd, +12 for the frame):
;   shift $12, rows $14, tile_row $16, glyph_row $18, glyph $1A/$1C,
;   right $1E/$20, tile $22/$24
;==============================================================================
.SECTION ".tile_blit" SEMIFREE
.ACCU 16
.INDEX 16

tileBlitGlyph2bpp:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    phd
    tsc
    sec
    sbc #12
    tcs                         ; 12-byte frame
    inc a
    tcd                         ; D = the frame's first byte

    ; The three pointers, each with its row offset added (a buffer never
    ; crosses a bank; the carry is kept anyway). Every 16-bit store of a bank
    ; byte also writes the byte above it, so the order is tile, right, glyph,
    ; flag, rows: each store's spill is the next field, written after.
    lda $16                     ; tile_row
    and #$00FF
    asl a
    tax                         ; X = tile_row * 2, for right too
    clc
    adc $22                     ; tile, low 16
    sta $00
    lda $24                     ; tile bank
    adc #0
    sta $02                     ; $02 = bank ($03 next)
    txa
    clc
    adc $1E                     ; right, low 16 (NULL: a dead value)
    sta $03
    lda $20                     ; right bank
    adc #0
    sta $05                     ; $05 = bank ($06 next)
    lda $18                     ; glyph_row
    and #$00FF
    asl a
    clc
    adc $1A                     ; glyph, low 16
    sta $06
    lda $1C                     ; glyph bank
    adc #0
    sta $08                     ; $08 = bank ($09 next)

    stz $09                     ; no right part ($09; $0A next)
    lda $12                     ; shift
    and #$0007
    beq @no_right               ; shift 0: nothing crosses
    lda $1E                     ; right, low 16
    bne @has_right
    lda $20
    and #$00FF
    beq @no_right               ; NULL
@has_right:
    inc $09                     ; right present ($0A still 0)
@no_right:

    sep #$20
    .ACCU 8
    lda $14                     ; rows
    sta $0A
    rep #$20
    .ACCU 16
    lda $12                     ; shift
    and #$0007
    asl a
    tax                         ; X = shift * 2: the jump table index
    ldy #$0000                  ; Y = row * 2 in the tile and in the glyph

@row:
    ; ---- plane 0 ----
    lda [$06],y                 ; p1:p0
    and #$00FF
    xba                         ; A = p0 << 8
    jmp (@shift0,x)
@sh0_7: lsr a
@sh0_6: lsr a
@sh0_5: lsr a
@sh0_4: lsr a
@sh0_3: lsr a
@sh0_2: lsr a
@sh0_1: lsr a
@sh0_0:
    sep #$20
    .ACCU 8
    xba                         ; A = the part that stays; B = the part that crosses
    beq @p0_no_left
    ora [$00],y
    sta [$00],y
@p0_no_left:
    lda $09
    beq @p0_done
    xba                         ; A = the part that crosses
    beq @p0_done
    ora [$03],y
    sta [$03],y
@p0_done:
    rep #$20
    .ACCU 16

    ; ---- plane 1 ----
    lda [$06],y                 ; p1:p0 again
    and #$FF00                  ; A = p1 << 8
    jmp (@shift1,x)
@sh1_7: lsr a
@sh1_6: lsr a
@sh1_5: lsr a
@sh1_4: lsr a
@sh1_3: lsr a
@sh1_2: lsr a
@sh1_1: lsr a
@sh1_0:
    iny                         ; the plane-1 byte of the row
    sep #$20
    .ACCU 8
    xba
    beq @p1_no_left
    ora [$00],y
    sta [$00],y
@p1_no_left:
    lda $09
    beq @p1_done
    xba
    beq @p1_done
    ora [$03],y
    sta [$03],y
@p1_done:
    iny                         ; next row
    dec $0A
    beq @done
    rep #$20
    .ACCU 16
    bra @row

@done:
    rep #$20
    .ACCU 16
    tsc
    clc
    adc #12
    tcs
    pld
    plp
    rtl

; jump tables: entry n lands on the chain so that n `lsr a` run
@shift0:
    .dw @sh0_0, @sh0_1, @sh0_2, @sh0_3, @sh0_4, @sh0_5, @sh0_6, @sh0_7
@shift1:
    .dw @sh1_0, @sh1_1, @sh1_2, @sh1_3, @sh1_4, @sh1_5, @sh1_6, @sh1_7
.ENDS
