;==============================================================================
; Window HDMA Triangle - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; Two backgrounds: bg1 (palette slot 0) and bg2 (palette slot 1)
; Converted with: gfx4snes -s 8 -o 16 -u 16 -p -m -i res/bg1.png
;                 gfx4snes -s 8 -o 16 -u 16 -e 1 -p -m -i res/bg2.png
;==============================================================================

.section ".rodata1" superfree

tiles_bg1:
.incbin "res/bg1.pic"
tiles_bg1_end:

tilemap_bg1:
.incbin "res/bg1.map"
tilemap_bg1_end:

palette_bg1:
.incbin "res/bg1.pal"

.ends

.section ".rodata2" superfree

tiles_bg2:
.incbin "res/bg2.pic"
tiles_bg2_end:

tilemap_bg2:
.incbin "res/bg2.map"
tilemap_bg2_end:

palette_bg2:
.incbin "res/bg2.pal"

.ends
