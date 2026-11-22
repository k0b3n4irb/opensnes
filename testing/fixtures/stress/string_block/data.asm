; The string_block fixture's assembly: a 4 KB source table in ROM (an asset
; bank, never bank $00), the data bank register read back, the NMI turned
; off and on around the timed calls, and the mark the timing reads.

ASSET_SECTION "sb_rom"
sb_rom:
.rept 4112 INDEX i
    .db (i * 13 + 7) & $FF
.endr
.ENDS

.SECTION ".sb_helpers" SUPERFREE

; u16 sb_dbr(void): the data bank register, in the low byte
sb_dbr:
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

; void sb_quiet(void) / sb_loud(void): NMI off / back as the shadow says
sb_quiet:
    php
    sep #$20
    .ACCU 8
    lda #$00
    sta.l $004200
    plp
    rtl

sb_loud:
    php
    sep #$20
    .ACCU 8
    lda.l nmitimen_shadow
    sta.l $004200
    plp
    rtl

; void sb_mark(void): the timing reads the master cycles at each arrival here
sb_mark:
    rtl

.ENDS
