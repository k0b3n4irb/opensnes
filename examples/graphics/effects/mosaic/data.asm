.section ".rodata1" superfree
tiles:
.incbin "res/pvsneslib.pic"
tiles_end:
.ends

.section ".rodata2" superfree
tilemap:
.incbin "res/pvsneslib.map"
tilemap_end:

palette:
.incbin "res/pvsneslib.pal"
palette_end:
.ends
