;==============================================================================
; Parallax Scrolling - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; Background image: 512x256 starfield/grass scene split into 3 parallax zones.
; Converted with: gfx4snes -s 8 -o 32 -u 16 -p -m -i res/back.png
;==============================================================================

ASSET_SECTION ".rodata1"

tiles:
.incbin "res/back.pic"
tiles_end:

.ends

ASSET_SECTION ".rodata2"

tilemap:
.incbin "res/back.map"

palette:
.incbin "res/back.pal"

.ends
