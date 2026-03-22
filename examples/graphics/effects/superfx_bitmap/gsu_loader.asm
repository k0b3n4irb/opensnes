;==============================================================================
; GSU Code Loader — WRAM-safe launch + SRAM readback
;==============================================================================
; ALL GSU launches MUST use the WRAM stub. The SNES CPU cannot read ROM
; while the GSU is running (RON=1). Even a 3-instruction GSU program can
; race the CPU's instruction prefetch.
;==============================================================================

.ifdef SUPERFX

;------------------------------------------------------------------------------
; GSU program binary
;------------------------------------------------------------------------------
.SECTION ".gsu_code" SUPERFREE
gsu_program:
    .incbin "gsu_render.sfx.bin"
gsu_program_end:
.ENDS

;------------------------------------------------------------------------------
; WRAM area + result variables
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_vars" BANK 0 SLOT 1
gsu_wram_area: dsb 64
gsu_result: dsb 2
gsu_sram_byte0: dsb 1
gsu_sram_byte1: dsb 1
gsu_sram_word: dsb 2
.ENDS

;------------------------------------------------------------------------------
; launchGSU + WRAM stub (same section for label arithmetic)
;------------------------------------------------------------------------------
.SECTION ".gsu_launcher" SEMIFREE

.ACCU 16
.INDEX 16

launchGSU:
    php

    ; Copy WRAM stub from ROM to WRAM
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l _wram_stub,x
    sta.l gsu_wram_area,x
    inx
    cpx #(_wram_stub_end - _wram_stub)
    bne -

    ; Execute from WRAM
    jsl gsu_wram_area

    ; GSU finished — read results
    rep #$20
    .ACCU 16
    lda.l $3000              ; GSU R0
    sta.l gsu_result

    ; Read SRAM bytes written by GSU
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

    plp
    rtl

;--- WRAM stub (copied and executed from WRAM) ---
_wram_stub:
    sep #$20
    .ACCU 8

    ; Disable NMI (vector is in ROM)
    lda #$00
    sta.l $4200

    ; Configure GSU
    lda #$A0
    sta.l $3037              ; CFGR: IRQ mask + fast multiply

    ; Give buses to GSU
    lda #$18
    sta.l $303A              ; SCMR: RAN+RON

    ; Set program bank
    lda #:gsu_program
    sta.l $3034              ; PBR

    ; Start GSU
    rep #$20
    .ACCU 16
    lda #gsu_program
    sta.l $301E              ; R15 → GO!

    ; Poll SFR
    sep #$20
    .ACCU 8
-   lda.l $3030
    and #$20
    bne -

    ; Reclaim buses
    lda #$00
    sta.l $303A

    ; Re-enable NMI
    lda #$81
    sta.l $4200

    rtl
_wram_stub_end:

.ENDS

;------------------------------------------------------------------------------
; setupBitmapTilemap — Identity tilemap (tiles 0-511) at VRAM $4000
;------------------------------------------------------------------------------
.SECTION ".gsu_tilemap" SEMIFREE
.ACCU 16
.INDEX 16
setupBitmapTilemap:
    php
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115
    rep #$20
    .ACCU 16
    lda #$4000
    sta.l $2116
    ; Identity tiles 0-511 (top 128 pixels)
    ldx #$0000
-   stx $2118
    inx
    cpx #$0200
    bne -

    ; Fill remaining 512 entries with tile 0 (black, bottom of screen)
    ldx #$0000
-   stx $2118
    inx
    cpx #$0200
    bne -

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; dmaBitmapToVRAM — DMA 16KB from SRAM $70:0000 to VRAM $0000
;------------------------------------------------------------------------------
.SECTION ".gsu_dma" SEMIFREE
.ACCU 16
.INDEX 16
dmaBitmapToVRAM:
    php
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $2116
    lda #16384
    sta.l $4305
    lda #$0000
    sta.l $4302
    sep #$20
    .ACCU 8
    lda #$70
    sta.l $4304
    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301
    lda #$01
    sta.l $420B
    plp
    rtl
.ENDS

.endif
