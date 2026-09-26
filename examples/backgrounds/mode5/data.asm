;----------------------------------------------------------------------
; Mode 5 background data
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

tiles:      .incbin "res/bg.pic"
tiles_end:

tilemap:    .incbin "res/bg.map"
tilemap_end:

palette:    .incbin "res/bg.pal"
palette_end:

.ends
