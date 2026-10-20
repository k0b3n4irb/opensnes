;----------------------------------------------------------------------
; mode7_racing — the data no tool converts, written by
; devtools/art_assets/build.py (racing.py) with the circuit picture:
; the collision class map and the rivals' two lanes. C reads them as
; const arrays.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata3"

; 128x128 bytes: 0 = road, 1 = gravel, 2 = water or a prop, 3 = boost
; pad. 16 KB, in an asset bank like any const data; C reads it far.
track_class:
.incbin "res/track_class.bin"
track_class_end:

; 2 lanes x 256 points, each x then y as a little-endian word (pixels)
lanes_xy:
.incbin "res/lanes_xy.bin"

; the heading at each of those points, one byte (0-255, 64 = east)
lanes_heading:
.incbin "res/lanes_heading.bin"

.ends
