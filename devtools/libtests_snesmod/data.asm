; Raw IRQ handler for the snesmodInit / NMITIMEN vector: acknowledge TIMEUP,
; count, rti (same contract as devtools/libtests/data.asm).
.section ".irq_test" semifree bank 0

irqTestHandler:
    rep #$20
    .ACCU 16
    pha
    sep #$20
    .ACCU 8
    lda.l $004211           ; TIMEUP: acknowledge the IRQ line
    rep #$20
    .ACCU 16
    lda.l irq_count
    inc a
    sta.l irq_count
    pla
    rti

.ends
