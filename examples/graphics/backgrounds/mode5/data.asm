;----------------------------------------------------------------------
; Mode 5 background data
;----------------------------------------------------------------------

.section ".rodata1" superfree

tiles:      .incbin "res/bg.pic"
tiles_end:

tilemap:    .incbin "res/bg.map"
tilemap_end:

palette:    .incbin "res/bg.pal"
palette_end:

.ends
