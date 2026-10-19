; data.asm - likemario asset data: the level map and its tile properties
;
; Tile/sprite graphics go in SUPERFREE (any bank, accessed via DMA only).
; Map/collision data are read from C through const pointers (far reads).
;
; A C pointer carries its bank byte, and dmaCopyVram / dmaCopyCGram read
; it: main.c calls the dma helpers directly. The dynamic sprite engine gets
; its bank byte from OAM_SET_GFX(), which reads it from the pointer.

;----------------------------------------------------------------------
; Tile and sprite graphics (DMA-only access, any bank is fine)
;----------------------------------------------------------------------
;----------------------------------------------------------------------
; Map and collision data. C reads them directly — through CONST pointers
; since 2026-09-23, so every read is a far read and the data can live in
; the asset banks like everything else (18 KB that sat in bank $00 and
; left it 12 bytes from full).
;----------------------------------------------------------------------
ASSET_SECTION "rodata2"

maplevel:       .incbin "res/BG1.m16"

tilesetatt:     .incbin "res/map_1_1.b16"

.ends

