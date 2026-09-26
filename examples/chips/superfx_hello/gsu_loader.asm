;==============================================================================
; GSU program + result readback
;==============================================================================
; The launch itself is the library's (gsuSetProgram + gsuLaunch, called from
; main.c): it runs the GSU from a WRAM loop and keeps interrupts alive during
; the job. Until 2026-09-26 this file carried its own WRAM launcher, which
; disabled NMI for the job and re-enabled it with a hardcoded $81.
;==============================================================================

.ifdef SUPERFX

;------------------------------------------------------------------------------
; GSU program binary
;------------------------------------------------------------------------------
ASSET_SECTION ".gsu_code"
gsu_program:
    .incbin "gsu_hello.sfx.bin"
gsu_program_end:
.ENDS

;------------------------------------------------------------------------------
; WRAM area + result variables
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_vars" BANK 0 SLOT 1
gsu_result: dsb 2
gsu_sram_byte0: dsb 1
gsu_sram_byte1: dsb 1
gsu_sram_word: dsb 2
gsu_fmult_test1: dsb 2     ; FMULT 2.0*2.0 result (expected $4000)
gsu_fmult_test2: dsb 2     ; FMULT 1.5*3.0 result (expected $4800)
.ENDS

;------------------------------------------------------------------------------
; launchGSU + WRAM stub (same section for label arithmetic)
;------------------------------------------------------------------------------
.SECTION ".gsu_readback" SEMIFREE
.ACCU 16
.INDEX 16
; gsuHelloReadResults — copy what the GSU program left in R0 and in the
; first bytes of Game Pak RAM into the WRAM variables main.c prints. Call it
; after gsuLaunch() returns (the CPU owns the cartridge again).
gsuHelloReadResults:
    php
    rep #$20
    .ACCU 16
    lda.l $3000              ; GSU R0
    sta.l gsu_result
    sep #$20
    .ACCU 8
    lda.l $700000            ; SRAM[0]
    sta.l gsu_sram_byte0
    lda.l $700001            ; SRAM[1]
    sta.l gsu_sram_byte1
    rep #$20
    .ACCU 16
    lda.l $700002            ; SRAM[2..3] (STW test)
    sta.l gsu_sram_word
    lda.l $700004            ; SRAM[4..5] (FMULT test 1)
    sta.l gsu_fmult_test1
    lda.l $700006            ; SRAM[6..7] (FMULT test 2)
    sta.l gsu_fmult_test2
    plp
    rtl
.ENDS

.endif
