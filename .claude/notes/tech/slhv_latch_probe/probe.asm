.RAMSECTION ".latch_results" BANK 0 SLOT 1
r_first:  dsb 2
r_second: dsb 2
r_done:   dsb 2
.ENDS
.SECTION ".latch_probe" SEMIFREE
.ACCU 16
.INDEX 16
; STAT78 once, then two SLHV latches ~130 lines apart with NO STAT78 in
; between; OPVCT read twice each time (low, then bit 8), so the read-twice
; flip-flop is back on the low byte for the second latch.
latchProbe:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    lda.l $00213F
    lda.l $002137
    lda.l $00213D
    sta.l r_first
    lda.l $00213D
    and #$01
    sta.l r_first+1
    ldx #$6000
-   dex
    bne -
    lda.l $002137
    lda.l $00213D
    sta.l r_second
    lda.l $00213D
    and #$01
    sta.l r_second+1
    rep #$20
    .ACCU 16
    lda #$D0E5
    sta.l r_done
    plp
    rtl
.ENDS
