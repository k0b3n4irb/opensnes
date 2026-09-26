;==============================================================================
; Slope Mario - Graphics and Map Data
; Ported from PVSnesLib slopemario by Nub1604
;
; Single object type (Mario) with slope collision.
;==============================================================================

ASSET_SECTION ".rodata1"

tileset:
.incbin "res/tiles.pic"
tilesetend:

tilepal:
.incbin "res/tiles.pal"

mariogfx:
.incbin "res/mario_sprite.pic"
mariogfx_end:

mariopal:
.incbin "res/mario_sprite.pal"

.ends

ASSET_SECTION ".rodata2"

mapmario:
.incbin "res/BG1.m16"

objmario:
.incbin "res/map_1_1.o16"

tilesetatt:
.incbin "res/map_1_1.b16"

tilesetdef:
.incbin "res/map_1_1.t16"

.ends

