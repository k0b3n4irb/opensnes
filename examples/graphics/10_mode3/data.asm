;==============================================================================
; Mode 3 Example - 256-color Background
; Demonstrates 8bpp graphics with split DMA transfer (>32KB tiles)
;==============================================================================

;------------------------------------------------------------------------------
; Graphics data - Split across multiple banks due to size
;------------------------------------------------------------------------------

; First part of tiles (32KB)
.section ".rodata1" superfree
tiles_part1:
.incbin "res/pvsneslib.pic" SKIP 0 READ $8000
tiles_part1_end:
.ends

; Second part of tiles (remaining ~7KB)
.section ".rodata2" superfree
tiles_part2:
.incbin "res/pvsneslib.pic" SKIP $8000
tiles_part2_end:
.ends

; Tilemap and palette
.section ".rodata3" superfree
tilemap: .incbin "res/pvsneslib.map"
tilemap_end:

palette: .incbin "res/pvsneslib.pal"
palette_end:
.ends

;------------------------------------------------------------------------------
; load_mode3_graphics - Load 256-color background
;------------------------------------------------------------------------------
.section ".mode3_code" superfree

load_mode3_graphics:
    php
    phb
    sep #$20
    lda #$00
    pha
    plb

    ; Mode 3 uses 8bpp tiles (256 colors)
    ; Tiles at VRAM $0000, Map at $6000
    ; Total tile data is ~40KB, split into 2 DMA transfers from different banks

    ; First DMA: 32KB to VRAM $0000
    rep #$20
    lda #$0000
    sta $2116
    lda #(tiles_part1_end - tiles_part1)
    sta $4305
    lda #tiles_part1
    sta $4302
    sep #$20
    lda #:tiles_part1
    sta $4304
    lda #$80
    sta $2115
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ; Second DMA: remaining bytes to VRAM $4000
    rep #$20
    lda #$4000              ; Continue at word address $4000
    sta $2116
    lda #(tiles_part2_end - tiles_part2)
    sta $4305
    lda #tiles_part2
    sta $4302
    sep #$20
    lda #:tiles_part2
    sta $4304
    lda #$01
    sta $420B

    ; Load tilemap to VRAM $6000
    rep #$20
    lda #$6000
    sta $2116
    lda #(tilemap_end - tilemap)
    sta $4305
    lda #tilemap
    sta $4302
    sep #$20
    lda #:tilemap
    sta $4304
    lda #$01
    sta $420B

    ; Load 256-color palette to CGRAM
    lda #0
    sta $2121
    rep #$20
    lda #(palette_end - palette)
    sta $4305
    lda #palette
    sta $4302
    sep #$20
    lda #:palette
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
; setup_mode3_regs - Configure PPU for Mode 3
;------------------------------------------------------------------------------
setup_mode3_regs:
    php
    sep #$20

    ; Mode 3 (256 colors BG1, 16 colors BG2)
    lda #$03
    sta $2105

    ; BG1 tilemap at $6000, 32x32
    lda #$60
    sta $2107

    ; BG1 tiles at $0000
    lda #$00
    sta $210B

    ; Enable BG1 only
    lda #$01
    sta $212C

    plp
    rtl

.ends
