;==============================================================================
; Minimal OpenSNES Test ROM - Startup Code
;==============================================================================

.include "hdr.asm"

;------------------------------------------------------------------------------
; TCC Imaginary Registers (required by compiler)
;------------------------------------------------------------------------------
.RAMSECTION ".registers" BANK 0 SLOT 1 ORGA 0 FORCE
tcc__r0     dsb 2
tcc__r0h    dsb 2
tcc__r1     dsb 2
tcc__r1h    dsb 2
tcc__r2     dsb 2
tcc__r2h    dsb 2
tcc__r3     dsb 2
tcc__r3h    dsb 2
tcc__r4     dsb 2
tcc__r4h    dsb 2
tcc__r5     dsb 2
tcc__r5h    dsb 2
tcc__r9     dsb 2
tcc__r9h    dsb 2
tcc__r10    dsb 2
tcc__r10h   dsb 2
.ENDS

.BANK 0
.ORG 0

.SECTION "Start" SEMIFREE

.ACCU 16
.INDEX 16
.16BIT

Start:
    ; Disable interrupts
    sei

    ; Switch to native mode
    clc
    xce

    ; 16-bit A/X/Y
    rep #$30

    ; Set stack
    ldx #$1FFF
    txs

    ; Set direct page to tcc registers
    lda #tcc__r0
    tad

    ; Initialize PPU - force blank
    sep #$20
    lda #$8F
    sta $2100

    ; Clear some PPU regs
    stz $2101
    stz $2102
    stz $2103
    stz $2105
    stz $2106
    stz $212C
    stz $212D

    ; Enable main screen BG1
    lda #$01
    sta $212C

    rep #$20

    ; Set data bank to $00
    pea $0000
    plb
    plb

    ; Call main
    jsr main

    ; Halt
    stp

.ENDS
