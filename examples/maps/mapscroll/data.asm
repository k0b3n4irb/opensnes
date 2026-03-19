;==============================================================================
; Map Scroll Example — Asset Data
;==============================================================================

;--- Tileset graphics and palette ---
.section ".rodata1" superfree

tileset:
.incbin "res/tilesMario.pic"
tileset_end:

tilesetpal:
.incbin "res/tilesMario.pal"
tilesetpal_end:

.ends

;--- Map data (from tmx2snes) ---
.section ".rodata2" superfree

mapdata:
.incbin "res/BG1.m16"
mapdata_end:

tilesetatt:
.incbin "res/tiledMario.b16"

tilesetdef:
.incbin "res/tiledMario.t16"

.ends

;--- Sprite graphics ---
.section ".rodata3" superfree

gfxsprite:
.incbin "res/mario.pic"
gfxsprite_end:

palsprite:
.incbin "res/mario.pal"
palsprite_end:

.ends
