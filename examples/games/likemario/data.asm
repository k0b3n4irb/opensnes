; data.asm - LikeMario asset data
;
; Tile/sprite graphics go in SUPERFREE (any bank, accessed via DMA only).
; Map/collision data go in bank 0 (accessed directly from C code).
;
; Post-A6+A7 (v0.19.0), C pointers carry the bank byte natively; dmaCopyVram
; and dmaCopyCGram read it from the caller's Kl pointer. The legacy
; loadGraphics() ASM loader is gone — main.c calls dma helpers directly.
; getSpriteTilBank() below is still needed for the dynamic sprite engine,
; which sets up its own DMA from the bank byte rather than going through
; a Kl-pointer helper.

;----------------------------------------------------------------------
; Tile and sprite graphics (DMA-only access, any bank is fine)
;----------------------------------------------------------------------
.section ".rodata1" superfree

tiles_til:        .incbin "res/tiles.pic"
tiles_tilend:

tiles_pal:        .incbin "res/tiles.pal"
tiles_palend:

mario_sprite_til: .incbin "res/mario_sprite.pic"
mario_sprite_tilend:

mario_sprite_pal: .incbin "res/mario_sprite.pal"
mario_sprite_palend:

.ends

;----------------------------------------------------------------------
; Map and collision data (C code reads directly, MUST be in bank 0)
;----------------------------------------------------------------------
.section ".rodata2" semifree bank 0

mapmario:       .incbin "res/BG1.m16"

tilesetatt:     .incbin "res/map_1_1.b16"

.ends


;----------------------------------------------------------------------
; u8 getSpriteTilBank(void)
;
; Returns the bank byte of mario_sprite_til so C code can use
; OAM_SET_GFX_BANK() for correct dynamic sprite DMA.
;----------------------------------------------------------------------
.section ".bank_helpers" superfree

getSpriteTilBank:
    php
    sep #$20
    lda #:mario_sprite_til      ; bank byte from linker
    rep #$20
    and #$00FF                  ; zero-extend to 16-bit return value
    plp
    rtl

.ends
