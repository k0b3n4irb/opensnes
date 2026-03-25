;==============================================================================
; GSU Code Loader — minimal per-example file
;==============================================================================
; Only contains:
; 1. .incbin of the GSU binary (example-specific)
; 2. gsuSetProgram — exports the GSU binary address to the library
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
.SECTION ".gsu_code" SUPERFREE
gsu_program:
    .incbin "gsu_3d.sfx.bin"
gsu_program_end:
.ENDS

;------------------------------------------------------------------------------
; Example-specific RAM variables
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_example_vars" BANK 0 SLOT 1
edge_buffer: dsb 48         ; 12 edges × 4 bytes (x0,y0,x1,y1)
.ENDS

;------------------------------------------------------------------------------
; gsuSetProgram — tell the library where the GSU binary lives
;------------------------------------------------------------------------------
.SECTION ".gsu_set_program" SEMIFREE
.ACCU 16
.INDEX 16
gsuSetProgram:
    php
    sep #$20
    .ACCU 8
    lda #:gsu_program
    sta.l gsu_prog_bank
    rep #$20
    .ACCU 16
    lda #gsu_program
    sta.l gsu_prog_addr
    plp
    rtl
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

.endif
