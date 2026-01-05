;==============================================================================
; Minimal VRAM Test - Pure Assembly
;==============================================================================
; This tests if the ROM structure and vectors are correct by writing
; a simple pattern to VRAM without any C code.
;==============================================================================

.include "hdr.asm"

.BANK 0
.ORG 0
.EMPTYFILL $00

;------------------------------------------------------------------------------
; NMI Handler - minimal (EmptyHandler is in hdr.asm)
;------------------------------------------------------------------------------
.SECTION ".nmi_handler" SEMIFREE
NmiHandler:
    sep #$20
    lda $4210       ; Acknowledge NMI
    rti
.ENDS

;------------------------------------------------------------------------------
; Start - Main Entry Point
;------------------------------------------------------------------------------
.SECTION ".start" SEMIFREE
Start:
    sei
    clc
    xce             ; Native mode

    ; Use Direct Page addressing (always bank 0) like pvsneslib
    rep #$10        ; 16-bit X/Y
    sep #$20        ; 8-bit A

    ; Set stack
    ldx #$1FFF
    txs

    ; Force blank ON
    lda #$8F
    sta $2100

    ; Mode 0
    stz $2105

    ; BG1 tilemap at $0800
    lda #$08
    sta $2107

    ; BG1 tiles at $0000
    stz $210B

    ; Set VMAIN for word increment after high byte
    lda #$80
    sta $2115

    ; Write tile 0 to VRAM $0000 (8 rows of solid white)
    stz $2116       ; VMADDL = 0
    stz $2117       ; VMADDH = 0

    lda #$FF
    ldx #8
-   sta $2118       ; VMDATAL = $FF (bitplane 0)
    stz $2119       ; VMDATAH = $00 (bitplane 1), address++
    dex
    bne -

    ; Clear tilemap at $0800 with tile 0
    stz $2116       ; VMADDL = 0
    lda #$08
    sta $2117       ; VMADDH = 8 -> word address $0800

    ldx #1024
-   stz $2118       ; Tile 0
    stz $2119       ; No flip, palette 0
    dex
    bne -

    ; Set palette
    stz $2121       ; CGADD = 0

    ; Color 0: Dark blue
    stz $2122       ; Low byte
    lda #$28
    sta $2122       ; High byte

    ; Color 1: White
    lda #$FF
    sta $2122
    lda #$7F
    sta $2122

    ; Enable BG1 on main screen
    lda #$01
    sta $212C

    ; Screen ON (full brightness)
    lda #$0F
    sta $2100

    ; Infinite loop
-   bra -

.ENDS
