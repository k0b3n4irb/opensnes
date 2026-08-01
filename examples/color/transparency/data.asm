;==============================================================================
; Transparency — Graphics Data
; Landscape (BG1, 4bpp) + Clouds (BG3, 2bpp)
;==============================================================================

;--- Landscape: 4bpp tiles + tilemap + palette (palette bank 1) ---
.section ".rodata1" superfree

land_tiles:
.incbin "res/backgrounds.pic"
land_tiles_end:

land_map:
.incbin "res/backgrounds.map"
land_map_end:

land_pal:
.incbin "res/backgrounds.pal"
land_pal_end:

.ends

;--- Clouds: 2bpp tiles + tilemap + palette (palette bank 0) ---
.section ".rodata2" superfree

cloud_tiles:
.incbin "res/clouds.pic"
cloud_tiles_end:

cloud_map:
.incbin "res/clouds.map"
cloud_map_end:

cloud_pal:
.incbin "res/clouds.pal"
cloud_pal_end:

.ends

