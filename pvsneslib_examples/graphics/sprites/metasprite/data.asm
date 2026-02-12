; MetaSprite - Asset data
; Background tiles/map/palette + sprite tiles/palettes for 32x32, 16x16, 8x8

; Background tiles
.section ".rodata1" superfree
bg_tiles:
.incbin "res/pvsneslib.pic"
bg_tiles_end:
.ends

.section ".rodata2" superfree
bg_map:
.incbin "res/pvsneslib.map"
bg_map_end:

bg_pal:
.incbin "res/pvsneslib.pal"
bg_pal_end:
.ends

; 32x32 sprite tiles and palette
.section ".rodata3" superfree
spr32_tiles:
.incbin "res/spritehero32.pic"
spr32_tiles_end:

spr32_pal:
.incbin "res/spritehero32.pal"
spr32_pal_end:
.ends

; 16x16 sprite tiles and palette
.section ".rodata4" superfree
spr16_tiles:
.incbin "res/spritehero16.pic"
spr16_tiles_end:

spr16_pal:
.incbin "res/spritehero16.pal"
spr16_pal_end:
.ends

; 8x8 sprite tiles and palette
.section ".rodata5" superfree
spr8_tiles:
.incbin "res/spritehero8.pic"
spr8_tiles_end:

spr8_pal:
.incbin "res/spritehero8.pal"
spr8_pal_end:
.ends
