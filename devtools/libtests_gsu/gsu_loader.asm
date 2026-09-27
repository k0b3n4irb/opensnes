;==============================================================================
; The GSU job binary and the Game Pak RAM readback for main.c
;==============================================================================
.ifdef SUPERFX
ASSET_SECTION ".gsu_job"
gsu_job:
    .incbin "gsu_job.sfx.bin"
gsu_job_end:
.ENDS
.endif

.ifdef SUPERFX
.RAMSECTION ".gsu_results" BANK 0 SLOT 1
r_sum:    dsb 2
r_outer:  dsb 2
r_marker: dsb 2
.ENDS

.SECTION ".gsu_readback" SEMIFREE
.ACCU 16
.INDEX 16
; gsuJobReadResults — Game Pak RAM $70:0000-0005 -> r_sum, r_outer, r_marker.
; Call after gsuWait(), when the CPU owns the RAM again.
gsuJobReadResults:
    php
    rep #$20
    .ACCU 16
    lda.l $700000
    sta.l r_sum
    lda.l $700002
    sta.l r_outer
    lda.l $700004
    sta.l r_marker
    plp
    rtl
.ENDS
.endif
