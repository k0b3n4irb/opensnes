ASSET_SECTION ".rodata1"
tiles:
.incbin "res/opensnes.pic"
tiles_end:
.ends

ASSET_SECTION ".rodata2"
tilemap:
.incbin "res/opensnes.map"
tilemap_end:

palette:
.incbin "res/opensnes.pal"
palette_end:
.ends
