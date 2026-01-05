;==============================================================================
; Hello World - Pure Assembly (no C code)
;==============================================================================
; Based on minimal_test.asm which works correctly
;==============================================================================

.include "hdr.asm"

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
; Start - Main Entry Point
;------------------------------------------------------------------------------
.SECTION ".start" SEMIFREE
Start:
    sei
    clc
    xce             ; Native mode

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

    ;--------------------------------------------------
    ; Write font tiles to VRAM $0000
    ;--------------------------------------------------
    stz $2116       ; VMADDL = 0
    stz $2117       ; VMADDH = 0

    ; Tile 0: D
    lda #$7C
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$7C
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 1: E
    lda #$7E
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$7C
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$7E
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 2: H
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$7E
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 3: L
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$7E
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 4: O
    lda #$3C
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$3C
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 5: R
    lda #$7C
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$7C
    sta $2118
    stz $2119
    lda #$48
    sta $2118
    stz $2119
    lda #$44
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 6: W
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$5A
    sta $2118
    stz $2119
    lda #$66
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 7: !
    lda #$18
    sta $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 8: Space (empty)
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119

    ;--------------------------------------------------
    ; Clear tilemap at $0800 with tile 8 (space)
    ;--------------------------------------------------
    stz $2116       ; VMADDL = 0
    lda #$08
    sta $2117       ; VMADDH = 8 -> word address $0800

    ldx #1024
-   lda #8          ; Tile 8 = space
    sta $2118
    stz $2119       ; Attributes = 0
    dex
    bne -

    ;--------------------------------------------------
    ; Write "HELLO WORLD!" at row 14, column 10
    ; Tilemap address = $0800 + (14*32) + 10 = $0800 + $1CA = $09CA
    ;--------------------------------------------------
    lda #$CA
    sta $2116       ; VMADDL = $CA
    lda #$09
    sta $2117       ; VMADDH = $09

    ; H=2, E=1, L=3, L=3, O=4, space=8, W=6, O=4, R=5, L=3, D=0, !=7
    lda #2
    sta $2118
    stz $2119       ; H
    lda #1
    sta $2118
    stz $2119       ; E
    lda #3
    sta $2118
    stz $2119       ; L
    lda #3
    sta $2118
    stz $2119       ; L
    lda #4
    sta $2118
    stz $2119       ; O
    lda #8
    sta $2118
    stz $2119       ; (space)
    lda #6
    sta $2118
    stz $2119       ; W
    lda #4
    sta $2118
    stz $2119       ; O
    lda #5
    sta $2118
    stz $2119       ; R
    lda #3
    sta $2118
    stz $2119       ; L
    lda #0
    sta $2118
    stz $2119       ; D
    lda #7
    sta $2118
    stz $2119       ; !

    ;--------------------------------------------------
    ; Set palette
    ;--------------------------------------------------
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

    ;--------------------------------------------------
    ; Enable BG1 on main screen
    ;--------------------------------------------------
    lda #$01
    sta $212C

    ;--------------------------------------------------
    ; Screen ON (full brightness)
    ;--------------------------------------------------
    lda #$0F
    sta $2100

    ; Infinite loop
-   bra -

.ENDS
