; race_delay - burn race_n loops of 5 CPU cycles (dex / bne), then return.
;
; The wait_race fixture grows race_n by one per frame so that the store of
; WaitForVBlank sweeps every phase of the frame, the window between its `sta`
; and its `wai` included. Reads race_n (a C global); no argument on the stack,
; no register is left changed (P, X, A are saved).

.section ".race_delay" semifree bank 0

race_delay:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    pha
    phx
    lda.l race_n
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
