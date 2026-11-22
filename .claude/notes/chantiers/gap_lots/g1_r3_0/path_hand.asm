k_path_asm:
    lda.w pa_lo
    sta.b PL0
    lda.w pa_hi
    sta.b PH0
    lda.w pa_lo+2
    sta.b PL1
    lda.w pa_hi+2
    sta.b PH1
    lda.w pa_lo+4
    sta.b PL2
    lda.w pa_hi+4
    sta.b PH2
    lda.w pa_lo+6
    sta.b PL3
    lda.w pa_hi+6
    sta.b PH3
    lda.w pa_step
    asl a
    sta.b PST
    lda.w pa_idx
    asl a
    tax
    ldy.w #NSUB
@loop:
    lda.l path_sin,x
    sta.b PS
    cmp.w #$8000                ; carry = sign
    lda.w #0
    sbc.w #0                    ; 0 if negative, $FFFF if not
    eor.w #$FFFF
    sta.b PSX
    txa
    clc
    adc.b PST
    and.w #$007E
    tax
    lda.b PL0
    clc
    adc.b PS
    sta.b PL0
    lda.b PH0
    adc.b PSX
    sta.b PH0
    lda.b PL1
    sec
    sbc.b PS
    sta.b PL1
    lda.b PH1
    sbc.b PSX
    sta.b PH1
    lda.b PS
    asl a
    clc
    adc.b PL2
    sta.b PL2
    lda.b PH2
    adc.b PSX
    sta.b PH2
    lda.b PS
    cmp.w #$8000
    ror a
    clc
    adc.b PL3
    sta.b PL3
    lda.b PH3
    adc.b PSX
    sta.b PH3
    dey
    bne @loop
    lda.b PL0
    sta.w pa_lo
    lda.b PH0
    sta.w pa_hi
    lda.b PL1
    sta.w pa_lo+2
    lda.b PH1
    sta.w pa_hi+2
    lda.b PL2
    sta.w pa_lo+4
    lda.b PH2
    sta.w pa_hi+4
    lda.b PL3
    sta.w pa_lo+6
    lda.b PH3
    sta.w pa_hi+6
    txa
    lsr a
    sta.w pa_idx
    rtl
