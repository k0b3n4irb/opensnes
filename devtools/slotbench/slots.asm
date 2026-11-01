; slotsUpdate in assembly: what the loop of a library module would cost.
; The same decision as the C one in main.c; the push stays vramQueuePushSprite.
; Bench only, not the API of a module. A leaves 16-bit; caller pops its args.
.SECTION ".slots_update" SUPERFREE

slotsUpdate:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    ldy.w slot_count
    ldx.w slot_next
@loop:
    sep #$20
    .ACCU 8
    lda.w slot_want,x
    cmp.w slot_sent,x
    rep #$20
    .ACCU 16
    beq @same
    lda.w vram_queue_cost
    cmp.w slot_budget
    beq +
    bcs @stop
+   phy
    phx
    lda.w slot_want,x
    and.w #$00FF
    asl a
    asl a
    tax
    lda.l frame_src+2,x
    pha
    lda.l frame_src,x
    pha
    lda 5,s             ; the slot
    asl a
    tax
    lda.w slot_vram,x
    pha
    pea 32
    jsl vramQueuePushSprite
    pla
    pla
    pla
    pla
    plx
    ply
    sep #$20
    .ACCU 8
    lda.w slot_want,x
    sta.w slot_sent,x
    rep #$20
    .ACCU 16
@same:
    inx
    cpx.w slot_count
    bne +
    ldx.w #0
+   dey
    bne @loop
@stop:
    stx.w slot_next
    plp
    rtl

.ENDS
