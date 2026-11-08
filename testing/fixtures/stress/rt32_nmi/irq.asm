; H-timer IRQ handler of the rt32_nmi fixture: acknowledges the timer and
; adds 1 to a 32-bit counter, on every scanline. irqSet takes ASM handlers
; only (interrupt.h): this one saves what it touches, and uses neither the
; compiler's runtime nor the hardware multiplier/divider ($4202-$4217),
; which the interrupted main thread may be in the middle of.

.section ".rt32_irq" semifree bank 0

rt32IrqHandler:
    rep #$20
    .ACCU 16
    pha                     ; interrupted C code is arbitrary: full 16-bit A
    sep #$20
    .ACCU 8
    lda.l $004211           ; TIMEUP: acknowledge the IRQ line
    rep #$20
    .ACCU 16
    lda.l irq_count
    clc
    adc.w #1
    sta.l irq_count
    lda.l irq_count+2
    adc.w #0
    sta.l irq_count+2
    pla
    rti

.ends
