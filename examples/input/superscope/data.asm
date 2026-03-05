;==============================================================================
; Super Scope Example - Graphics Data
;==============================================================================

.section ".rodata1" superfree

; Background: aim calibration target (256x224, 2bpp, 126 tiles)
aim_target_tiles:
.incbin "res/aim_adjust_target.pic"
aim_target_tiles_end:

; Background tilemap (32x28)
aim_target_map:
.incbin "res/aim_adjust_target.map"
aim_target_map_end:

; Background palette (4 colors)
aim_target_pal:
.incbin "res/aim_adjust_target.pal"
aim_target_pal_end:

.ends

.section ".rodata2" superfree

; Sprite tiles (128x192 sheet, 4bpp, 384 tiles)
sprites_tiles:
.incbin "res/sprites.pic"
sprites_tiles_end:

; Sprite palette (48 colors = 3 sub-palettes)
sprites_pal:
.incbin "res/sprites.pal"
sprites_pal_end:

.ends
