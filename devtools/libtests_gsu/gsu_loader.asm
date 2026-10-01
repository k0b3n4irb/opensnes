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
r_mul:    dsb 2             ; SRAM[6..7], mul_job's product
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

; gsuMulReadResult — Game Pak RAM $70:0006 (mul_job's product) -> r_mul,
; leaving r_sum / r_marker as the four sum jobs left them.
gsuMulReadResult:
    php
    rep #$20
    .ACCU 16
    lda.l $700006
    sta.l r_mul
    plp
    rtl

; gsuJobClearResults — zero Game Pak RAM $70:0000-0005, so the next job has
; to write its results again. The CPU must own the RAM (no job running).
gsuJobClearResults:
    php
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $700000
    sta.l $700002
    sta.l $700004
    sta.l $700006
    plp
    rtl
.ENDS
.endif
