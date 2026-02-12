;==============================================================================
; Window Effect - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; Two backgrounds for the window demo: pvsneslibbg1 and pvsneslibbg2
; Each has tiles, tilemap, and palette (16 colors / 4bpp).
;==============================================================================

.section ".rodata1" superfree

bg1_tiles:
.incbin "res/pvsneslibbg1.pic"
bg1_tiles_end:

bg1_map:
.incbin "res/pvsneslibbg1.map"
bg1_map_end:

bg1_pal:
.incbin "res/pvsneslibbg1.pal"
bg1_pal_end:

.ends

.section ".rodata2" superfree

bg2_tiles:
.incbin "res/pvsneslibbg2.pic"
bg2_tiles_end:

bg2_map:
.incbin "res/pvsneslibbg2.map"
bg2_map_end:

bg2_pal:
.incbin "res/pvsneslibbg2.pal"
bg2_pal_end:

.ends
