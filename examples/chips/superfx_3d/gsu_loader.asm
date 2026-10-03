;==============================================================================
; GSU Code Loader — minimal per-example file
;==============================================================================
; Only contains:
; 1. .incbin of the GSU binary (example-specific)
; 2. (gsuSetProgram lives in the library since 2026-09-26; main.c calls it)
; 3. Example-specific RAMSECTION (edge_buffer)
; 4. writeEdgesToSRAM — copies edge data to SRAM (example-specific)
;
; All generic SuperFX functions (WRAM stub, DMA, HDMA, tilemap) are
; now in the library (lib/source/superfx.asm).
;==============================================================================

.ifdef SUPERFX

;------------------------------------------------------------------------------
; GSU program binary
;------------------------------------------------------------------------------
GSU_SECTION ".gsu_code"
gsu_program:
    .incbin "gsu_3d.sfx.bin"
gsu_program_end:
.ENDS

;------------------------------------------------------------------------------
; Example-specific RAM variables
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_example_vars" BANK 0 SLOT 1
edge_buffer: dsb 48         ; 12 edges × 4 bytes (x0,y0,x1,y1)
    irq_ticks       dsb 2   ; V-timer IRQs handled by vtimer_irq (GSU idle)
.ENDS


;------------------------------------------------------------------------------
; writeEdgesToSRAM — Copy edge_buffer (48 bytes) to SRAM $70:4000
;------------------------------------------------------------------------------
.SECTION ".gsu_edges" SEMIFREE
.ACCU 16
.INDEX 16
writeEdgesToSRAM:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l edge_buffer,x
    sta.l $704000,x
    inx
    cpx #48
    bne -
    plp
    rtl
.ENDS

; vtimer_irq — a V-timer IRQ handler armed during the whole demo, to show
; (and let luna test) that an IRQ stays armed across GSU jobs. When the GSU
; is idle the vector reaches this ROM handler; during a job crt0's WRAM stub
; acknowledges the IRQ without it (ROM is unreadable then), so irq_ticks
; counts only the frames whose line 200 fell outside a job.
; Raw IRQ contract (interrupt.h): save what is touched, read $4211, rti.
.SECTION ".gsu_vtimer_irq" SEMIFREE
vtimer_irq:
    rep #$20
    .ACCU 16
    pha
    phb
    pea $0000
    plb
    plb                     ; DB = $00 for irq_ticks and $4211
    inc.w irq_ticks
    sep #$20
    .ACCU 8
    lda.w $4211             ; TIMEUP: acknowledge
    rep #$20
    .ACCU 16
    plb
    pla
    rti
.ENDS

.endif
