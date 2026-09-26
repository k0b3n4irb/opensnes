; data.asm - LikeMario asset data
;
; Tile/sprite graphics go in SUPERFREE (any bank, accessed via DMA only).
; Map/collision data are read from C through const pointers (far reads).
;
; Post-A6+A7 (v0.19.0), C pointers carry the bank byte natively; dmaCopyVram
; and dmaCopyCGram read it from the caller's Kl pointer. The legacy
; loadGraphics() ASM loader is gone — main.c calls dma helpers directly.
; The dynamic sprite engine gets its bank byte from OAM_SET_GFX(), which
; reads it from the pointer (the getSpriteTilBank() helper that lived here
; worked around a macro that recorded bank 0; removed 2026-09-20).

;----------------------------------------------------------------------
; Tile and sprite graphics (DMA-only access, any bank is fine)
;----------------------------------------------------------------------
ASSET_SECTION ".rodata1"

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
; Map and collision data. C reads them directly — through CONST pointers
; since 2026-09-23, so every read is a far read and the data can live in
; the asset banks like everything else (18 KB that sat in bank $00 and
; left it 12 bytes from full).
;----------------------------------------------------------------------
ASSET_SECTION "rodata2"

mapmario:       .incbin "res/BG1.m16"

tilesetatt:     .incbin "res/map_1_1.b16"

.ends

