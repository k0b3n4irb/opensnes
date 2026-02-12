.section ".rodata1" superfree
tiles:
.incbin "res/back.pic"
tiles_end:
.ends

.section ".rodata2" superfree
tilemap:
.incbin "res/back.map"
tilemap_end:

palette:
.incbin "res/back.pal"
palette_end:
.ends
