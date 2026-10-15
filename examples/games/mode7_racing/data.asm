;----------------------------------------------------------------------
; mode7_racing — the collision class map (1 byte per tile), written by
; gen_track.py with the track picture. C reads it as a const array.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata3"

; 128x128 bytes: 0 = road, 1 = grass, 2 = wall. 16 KB, in an asset bank
; like any const data; C reads it far.
track_class:
.incbin "res/track_class.bin"
track_class_end:

.ends
