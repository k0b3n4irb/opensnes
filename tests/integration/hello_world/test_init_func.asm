;==============================================================================
; Test: Does a separate InitHardware function break the display?
;==============================================================================

.include "hdr.asm"

;------------------------------------------------------------------------------
; RAMSECTION - This works fine (already tested)
;------------------------------------------------------------------------------
.RAMSECTION ".registers" BANK 0 SLOT 1 ORGA 0 FORCE
    tcc__r0     dsb 2
    tcc__r0h    dsb 2
    tcc__r1     dsb 2
    tcc__r1h    dsb 2
.ENDS

.BANK 0
.ORG 0
.EMPTYFILL $00

;------------------------------------------------------------------------------
; NMI Handler (must be first so it's at a known address)
;------------------------------------------------------------------------------
.SECTION ".nmi_handler" SEMIFREE
NmiHandler:
    sep #$20
    lda $4210       ; Acknowledge NMI
    rti
.ENDS

;------------------------------------------------------------------------------
; TEST: Add InitHardware as separate function (like original crt0.asm)
;------------------------------------------------------------------------------
.SECTION ".init" SEMIFREE
InitHardware:
    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A

    ; Screen off (force blank)
    lda #$8F
    sta $2100

    ; Clear sprite registers
    stz $2101
    stz $2102
    stz $2103

    ; Clear BG mode and tilemap registers
    stz $2105
    stz $2106
    stz $2107
    stz $2108
    stz $2109
    stz $210A
    stz $210B
    stz $210C

    ; VRAM increment mode
    lda #$80
    sta $2115

    ; Clear VRAM address
    stz $2116
    stz $2117

    ; Main screen - BG1 enabled
    lda #$01
    sta $212C

    rts
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

    ; Call InitHardware (THIS IS THE TEST)
    jsr InitHardware

    ; BG1 tilemap at $0800 (InitHardware cleared this to 0!)
    lda #$08
    sta $2107

    ; BG1 tiles at $0000
    stz $210B

    ; VMAIN for word increment after high byte
    lda #$80
    sta $2115

    ; Write font tiles to VRAM $0000
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

    ; Clear tilemap at $0800 with tile 8 (space)
    stz $2116       ; VMADDL = 0
    lda #$08
    sta $2117       ; VMADDH = 8 -> word address $0800

    ldx #1024
_clear_tilemap:
    lda #8          ; Tile 8 = space
    sta $2118
    stz $2119       ; Attributes = 0
    dex
    bne _clear_tilemap

    ; Write "HELLO WORLD!" at row 14, column 10
    lda #$CA
    sta $2116       ; VMADDL = $CA
    lda #$09
    sta $2117       ; VMADDH = $09

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

    ; Set palette
    stz $2121       ; CGADD = 0
    stz $2122       ; Low byte
    lda #$28
    sta $2122       ; High byte (dark blue)
    lda #$FF
    sta $2122
    lda #$7F
    sta $2122       ; White

    ; Enable BG1 on main screen
    lda #$01
    sta $212C

    ; Screen ON (full brightness)
    lda #$0F
    sta $2100

    ; Infinite loop
_halt:
    bra _halt

.ENDS
