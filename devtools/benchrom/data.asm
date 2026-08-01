;----------------------------------------------------------------------
; benchrom — real map data for the map.asm measurement points (same
; blobs and bank-2 pinning as devtools/libtests, shared with the
; examples/maps/map_scroll assets).
;----------------------------------------------------------------------

.section ".rodata2" semifree bank 2

mapdata:
.incbin "../../examples/maps/map_scroll/res/BG1.m16"
mapdata_end:

tilesetdef:
.incbin "../../examples/maps/map_scroll/res/tiledMario.t16"
tilesetdef_end:

tilesetatt:
.incbin "../../examples/maps/map_scroll/res/tiledMario.b16"
tilesetatt_end:

.ends

.section ".rodata_spr" superfree

; 16x16 sprite sheet from the dynamic_sprite example (4bpp .pic)
spr16_tiles:
.incbin "../../examples/sprites/dynamic_sprite/res/sprite16_grid.pic"
spr16_tiles_end:

.ends
