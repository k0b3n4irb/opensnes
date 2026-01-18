;==============================================================================
; Scrolling Demo - Graphics Data
; Uses real image assets for parallax scrolling demonstration
;==============================================================================

.SECTION ".scrolldata" SUPERFREE

;------------------------------------------------------------------------------
; load_graphics - Load tiles, tilemaps, and palettes to VRAM/CGRAM
;------------------------------------------------------------------------------
load_graphics:
    php
    phb
    rep #$20
    sep #$10

    ;--------------------------------------------------
    ; Load BG1 (foreground) tiles to VRAM $4000
    ;--------------------------------------------------
    lda #$4000
    sta.l $2116         ; VMADD

    lda #$1801          ; DMA to VRAM, 2-byte
    sta.l $4300
    lda #shader_tiles
    sta.l $4302
    lda #:shader_tiles
    sep #$20
    sta.l $4304
    rep #$20
    lda #(shader_tiles_end - shader_tiles)
    sta.l $4305

    sep #$20
    lda #$80
    sta.l $2115         ; VMAIN - increment on high byte
    lda #$01
    sta.l $420B         ; Start DMA
    rep #$20

    ;--------------------------------------------------
    ; Load BG2 (background) tiles to VRAM $5000
    ;--------------------------------------------------
    lda #$5000
    sta.l $2116

    lda #bg_tiles
    sta.l $4302
    lda #:bg_tiles
    sep #$20
    sta.l $4304
    rep #$20
    lda #(bg_tiles_end - bg_tiles)
    sta.l $4305

    sep #$20
    lda #$01
    sta.l $420B
    rep #$20

    ;--------------------------------------------------
    ; Load BG1 (foreground) tilemap to VRAM $1800
    ;--------------------------------------------------
    lda #$1800
    sta.l $2116

    lda #shader_map
    sta.l $4302
    lda #:shader_map
    sep #$20
    sta.l $4304
    rep #$20
    lda #(shader_map_end - shader_map)
    sta.l $4305

    sep #$20
    lda #$01
    sta.l $420B
    rep #$20

    ;--------------------------------------------------
    ; Load BG2 (background) tilemap to VRAM $1400
    ;--------------------------------------------------
    lda #$1400
    sta.l $2116

    lda #bg_map
    sta.l $4302
    lda #:bg_map
    sep #$20
    sta.l $4304
    rep #$20
    lda #(bg_map_end - bg_map)
    sta.l $4305

    sep #$20
    lda #$01
    sta.l $420B
    rep #$20

    ;--------------------------------------------------
    ; Load BG1 palette to CGRAM (palette 1, offset 16)
    ;--------------------------------------------------
    sep #$20
    lda #16             ; Palette 1 starts at color 16
    sta.l $2121         ; CGADD

    rep #$20
    lda #$2200          ; DMA to CGRAM
    sta.l $4300
    lda #shader_pal
    sta.l $4302
    lda #:shader_pal
    sep #$20
    sta.l $4304
    rep #$20
    lda #(shader_pal_end - shader_pal)
    sta.l $4305

    sep #$20
    lda #$01
    sta.l $420B
    rep #$20

    ;--------------------------------------------------
    ; Load BG2 palette to CGRAM (palette 0, offset 0)
    ;--------------------------------------------------
    sep #$20
    lda #0              ; Palette 0 starts at color 0
    sta.l $2121

    rep #$20
    lda #$2200
    sta.l $4300
    lda #bg_pal
    sta.l $4302
    lda #:bg_pal
    sep #$20
    sta.l $4304
    rep #$20
    lda #(bg_pal_end - bg_pal)
    sta.l $4305

    sep #$20
    lda #$01
    sta.l $420B

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; Graphics Data - Include binary files from pvsneslib
;------------------------------------------------------------------------------

; Foreground layer (shader) - scrolls at full speed
shader_tiles:
    .INCBIN "res/shader.pic"
shader_tiles_end:

shader_map:
    .INCBIN "res/shader.map"
shader_map_end:

shader_pal:
    .INCBIN "res/shader.pal"
shader_pal_end:

; Background layer (pvsneslib logo) - scrolls at half speed for parallax
bg_tiles:
    .INCBIN "res/pvsneslib.pic"
bg_tiles_end:

bg_map:
    .INCBIN "res/pvsneslib.map"
bg_map_end:

bg_pal:
    .INCBIN "res/pvsneslib.pal"
bg_pal_end:

.ENDS
