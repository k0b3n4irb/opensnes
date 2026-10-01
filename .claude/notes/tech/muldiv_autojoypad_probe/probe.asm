.RAMSECTION ".mul_results" BANK 0 SLOT 1
r_in_busy:  dsb 2     ; HVBJOY bit 0 was set when the product was read
r_ok_busy:  dsb 2     ; products correct while auto-read ran
r_bad_busy: dsb 2
r_ok_idle:  dsb 2     ; products correct outside auto-read
r_bad_idle: dsb 2
r_first_bad: dsb 2
r_done:     dsb 2
.ENDS
.SECTION ".mul_probe" SEMIFREE
.ACCU 16
.INDEX 16
; Main thread only, NMI off (no reentrancy): 2000 multiplies 123 x 45
; = 5535 at varying times; each read classified by HVBJOY bit 0 sampled
; right after the read.
mulProbe:
    php
    sep #$20
    .ACCU 8
    lda #$01                 ; auto-joypad ON, NMI OFF
    sta.l $004200
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #0
    sta.l r_in_busy
    sta.l r_ok_busy
    sta.l r_bad_busy
    sta.l r_ok_idle
    sta.l r_bad_idle
    sta.l r_first_bad
    ldy #2000
@loop:
    sep #$20
    .ACCU 8
    lda #123
    sta.l $004202
    lda #45
    sta.l $004203
    nop
    nop
    nop
    nop
    nop                      ; > 8 CPU cycles
    rep #$20
    .ACCU 16
    lda.l $004216
    tax
    sep #$20
    .ACCU 8
    lda.l $004212
    and #$01
    rep #$20
    .ACCU 16
    beq @idle
    lda.l r_in_busy
    inc a
    sta.l r_in_busy
    cpx #5535
    beq +
    lda.l r_bad_busy
    inc a
    sta.l r_bad_busy
    txa
    sta.l r_first_bad
    bra @next
+   lda.l r_ok_busy
    inc a
    sta.l r_ok_busy
    bra @next
@idle:
    cpx #5535
    beq +
    lda.l r_bad_idle
    inc a
    sta.l r_bad_idle
    bra @next
+   lda.l r_ok_idle
    inc a
    sta.l r_ok_idle
@next:
    ; spread the samples over the frame: ~80 + (Y*7 & 255) loops
    tya
    asl a
    asl a
    asl a
    and #$00FF
    clc
    adc #80
    tax
-   dex
    bne -
    dey
    bne @loop
    lda #$D0E5
    sta.l r_done
    plp
    rtl
.ENDS
