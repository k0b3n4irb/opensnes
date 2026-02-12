.section ".rodata1" superfree
tiles:
.incbin "res/map_512_512.pic"
tiles_end:
.ends

.section ".rodata2" superfree
tilemap:
.incbin "res/map_512_512.map"
tilemap_end:

palette:
.incbin "res/map_512_512.pal"
palette_end:
.ends
