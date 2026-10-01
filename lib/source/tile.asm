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
