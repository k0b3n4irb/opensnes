; The world_place fixture's assembly: arrays in ROM (an asset bank, never
; bank $00) for oamPlaceWorld to read through 24-bit pointers, the NMI
; turned off and on, and the data bank read back.

ASSET_SECTION "wp_rom"
wp_rom_x:
.rept 144 INDEX i
    .dw ((i * 151 + 37) & $3FF) - 300
.endr
wp_rom_y:
.rept 144 INDEX i
    .dw ((i * 89 + 5) & $1FF) - 120
.endr
wp_rom_tile:
.rept 144 INDEX i
    .db (i * 7 + 3) & $FF
.endr
wp_rom_attr:
.rept 144 INDEX i
    .db (i * 29 + 1) & $FF
.endr
; an order table: a permutation of 0..127 by 37 (odd, so a bijection), with
; the bytes at or above a batch's count hiding their slot
wp_rom_order:
.rept 144 INDEX i
    .db (i * 37) & $7F
.endr
.ENDS

.SECTION ".wp_helpers" SUPERFREE

; void wp_quiet(void) / wp_loud(void): NMI off / back as the shadow says
wp_quiet:
    php
    sep #$20
    .ACCU 8
    lda #$00
    sta.l $004200
    plp
    rtl

; u16 wp_dbr(void): the data bank register, in the low byte
wp_dbr:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #$0000
    sep #$20
    .ACCU 8
    phb
    pla
    rep #$20
    .ACCU 16
    plp
    rtl

wp_loud:
    php
    sep #$20
    .ACCU 8
    lda.l nmitimen_shadow
    sta.l $004200
    plp
    rtl

.ENDS
