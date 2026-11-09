; koff_delay - burn koff_n loops of 5 CPU cycles (dex / bne), then return.
;
; The koff_hold fixture gives koff_n a pseudo-random value of 0-127 before
; each audioStopVoices(), so that the sound CPU writes KOFF at every phase of
; the S-DSP's two-sample KOFF period (64 SPC700 cycles; one step here is
; about 36 master cycles, under two SPC700 cycles). Reads koff_n (a C
; global); no argument on the stack, no register is left changed.

.section ".koff_delay" semifree bank 0

koff_delay:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    pha
    phx
    lda.l koff_n
    tax
    beq @out
@loop:
    dex
    bne @loop
@out:
    plx
    pla
    plp
    rtl

.ends
