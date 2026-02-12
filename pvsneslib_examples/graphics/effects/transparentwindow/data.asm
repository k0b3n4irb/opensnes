.section ".rodata1" superfree
tiles:
.incbin "res/background.pic"
tiles_end:
.ends

.section ".rodata2" superfree
tilemap:
.incbin "res/background.map"
tilemap_end:

palette:
.incbin "res/background.pal"
palette_end:
.ends
