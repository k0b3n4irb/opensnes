;----------------------------------------------------------------------
; hicolorIrqStream — per-scanline CGRAM palette stream (H-timer IRQ)
;
; krom (Peter Lemon)'s HTIMERIRQ from HiColor64PerTileRow, wrapped in
; the register save/restore his demo doesn't need (his IRQ only ever
; interrupts a WAI loop; ours interrupts arbitrary C code).
;
; Per scanline: acknowledge the IRQ, latch the V counter, and — below
; line 216 — fire a 16-byte general DMA into CGDATA ($2122) on channel
; 0. The channel's source address auto-advances across transfers, so
; over 8 lines a full 64-color set streams into CGRAM. On lines where
; (V & 15) == 8 the CGRAM address resets to 0: combined with the
; tilemap's alternating palette bits, even rows draw colors 0-63 while
; 64-127 refill, and vice versa. The VBlank callback (main.c) rewinds
; the stream each frame.
;
; Contract per interrupt.h: registered with irqSetBank(); this handler
; owns save/restore, the $4211 acknowledge, and the RTI.
;----------------------------------------------------------------------

; lint-asm-abi: skip-file raw IRQ handler, not a C-callable function

.section ".hicolor_irq" superfree

hicolorIrqStream:
    rep #$30
    .ACCU 16
    .INDEX 16
    pha                 ; full 16-bit A — interrupted C code is arbitrary
    phx
    phb
    phk                 ; DBR = program bank; LoROM banks $00-$3F all
    plb                 ; mirror the PPU/CPU registers at $2100-$43FF

    sep #$20
    .ACCU 8
    lda.w $4211         ; TIMEUP: acknowledge the IRQ line
    lda.w $213F         ; STAT78: reset the OPHCT/OPVCT read pointers —
                        ; arbitrary interrupted code may have left a
                        ; half-completed counter read (krom's demo owns
                        ; the whole machine and can skip this)
    lda.w $2137         ; SLHV: latch H/V counter
    lda.w $213D         ; OPVCT read 1: V counter low byte
    xba
    lda.w $213D         ; OPVCT read 2: bit 0 = V counter bit 8
    and.b #$01
    xba                 ; A = V low, B = V high
    tax                 ; X (16-bit) <- B:A = full scanline number
    .INDEX 16

    cpx.w #216          ; krom: no stream on lines 216+ (VBlank rewinds)
    bpl @skip

    and.b #$0F
    cmp.b #$08
    bne @dma
    stz.w $2121         ; (V & 15) == 8: CGRAM address back to color 0

@dma:
    ldx.w #16           ; 16 bytes = 8 colors this scanline
    stx.w $4305         ; DAS0
    lda.b #$01
    sta.w $420B         ; MDMAEN: fire channel 0 (source auto-advances)

@skip:
    plb
    rep #$30
    .ACCU 16
    .INDEX 16
    plx
    pla
    rti

.ends
