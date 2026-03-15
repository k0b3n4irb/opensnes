;----------------------------------------------------------------------
; Mode 3 background data (256 colors, 8bpp)
;
; Tileset is >32KB — split across two SUPERFREE sections.
; loadGraphics handles all DMA with explicit bank bytes.
;----------------------------------------------------------------------

.section ".rodata1" superfree

; First 32KB of tiles (may end up in bank $01+)
tiles:      .incbin "res/bg.pic" skip 0 read 32768
tiles_end:

.ends

.section ".rodata2" superfree

; Remaining tiles
tiles2:     .incbin "res/bg.pic" skip 32768
tiles2_end:

; Tilemap + palette
tilemap:    .incbin "res/bg.map"
tilemap_end:

palette:    .incbin "res/bg.pal"
palette_end:

.ends

;----------------------------------------------------------------------
; loadGraphics — DMA all assets with correct bank bytes
;----------------------------------------------------------------------
.section ".loader" superfree

loadGraphics:
    php
    sep #$20
    rep #$10

    ;-- 1. First 32KB tiles → VRAM $0000 --
    lda #$80
    sta.l $2115                 ; VMAIN: word increment
    rep #$20
    lda #$0000
    sta.l $2116                 ; VRAM dest
    lda #(tiles_end - tiles)
    sta.l $4305                 ; byte count
    lda #tiles
    sta.l $4302                 ; source addr
    sep #$20
    lda #:tiles                 ; BANK BYTE from linker
    sta.l $4304
    lda #$01
    sta.l $4300                 ; DMA mode: word write
    lda #$18
    sta.l $4301                 ; dest: VMDATAL
    lda #$01
    sta.l $420B                 ; start DMA ch0

    ;-- 2. Remaining tiles → VRAM $4000 --
    rep #$20
    lda #$4000
    sta.l $2116                 ; VRAM dest
    lda #(tiles2_end - tiles2)
    sta.l $4305
    lda #tiles2
    sta.l $4302
    sep #$20
    lda #:tiles2
    sta.l $4304
    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301
    lda #$01
    sta.l $420B

    ;-- 3. Tilemap → VRAM $6000 --
    rep #$20
    lda #$6000
    sta.l $2116
    lda #(tilemap_end - tilemap)
    sta.l $4305
    lda #tilemap
    sta.l $4302
    sep #$20
    lda #:tilemap
    sta.l $4304
    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301
    lda #$01
    sta.l $420B

    ;-- 4. Palette → CGRAM color 0 (256 colors = 512 bytes) --
    sep #$20
    lda #$00
    sta.l $2121                 ; CGADD = 0
    rep #$20
    lda #(palette_end - palette)
    sta.l $4305
    lda #palette
    sta.l $4302
    sep #$20
    lda #:palette
    sta.l $4304
    lda #$00
    sta.l $4300                 ; DMA mode: byte write
    lda #$22                    ; dest: CGDATA
    sta.l $4301
    lda #$01
    sta.l $420B

    plp
    rtl

.ends
