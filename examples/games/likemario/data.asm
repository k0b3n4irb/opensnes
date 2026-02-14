; data.asm - LikeMario asset data
;
; Tile/sprite graphics go in SUPERFREE (any bank, accessed via DMA only).
; Map/collision data go in bank 0 (accessed directly from C code).

;----------------------------------------------------------------------
; Tile and sprite graphics (DMA-only access, any bank is fine)
;----------------------------------------------------------------------
.section ".rodata1" superfree

tiles_til:      .incbin "tiles.pic"
tiles_tilend:

tiles_pal:      .incbin "tiles.pal"
tiles_palend:

mario_sprite_til: .incbin "mario_sprite.pic"
mario_sprite_tilend:

mario_sprite_pal: .incbin "mario_sprite.pal"
mario_sprite_palend:

jumpsnd:        .incbin "mariojump.brr"
jumpsndend:

.ends

;----------------------------------------------------------------------
; Map and collision data (C code reads directly, MUST be in bank 0)
;----------------------------------------------------------------------
.section ".rodata2" semifree bank 0

mapmario:       .incbin "BG1.m16"

tilesetatt:     .incbin "map_1_1.b16"

.ends
