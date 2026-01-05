;==============================================================================
; OpenSNES CRT0 Minimal - Startup code for C programs
;==============================================================================

.include "hdr.asm"

;------------------------------------------------------------------------------
; Compiler Imaginary Registers (required by cc65816/QBE backend)
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
.EMPTYFILL $00

;------------------------------------------------------------------------------
; NMI Handler
;------------------------------------------------------------------------------
.SECTION ".nmi_handler" SEMIFREE
NmiHandler:
    sep #$20
    lda $4210       ; Acknowledge NMI
    rti
.ENDS

;------------------------------------------------------------------------------
; InitHardware - Basic PPU initialization
;------------------------------------------------------------------------------
.SECTION ".init" SEMIFREE
InitHardware:
    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A

    ; Screen off (force blank)
    lda #$8F
    sta $2100

    ; Mode 0 (4 BG layers, 2bpp each)
    stz $2105

    ; BG1 tilemap at VRAM $0800
    lda #$08
    sta $2107

    ; BG1 tiles at VRAM $0000
    stz $210B

    ; VRAM increment mode
    lda #$80
    sta $2115

    ; Main screen - BG1 enabled
    lda #$01
    sta $212C

    rts
.ENDS

;------------------------------------------------------------------------------
; Start - Reset Vector Entry Point
;------------------------------------------------------------------------------
.SECTION ".start" SEMIFREE
Start:
    sei                 ; Disable interrupts
    clc
    xce                 ; Switch to native mode

    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A

    ; Set stack
    ldx #$1FFF
    txs

    ; Initialize hardware
    jsr InitHardware

    ; Set up for C code
    rep #$30            ; 16-bit A and X/Y

    ; Set Direct Page to compiler registers
    lda #tcc__r0
    tcd

    ; Clear compiler registers
    stz tcc__r0
    stz tcc__r1

    ; Call main() - graphics setup happens in C
    jsl main

    ; main() returned - halt
_halt:
    bra _halt

.ENDS
