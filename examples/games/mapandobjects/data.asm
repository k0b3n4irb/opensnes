ASSET_SECTION ".rodata1"

tileset:
.incbin "res/tilesMario.pic"
tilesetend:

tilesetpal:
.incbin "res/tilesMario.pal"

mapmario:
.incbin "res/BG1.m16"

objmario:
.incbin "res/tiledMario.o16"

tilesetatt:
.incbin "res/tiledMario.b16"

tilesetdef:
.incbin "res/tiledMario.t16"

.ends

ASSET_SECTION ".rodata2"

sprmario:
.incbin "res/mario.pic"

sprgoomba:
.incbin "res/goomba.pic"

sprkoopatroopa:
.incbin "res/koopatroopa.pic"

palsprite:
.incbin "res/mario.pal"

.ends

