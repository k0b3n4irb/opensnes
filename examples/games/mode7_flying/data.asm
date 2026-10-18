;----------------------------------------------------------------------
; mode7_flying — the landing class map (1 byte per tile), written by
; gen_terrain.py with the terrain picture. C reads it as a const array.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata3"

; 128x128 bytes: 0 = field, 1 = water, 2 = landing pad.
terrain_class:
.incbin "res/terrain_class.bin"
terrain_class_end:

.ends
