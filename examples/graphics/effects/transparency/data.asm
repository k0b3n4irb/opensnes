;==============================================================================
; Transparency — Graphics Data
; Landscape (BG1, 4bpp) + Clouds (BG3, 2bpp)
;==============================================================================

;--- Landscape: 4bpp tiles + tilemap + palette (palette bank 1) ---
.section ".rodata1" superfree

land_tiles:
.incbin "res/backgrounds.pic"
land_tiles_end:

land_map:
.incbin "res/backgrounds.map"
land_map_end:

land_pal:
.incbin "res/backgrounds.pal"
land_pal_end:

.ends

;--- Clouds: 2bpp tiles + tilemap + palette (palette bank 0) ---
.section ".rodata2" superfree

cloud_tiles:
.incbin "res/clouds.pic"
cloud_tiles_end:

cloud_map:
.incbin "res/clouds.map"
cloud_map_end:

cloud_pal:
.incbin "res/clouds.pal"
cloud_pal_end:

.ends

;==============================================================================
; void loadGraphics(void)
;
; DMA all data to VRAM/CGRAM with correct bank bytes.
; Must be called during forced blank (INIDISP=$80).
;
; VRAM layout:
;   $0000: BG1 tiles (4bpp landscape)
;   $1000: BG3 tiles (2bpp clouds)
;   $2000: BG1 tilemap (SC_32x32)
;   $2400: BG3 tilemap (SC_32x32)
;
; CGRAM layout:
;   Colors 0-3:   Cloud palette (BG3, 2bpp palette bank 0)
;   Colors 16-31: Land palette (BG1, 4bpp palette bank 1 via -e 1)
;==============================================================================
.section ".loader" superfree

loadGraphics:
    php

    ;--- BG1 tiles to VRAM $0000 ---
    sep #$20
    lda #$80
    sta.l $2115             ; VMAIN: word increment

    rep #$20
    lda #$0000
    sta.l $2116             ; VRAM addr = $0000

    lda #(land_tiles_end - land_tiles)
    sta.l $4305             ; size

    lda #land_tiles
    sta.l $4302             ; source addr

    sep #$20
    lda #:land_tiles
    sta.l $4304             ; source bank

    lda #$01
    sta.l $4300             ; DMA mode: word write
    lda #$18
    sta.l $4301             ; dest: VMDATAL

    lda #$01
    sta.l $420B             ; start DMA

    ;--- BG3 tiles to VRAM $1000 ---
    rep #$20
    lda #$1000
    sta.l $2116

    lda #(cloud_tiles_end - cloud_tiles)
    sta.l $4305

    lda #cloud_tiles
    sta.l $4302

    sep #$20
    lda #:cloud_tiles
    sta.l $4304

    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301

    lda #$01
    sta.l $420B

    ;--- BG1 tilemap to VRAM $2000 ---
    rep #$20
    lda #$2000
    sta.l $2116

    lda #(land_map_end - land_map)
    sta.l $4305

    lda #land_map
    sta.l $4302

    sep #$20
    lda #:land_map
    sta.l $4304

    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301

    lda #$01
    sta.l $420B

    ;--- BG3 tilemap to VRAM $2400 ---
    rep #$20
    lda #$2400
    sta.l $2116

    lda #(cloud_map_end - cloud_map)
    sta.l $4305

    lda #cloud_map
    sta.l $4302

    sep #$20
    lda #:cloud_map
    sta.l $4304

    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301

    lda #$01
    sta.l $420B

    ;--- Cloud palette to CGRAM color 0 ---
    sep #$20
    lda #$00
    sta.l $2121             ; CGADD = color 0

    rep #$20
    lda #(cloud_pal_end - cloud_pal)
    sta.l $4305

    lda #cloud_pal
    sta.l $4302

    sep #$20
    lda #:cloud_pal
    sta.l $4304

    lda #$00
    sta.l $4300             ; DMA mode: byte write
    lda #$22
    sta.l $4301             ; dest: CGDATA

    lda #$01
    sta.l $420B

    ;--- Land palette to CGRAM color 16 (palette bank 1) ---
    sep #$20
    lda #16
    sta.l $2121             ; CGADD = color 16

    rep #$20
    lda #(land_pal_end - land_pal)
    sta.l $4305

    lda #land_pal
    sta.l $4302

    sep #$20
    lda #:land_pal
    sta.l $4304

    lda #$00
    sta.l $4300
    lda #$22
    sta.l $4301

    lda #$01
    sta.l $420B

    plp
    rtl

.ends
