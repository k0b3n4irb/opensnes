; DynamicSprite - Asset data
; Sprite tiles and palette for animated dynamic sprites

.section ".rodata1" superfree
spr_tiles:
.incbin "res/sprites.pic"
spr_tiles_end:

spr_pal:
.incbin "res/sprites.pal"
spr_pal_end:
.ends
