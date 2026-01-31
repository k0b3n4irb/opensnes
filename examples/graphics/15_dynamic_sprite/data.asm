;==============================================================================
; Dynamic Sprite Engine Example - Graphics Data
;==============================================================================

.section ".rodata1" superfree

; 16x16 sprite tiles (4bpp)
; Source: sprite16_grid.png = 128x48 = 24 frames of 16x16 sprites
; Organized for VRAM layout (128 pixels wide = 16 tiles per row)
spr16_tiles:
.incbin "res/sprite16_grid.pic"
spr16_tiles_end:

; 16x16 sprite palette (32 bytes, 16 colors)
; Proper palette generated from the source PNG
spr16_properpal:
.incbin "res/sprite16_proper.pal"
spr16_properpal_end:

.ends

.section ".rodata2" superfree

; 32x32 sprite tiles (4bpp)
; Source: sprite32.png = 32x256 = 8 frames of 32x32
spr32_tiles:
.incbin "res/sprite32.pic"
spr32_tiles_end:

; 32x32 sprite palette
spr32_pal:
.incbin "res/sprite32.pal"
spr32_pal_end:

.ends
