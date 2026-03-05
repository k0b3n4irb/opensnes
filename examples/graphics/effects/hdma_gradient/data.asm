;==============================================================================
; HDMAGradient - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; 256-color (8bpp) Mode 3 image. Tile data is ~39KB (>32KB),
; split into 2 SUPERFREE sections for the DMA transfer.
;
; IMPORTANT: The first tile chunk (32KB) may be placed in bank $01+ by the
; linker. Since dmaCopyVram() hardcodes bank $00, we provide a dedicated
; loadGraphics assembly function that uses :label for the correct bank byte.
;==============================================================================

; First 32KB of tile data
.section ".rodata_tiles1" superfree

pvsneslib_tiles:
.incbin "res/pvsneslib.pic" READ 32768

.ends

; Remaining tile data (39744 - 32768 = 6976 bytes)
.section ".rodata_tiles2" superfree

pvsneslib_tiles2:
.incbin "res/pvsneslib.pic" SKIP 32768
pvsneslib_tiles2_end:

.ends

; Tilemap and palette
.section ".rodata_map" superfree

pvsneslib_map:
.incbin "res/pvsneslib.map"
pvsneslib_map_end:

pvsneslib_palette:
.incbin "res/pvsneslib.pal"
pvsneslib_palette_end:

.ends

;==============================================================================
; void loadGraphics(void)
;
; Loads all tile data, palette, and tilemap to VRAM/CGRAM using DMA with
; correct bank bytes. Must be called during forced blank (INIDISP=$80).
;
; VRAM layout:
;   $0000: tilemap (32x32)
;   $1000: tile data (8bpp, split across 2 DMA transfers)
;==============================================================================
.section ".loader" superfree

loadGraphics:
    php

    ;--- Tiles part 1: 32KB to VRAM $1000 ---
    sep #$20
    lda #$80
    sta.l $2115             ; VMAIN: word mode, increment on high byte

    rep #$20
    lda #$1000
    sta.l $2116             ; VMADDL/H = $1000

    lda #$8000
    sta.l $4305             ; size = 32768 bytes

    lda #pvsneslib_tiles
    sta.l $4302             ; source address (16-bit)

    sep #$20
    lda #:pvsneslib_tiles   ; bank byte from linker
    sta.l $4304             ; source bank

    lda #$01
    sta.l $4300             ; DMA mode: 2-register word write
    lda #$18
    sta.l $4301             ; B-bus: VMDATAL ($2118)

    lda #$01
    sta.l $420B             ; start DMA channel 0

    ;--- Tiles part 2: remaining bytes to VRAM $5000 ---
    rep #$20
    lda #$5000
    sta.l $2116             ; VMADDL/H = $5000

    lda #(pvsneslib_tiles2_end - pvsneslib_tiles2)
    sta.l $4305             ; size = remaining tile bytes

    lda #pvsneslib_tiles2
    sta.l $4302             ; source address

    sep #$20
    lda #:pvsneslib_tiles2  ; bank byte
    sta.l $4304

    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301

    lda #$01
    sta.l $420B

    ;--- Palette: 256 colors to CGRAM ---
    sep #$20
    lda #$00
    sta.l $2121             ; CGADD = color 0

    rep #$20
    lda #(pvsneslib_palette_end - pvsneslib_palette)
    sta.l $4305             ; size

    lda #pvsneslib_palette
    sta.l $4302             ; source address

    sep #$20
    lda #:pvsneslib_palette ; bank byte
    sta.l $4304

    lda #$00
    sta.l $4300             ; DMA mode: 1-register byte write
    lda #$22
    sta.l $4301             ; B-bus: CGDATA ($2122)

    lda #$01
    sta.l $420B

    ;--- Tilemap to VRAM $0000 ---
    lda #$80
    sta.l $2115             ; VMAIN: word mode

    rep #$20
    lda #$0000
    sta.l $2116             ; VMADDL/H = $0000

    lda #(pvsneslib_map_end - pvsneslib_map)
    sta.l $4305             ; size

    lda #pvsneslib_map
    sta.l $4302             ; source address

    sep #$20
    lda #:pvsneslib_map     ; bank byte
    sta.l $4304

    lda #$01
    sta.l $4300             ; DMA mode: word write
    lda #$18
    sta.l $4301             ; B-bus: VMDATAL

    lda #$01
    sta.l $420B

    plp
    rtl

.ends
