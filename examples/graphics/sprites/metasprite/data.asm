;==============================================================================
; 16x16 Sprite Test - Graphics Data
;==============================================================================

.section ".rodata1" superfree

; 16x16 sprite tiles (4bpp, converted with gfx4snes -s 16 -p)
sprite_tiles:
.incbin "res/sprite.pic"
sprite_tiles_end:

; Sprite palette
sprite_pal:
.incbin "res/sprite.pal"
sprite_pal_end:

.ends
