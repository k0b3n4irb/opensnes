; IRQ handler for the fixture: acknowledges the timer and counts. irqSet
; takes ASM handlers only (interrupt.h); same shape as devtools/libtests.

.section ".irq_probe" semifree bank 0

irqProbeHandler:
    rep #$20
    .ACCU 16
    pha                     ; interrupted C code is arbitrary: full 16-bit A
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
