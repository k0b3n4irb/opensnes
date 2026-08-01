.section ".rodata1" superfree
tiles:
.incbin "res/opensnes.pic"
tiles_end:
.ends

.section ".rodata2" superfree
tilemap:
.incbin "res/opensnes.map"
tilemap_end:

palette:
.incbin "res/opensnes.pal"
palette_end:
.ends
