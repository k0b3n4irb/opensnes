; 65816 8-bit decimal-mode ADC/SBC. Each case stores {result, P} to bcd_res.
; (memmap injected by wrap_asm — do not include it here.)
.SECTION ".text.bcd_probe" SUPERFREE
bcd_probe:
    phb
    php
    sep #$20
    .ACCU 8
    lda #$00
    pha
    plb                 ; DBR=0
    sed                 ; decimal mode ON

    clc                 ; case 0: 0x09 + 0x01 -> 0x10, C=0
    lda #$09
    adc #$01
    sta.w bcd_res+0
    php
    pla
    sta.w bcd_res+1

    clc                 ; case 1: 0x19 + 0x28 -> 0x47, C=0
    lda #$19
    adc #$28
    sta.w bcd_res+2
    php
    pla
    sta.w bcd_res+3

    clc                 ; case 2: 0x99 + 0x01 -> 0x00, C=1
    lda #$99
    adc #$01
    sta.w bcd_res+4
    php
    pla
    sta.w bcd_res+5

    clc                 ; case 3: 0x50 + 0x50 -> 0x00, C=1 (BCD 100)
    lda #$50
    adc #$50
    sta.w bcd_res+6
    php
    pla
    sta.w bcd_res+7

    clc                 ; case 4: 0x0A + 0x00 -> 0x10, C=0 (invalid nibble adjust)
    lda #$0A
    adc #$00
    sta.w bcd_res+8
    php
    pla
    sta.w bcd_res+9

    sec                 ; case 5: 0x50 - 0x25 -> 0x25, C=1 (no borrow)
    lda #$50
    sbc #$25
    sta.w bcd_res+10
    php
    pla
    sta.w bcd_res+11

    sec                 ; case 6: 0x00 - 0x01 -> 0x99, C=0 (borrow)
    lda #$00
    sbc #$01
    sta.w bcd_res+12
    php
    pla
    sta.w bcd_res+13

    cld                 ; restore before returning to C
    lda #$DE
    sta.w bcd_res+14
    lda #$C0
    sta.w bcd_res+15
    plp
    plb
    rtl
.ENDS
