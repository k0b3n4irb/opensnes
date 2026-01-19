;==============================================================================
; Continuous Scroll Example - Graphics data and setup
; Port of PVSnesLib Mode1ContinuousScroll
;==============================================================================

;------------------------------------------------------------------------------
; Graphics data
;------------------------------------------------------------------------------
.section ".rodata1" superfree

; BG1 - Main scrolling background
bg1_tiles: .incbin "res/BG1.pic"
bg1_tiles_end:

bg1_pal: .incbin "res/BG1.pal"
bg1_pal_end:

bg1_map: .incbin "res/BG1.map"
bg1_map_end:

.ends

.section ".rodata2" superfree

; BG2 - Sub scrolling background (parallax)
bg2_tiles: .incbin "res/BG2.pic"
bg2_tiles_end:

bg2_pal: .incbin "res/BG2.pal"
bg2_pal_end:

bg2_map: .incbin "res/BG2.map"
bg2_map_end:

.ends

.section ".rodata3" superfree

; BG3 - Static HUD/overlay
bg3_tiles: .incbin "res/BG3.pic"
bg3_tiles_end:

bg3_pal: .incbin "res/BG3.pal"
bg3_pal_end:

bg3_map: .incbin "res/BG3.map"
bg3_map_end:

; Character sprite
char_tiles: .incbin "res/character.pic"
char_tiles_end:

char_pal: .incbin "res/character.pal"
char_pal_end:

.ends

;------------------------------------------------------------------------------
; Initial graphics loading
;------------------------------------------------------------------------------
.section ".scroll_code" superfree

load_scroll_graphics:
    php
    phb
    sep #$20
    lda #$00
    pha
    plb

    ;--- BG1 tiles at $2000 ---
    rep #$20
    lda #$2000
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

    ;--- BG1 palette at offset 32 (palette 2) ---
    lda #32
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

    ;--- BG1 initial map (first 2KB) to VRAM $0000 ---
    rep #$20
    lda #$0000
    sta $2116
    lda #2048
    sta $4305
    lda #bg1_map
    sta $4302
    sep #$20
    lda #:bg1_map
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ;--- BG2 tiles at $3000 ---
    rep #$20
    lda #$3000
    sta $2116
    lda #(bg2_tiles_end - bg2_tiles)
    sta $4305
    lda #bg2_tiles
    sta $4302
    sep #$20
    lda #:bg2_tiles
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ;--- BG2 palette at offset 64 (palette 4) ---
    lda #64
    sta $2121
    rep #$20
    lda #(bg2_pal_end - bg2_pal)
    sta $4305
    lda #bg2_pal
    sta $4302
    sep #$20
    lda #:bg2_pal
    sta $4304
    lda #$00
    sta $4300
    lda #$22
    sta $4301
    lda #$01
    sta $420B

    ;--- BG2 initial map to VRAM $0800 ---
    rep #$20
    lda #$0800
    sta $2116
    lda #2048
    sta $4305
    lda #bg2_map
    sta $4302
    sep #$20
    lda #:bg2_map
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ;--- BG3 tiles at $4000 ---
    rep #$20
    lda #$4000
    sta $2116
    lda #(bg3_tiles_end - bg3_tiles)
    sta $4305
    lda #bg3_tiles
    sta $4302
    sep #$20
    lda #:bg3_tiles
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ;--- BG3 palette at offset 0 ---
    lda #0
    sta $2121
    rep #$20
    lda #(bg3_pal_end - bg3_pal)
    sta $4305
    lda #bg3_pal
    sta $4302
    sep #$20
    lda #:bg3_pal
    sta $4304
    lda #$00
    sta $4300
    lda #$22
    sta $4301
    lda #$01
    sta $420B

    ;--- BG3 map to VRAM $1000 ---
    rep #$20
    lda #$1000
    sta $2116
    lda #(bg3_map_end - bg3_map)
    sta $4305
    lda #bg3_map
    sta $4302
    sep #$20
    lda #:bg3_map
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ;--- Character sprite tiles at $6000 ---
    rep #$20
    lda #$6000
    sta $2116
    lda #(char_tiles_end - char_tiles)
    sta $4305
    lda #char_tiles
    sta $4302
    sep #$20
    lda #:char_tiles
    sta $4304
    lda #$01
    sta $4300
    lda #$18
    sta $4301
    lda #$01
    sta $420B

    ;--- Character palette at offset 128 (sprite palette 0) ---
    lda #128
    sta $2121
    rep #$20
    lda #(char_pal_end - char_pal)
    sta $4305
    lda #char_pal
    sta $4302
    sep #$20
    lda #:char_pal
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
; Setup PPU registers for Mode 1
;------------------------------------------------------------------------------
setup_scroll_regs:
    php
    sep #$20

    ; Mode 1 with BG3 priority
    lda #$09                ; Mode 1 + BG3 priority bit
    sta $2105

    ; BG1 tilemap at $0000, 32x32 (1 page)
    lda #$00                ; $0000 + 32x32 size
    sta $2107

    ; BG2 tilemap at $0800, 32x32 (1 page)
    lda #$08                ; $0800 + 32x32 size
    sta $2108

    ; BG3 tilemap at $1000, 32x32
    lda #$10
    sta $2109

    ; BG1 tiles at $2000, BG2 tiles at $3000
    lda #$32
    sta $210B

    ; BG3 tiles at $4000
    lda #$04
    sta $210C

    ; Enable BG1, BG2, BG3, sprites on main screen
    lda #$17
    sta $212C

    plp
    rtl

.ends
