;==============================================================================
; GetColors - Graphics Data
; Ported from PVSnesLib to OpenSNES
;==============================================================================

.section ".rodata1" superfree

tiles:
.incbin "res/ortf.pic"
tiles_end:

tilemap:
.incbin "res/ortf.map"
tilemap_end:

palette:
.incbin "res/ortf.pal"
palette_end:

.ends
