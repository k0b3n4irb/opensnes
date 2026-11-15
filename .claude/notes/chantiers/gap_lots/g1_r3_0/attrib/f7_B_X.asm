; flags {'P': 'hand', 'E': 'hand', 'F': 'hand', 'B': 'X', 'C': 'down'}
.SECTION ".gap_pathpp" SUPERFREE
.ACCU 16
.INDEX 16
.DEFINE ACC0L   tcc__lf+0
.DEFINE ACC0H   tcc__lf+2
.DEFINE ACC1L   tcc__lf+4
.DEFINE ACC1H   tcc__lf+6
.DEFINE ACC2L   tcc__lf+8
.DEFINE ACC2H   tcc__lf+10
.DEFINE ACC3L   tcc__lf+12
.DEFINE ACC3H   tcc__lf+14
.DEFINE STEP    tcc__lf+16
.DEFINE IDX     tcc__lf+18
.DEFINE S       tcc__lf+20
.DEFINE M       tcc__lf+22
.DEFINE TL      tcc__lf+24
.DEFINE TH      tcc__lf+26
k_pathpp_asm:
    rep #$20
    .ACCU 16
    .INDEX 16
    lda.w pa_lo
    sta.b ACC0L
    lda.w pa_hi
    sta.b ACC0H
    lda.w pa_lo+2
    sta.b ACC1L
    lda.w pa_hi+2
    sta.b ACC1H
    lda.w pa_lo+4
    sta.b ACC2L
    lda.w pa_hi+4
    sta.b ACC2H
    lda.w pa_lo+6
    sta.b ACC3L
    lda.w pa_hi+6
    sta.b ACC3H
    lda.w pa_step
    asl a
    sta.b STEP
    lda.w pa_idx
    asl a
    tax
    ldy #$0040
@for_body:
    lda.l path_sin,x
    sta.b S
    cmp #$8000
    lda #$0000
    sbc #$0000
    eor #$FFFF
    sta.b M
    lda.b S
    cmp #$8000
    ror a
    clc
    adc.b ACC3L
    sta.b ACC3L
    lda.b ACC3H
    adc.b M
    sta.b ACC3H
    lda.b S
    asl a
    clc
    adc.b ACC2L
    sta.b ACC2L
    lda.b ACC2H
    adc.b M
    sta.b ACC2H
    clc
    lda.b ACC0L
    adc.b S
    sta.b ACC0L
    lda.b ACC0H
    adc.b M
    sta.b ACC0H
    sec
    lda.b ACC1L
    sbc.b S
    sta.b ACC1L
    lda.b ACC1H
    sbc.b M
    sta.b ACC1H
    txa
    clc
    adc.b STEP
    and #$007E
    tax
    dey
    bne @for_body
    lda.b ACC0H
    sta.w pa_hi
    lda.b ACC0L
    sta.w pa_lo
    lda.b ACC1H
    sta.w pa_hi+2
    lda.b ACC1L
    sta.w pa_lo+2
    lda.b ACC2H
    sta.w pa_hi+4
    lda.b ACC2L
    sta.w pa_lo+4
    lda.b ACC3H
    sta.w pa_hi+6
    lda.b ACC3L
    sta.w pa_lo+6
    txa
    lsr a
    sta.w pa_idx
    rtl
.ENDS
