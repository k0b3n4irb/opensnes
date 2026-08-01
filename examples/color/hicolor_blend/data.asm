;----------------------------------------------------------------------
; "3840 colors" blend — channel-split Mode 3 data (original art,
; gen_assets.py; committed outputs per the wavetable precedent).
; GB layer: 8bpp tiles with indices pre-offset to CGRAM 16-255.
; R layer: 4bpp red ramp on palette 0 (CGRAM 1-15).
;----------------------------------------------------------------------

.section ".rodata1" superfree

gb_tiles:       .incbin "res/gb.pic"
gb_tiles_end:

gb_map:         .incbin "res/gb.map"
gb_map_end:

.ends

.section ".rodata2" superfree

r_tiles:        .incbin "res/r.pic"
r_tiles_end:

r_map:          .incbin "res/r.map"
r_map_end:

blend_pal:      .incbin "res/blend.pal"
blend_pal_end:

.ends
