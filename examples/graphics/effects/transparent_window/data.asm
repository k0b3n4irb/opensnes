;==============================================================================
; Transparent Window - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; Single background image.
; Converted with: gfx4snes -s 8 -o 16 -u 16 -p -m -i res/background.png
;==============================================================================

.section ".rodata1" superfree

tiles:
.incbin "res/background.pic"
tiles_end:

tilemap:
.incbin "res/background.map"
tilemap_end:

palette:
.incbin "res/background.pal"

.ends
