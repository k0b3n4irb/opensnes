;==============================================================================
; Parallax Scrolling Example - Data and Graphics Loading
; Two backgrounds scrolling at different speeds (Mode 1)
;==============================================================================

;------------------------------------------------------------------------------
; Graphics data
;------------------------------------------------------------------------------
.section ".rodata1" superfree

; Background 1 - Main layer (static or slow scroll)
bg1_tiles: .incbin "res/pvsneslib.pic"
bg1_tiles_end:

bg1_map: .incbin "res/pvsneslib.map"
bg1_map_end:

bg1_pal: .incbin "res/pvsneslib.pal"
bg1_pal_end:

; Background 0 - Scrolling layer (faster scroll)
bg0_tiles: .incbin "res/shader.pic"
bg0_tiles_end:

bg0_map: .incbin "res/shader.map"
bg0_map_end:

bg0_pal: .incbin "res/shader.pal"
bg0_pal_end:

.ends

;------------------------------------------------------------------------------
; RAM for scroll position
;------------------------------------------------------------------------------
.ramsection ".parallax_vars" bank $7e slot 1
    scroll_x dsw 1
    scroll_y dsw 1
.ends

;------------------------------------------------------------------------------
; load_parallax_graphics - Load both backgrounds
;------------------------------------------------------------------------------
.section ".parallax_code" superfree

load_parallax_graphics:
    php
    phb
    sep #$20
    lda #$00
    pha
    plb

    ; BG1: Main background (tiles at $5000, map at $1400)
    rep #$20
    lda #$5000
    sta $2116
    lda #(bg1_tiles_end - bg1_tiles)
    sta $4305
    lda #bg1_tiles
    sta $4302
    sep #$20
    lda #:bg1_tiles
    sta $4304
    lda #$80
    sta $2115
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ; BG1 map at $1400
    rep #$20
    lda #$1400
    sta $2116
    lda #(bg1_map_end - bg1_map)
    sta $4305
    lda #bg1_map
    sta $4302
    sep #$20
    lda #:bg1_map
    sta $4304
    lda #$01
    sta $420B

    ; BG1 palette at offset 0
    lda #0
    sta $2121
    rep #$20
    lda #(bg1_pal_end - bg1_pal)
    sta $4305
    lda #bg1_pal
    sta $4302
    sep #$20
    lda #:bg1_pal
    sta $4304
    lda #$00
    sta $4300
    lda #$22
    sta $4301
    lda #$01
    sta $420B

    ; BG0: Scrolling layer (tiles at $4000, map at $1800)
    rep #$20
    lda #$4000
    sta $2116
    lda #(bg0_tiles_end - bg0_tiles)
    sta $4305
    lda #bg0_tiles
    sta $4302
    sep #$20
    lda #:bg0_tiles
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ; BG0 map at $1800
    rep #$20
    lda #$1800
    sta $2116
    lda #(bg0_map_end - bg0_map)
    sta $4305
    lda #bg0_map
    sta $4302
    sep #$20
    lda #:bg0_map
    sta $4304
    lda #$01
    sta $420B

    ; BG0 palette at offset 16 (palette 1)
    lda #16
    sta $2121
    rep #$20
    lda #(bg0_pal_end - bg0_pal)
    sta $4305
    lda #bg0_pal
    sta $4302
    sep #$20
    lda #:bg0_pal
    sta $4304
    lda #$00
    sta $4300
    lda #$22
    sta $4301
    lda #$01
    sta $420B

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; setup_parallax_regs - Configure PPU for Mode 1 with 2 BGs
;------------------------------------------------------------------------------
setup_parallax_regs:
    php
    sep #$20

    ; Mode 1
    lda #$01
    sta $2105

    ; BG1 tilemap at $1400
    lda #$14
    sta $2107

    ; BG2 tilemap at $1800
    lda #$18
    sta $2108

    ; BG1 tiles at $5000, BG2 tiles at $4000
    lda #$45              ; BG1=$5, BG2=$4
    sta $210B

    ; Enable BG1 and BG2
    lda #$03
    sta $212C

    plp
    rtl

;------------------------------------------------------------------------------
; update_parallax - Auto-scroll the foreground layer
;------------------------------------------------------------------------------
update_parallax:
    php
    rep #$20

    ; Increment scroll position
    lda scroll_x
    inc a
    sta scroll_x

    lda scroll_y
    inc a
    sta scroll_y

    ; Apply to BG1 (scrolling layer - register $210D/$210E)
    sep #$20
    lda scroll_x
    sta $210D
    lda scroll_x+1
    sta $210D

    lda scroll_y
    sta $210E
    lda scroll_y+1
    sta $210E

    plp
    rtl

.ends
