;==============================================================================
; HDMAGradient - Graphics Data
; OpenSNES logo, 256-color (8bpp) Mode 3 image.
;==============================================================================

; Tile data (< 32KB, single section)
.section ".rodata_tiles" superfree

opensnes_tiles:
.incbin "res/opensnes.pic"
opensnes_tiles_end:

.ends

; Tilemap and palette
.section ".rodata_map" superfree

opensnes_map:
.incbin "res/opensnes.map"
opensnes_map_end:

opensnes_palette:
.incbin "res/opensnes.pal"
opensnes_palette_end:

.ends

;==============================================================================
; void loadGraphics(void)
;
; Loads tile data, palette, and tilemap to VRAM/CGRAM using DMA.
; Must be called during forced blank (INIDISP=$80).
;
; VRAM layout:
;   $0000: tilemap (32x32)
;   $1000: tile data (8bpp)
;==============================================================================
.section ".loader" superfree

loadGraphics:
    php

    ;--- Tiles to VRAM $1000 ---
    sep #$20
    lda #$80
    sta.l $2115             ; VMAIN: word mode

    rep #$20
    lda #$1000
    sta.l $2116             ; VMADDL/H = $1000

    lda #(opensnes_tiles_end - opensnes_tiles)
    sta.l $4305             ; size

    lda #opensnes_tiles
    sta.l $4302             ; source address

    sep #$20
    lda #:opensnes_tiles    ; bank byte
    sta.l $4304

    lda #$01
    sta.l $4300             ; DMA mode: 2-register word write
    lda #$18
    sta.l $4301             ; B-bus: VMDATAL

    lda #$01
    sta.l $420B             ; start DMA

    ;--- Palette: 256 colors to CGRAM ---
    sep #$20
    lda #$00
    sta.l $2121             ; CGADD = color 0

    rep #$20
    lda #(opensnes_palette_end - opensnes_palette)
    sta.l $4305

    lda #opensnes_palette
    sta.l $4302

    sep #$20
    lda #:opensnes_palette
    sta.l $4304

    lda #$00
    sta.l $4300             ; DMA mode: 1-register byte write
    lda #$22
    sta.l $4301             ; B-bus: CGDATA

    lda #$01
    sta.l $420B

    ;--- Tilemap to VRAM $0000 ---
    lda #$80
    sta.l $2115

    rep #$20
    lda #$0000
    sta.l $2116

    lda #(opensnes_map_end - opensnes_map)
    sta.l $4305

    lda #opensnes_map
    sta.l $4302

    sep #$20
    lda #:opensnes_map
    sta.l $4304

    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301

    lda #$01
    sta.l $420B

    plp
    rtl

.ends
