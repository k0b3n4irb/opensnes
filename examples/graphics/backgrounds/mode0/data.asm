;==============================================================================
; Mode0 Example - Data and Graphics Loading
;==============================================================================

;------------------------------------------------------------------------------
; Graphics data (tiles, maps, palettes for 4 backgrounds)
;------------------------------------------------------------------------------
.section ".rodata1" superfree

; BG0 data
bg0_tiles: .incbin "res/bg0.pic"
bg0_tiles_end:

bg0_map: .incbin "res/bg0.map"
bg0_map_end:

bg0_pal: .incbin "res/bg0.pal"
bg0_pal_end:

; BG1 data
bg1_tiles: .incbin "res/bg1.pic"
bg1_tiles_end:

bg1_map: .incbin "res/bg1.map"
bg1_map_end:

bg1_pal: .incbin "res/bg1.pal"
bg1_pal_end:

; BG2 data
bg2_tiles: .incbin "res/bg2.pic"
bg2_tiles_end:

bg2_map: .incbin "res/bg2.map"
bg2_map_end:

bg2_pal: .incbin "res/bg2.pal"
bg2_pal_end:

; BG3 data
bg3_tiles: .incbin "res/bg3.pic"
bg3_tiles_end:

bg3_map: .incbin "res/bg3.map"
bg3_map_end:

bg3_pal: .incbin "res/bg3.pal"
bg3_pal_end:

.ends

;------------------------------------------------------------------------------
; RAM for scroll positions
;------------------------------------------------------------------------------
.ramsection ".mode0_vars" bank $7e slot 1
    scroll_bg1_x dsw 1
    scroll_bg2_x dsw 1
    scroll_bg3_x dsw 1
    scroll_flip  dsw 1
.ends

;------------------------------------------------------------------------------
; load_mode0_graphics - Load all 4 backgrounds to VRAM
;------------------------------------------------------------------------------
.section ".mode0_load" superfree

; Macro for DMA transfer to VRAM
.macro DMA_VRAM_TRANSFER ARGS vram_addr, src_label, src_end_label
    rep #$20
    lda #vram_addr
    sta $2116
    lda #(src_end_label - src_label)
    sta $4305
    lda #src_label
    sta $4302
    sep #$20
    lda #:src_label
    sta $4304
    lda #$80
    sta $2115
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B
.endm

; Macro for DMA transfer to CGRAM
.macro DMA_CGRAM_TRANSFER ARGS cgram_offset, src_label, src_end_label
    sep #$20
    lda #cgram_offset
    sta $2121
    rep #$20
    lda #(src_end_label - src_label)
    sta $4305
    lda #src_label
    sta $4302
    sep #$20
    lda #:src_label
    sta $4304
    lda #$00
    sta $4300
    lda #$22
    sta $4301
    lda #$01
    sta $420B
.endm

load_mode0_graphics:
    php
    phb
    sep #$20
    lda #$00
    pha
    plb

    ; BG0: Tiles at $2000, Map at $0000, Palette at 0
    DMA_VRAM_TRANSFER $2000, bg0_tiles, bg0_tiles_end
    DMA_VRAM_TRANSFER $0000, bg0_map, bg0_map_end
    DMA_CGRAM_TRANSFER 0, bg0_pal, bg0_pal_end

    ; BG1: Tiles at $3000, Map at $0400, Palette at 32
    DMA_VRAM_TRANSFER $3000, bg1_tiles, bg1_tiles_end
    DMA_VRAM_TRANSFER $0400, bg1_map, bg1_map_end
    DMA_CGRAM_TRANSFER 32, bg1_pal, bg1_pal_end

    ; BG2: Tiles at $4000, Map at $0800, Palette at 64
    DMA_VRAM_TRANSFER $4000, bg2_tiles, bg2_tiles_end
    DMA_VRAM_TRANSFER $0800, bg2_map, bg2_map_end
    DMA_CGRAM_TRANSFER 64, bg2_pal, bg2_pal_end

    ; BG3: Tiles at $5000, Map at $0C00, Palette at 96
    DMA_VRAM_TRANSFER $5000, bg3_tiles, bg3_tiles_end
    DMA_VRAM_TRANSFER $0C00, bg3_map, bg3_map_end
    DMA_CGRAM_TRANSFER 96, bg3_pal, bg3_pal_end

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; setup_mode0_regs - Configure PPU registers for Mode 0
;------------------------------------------------------------------------------
setup_mode0_regs:
    php
    sep #$20

    ; Set Mode 0
    lda #$00
    sta $2105             ; BGMODE = 0

    ; BG1 tilemap at $0000, 32x32
    lda #$00              ; ($0000 >> 8) | 0 (32x32)
    sta $2107             ; BG1SC

    ; BG2 tilemap at $0400, 32x32
    lda #$04              ; ($0400 >> 8) | 0
    sta $2108             ; BG2SC

    ; BG3 tilemap at $0800, 32x32
    lda #$08              ; ($0800 >> 8) | 0
    sta $2109             ; BG3SC

    ; BG4 tilemap at $0C00, 32x32
    lda #$0C              ; ($0C00 >> 8) | 0
    sta $210A             ; BG4SC

    ; BG1/BG2 tile data: BG1 at $2000, BG2 at $3000
    lda #$32              ; (($2000 >> 12) << 0) | (($3000 >> 12) << 4)
    sta $210B             ; BG12NBA

    ; BG3/BG4 tile data: BG3 at $4000, BG4 at $5000
    lda #$54              ; (($4000 >> 12) << 0) | (($5000 >> 12) << 4)
    sta $210C             ; BG34NBA

    ; Enable all 4 backgrounds on main screen
    lda #$0F              ; BG1 + BG2 + BG3 + BG4
    sta $212C             ; TM

    ; Clear sub screen
    lda #$00
    sta $212D             ; TS

    plp
    rtl

;------------------------------------------------------------------------------
; update_scrolling - Update BG scroll positions (parallax)
;------------------------------------------------------------------------------
update_scrolling:
    php
    rep #$20

    ; Increment flip counter
    lda scroll_flip
    inc a
    sta scroll_flip

    ; Only update every 3 frames
    cmp #3
    bcc @done

    ; Reset flip counter
    lda #0
    sta scroll_flip

    ; BG3 scrolls slowest (+1)
    lda scroll_bg3_x
    inc a
    sta scroll_bg3_x

    ; BG2 scrolls medium (+2)
    lda scroll_bg2_x
    clc
    adc #2
    sta scroll_bg2_x

    ; BG1 scrolls fastest (+3)
    lda scroll_bg1_x
    clc
    adc #3
    sta scroll_bg1_x

    ; Apply scroll values to hardware
    sep #$20

    ; BG2 horizontal scroll (register $210F)
    lda scroll_bg1_x
    sta $210F
    lda scroll_bg1_x+1
    sta $210F

    ; BG2 vertical scroll = 0
    stz $2110
    stz $2110

    ; BG3 horizontal scroll (register $2111)
    lda scroll_bg2_x
    sta $2111
    lda scroll_bg2_x+1
    sta $2111

    ; BG3 vertical scroll = 0
    stz $2112
    stz $2112

    ; BG4 horizontal scroll (register $2113)
    lda scroll_bg3_x
    sta $2113
    lda scroll_bg3_x+1
    sta $2113

    ; BG4 vertical scroll = 0
    stz $2114
    stz $2114

@done:
    plp
    rtl

.ends
