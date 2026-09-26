; Mode 7 - Asset data
;
; Mode 7 VRAM layout is interleaved:
;   Low bytes  (VMDATAL) = tilemap (128x128 tile indices)
;   High bytes (VMDATAH) = tile pixel data (8bpp, 256 tiles x 64 bytes)
;
; Loaded by dmaCopyVramMode7() (two DMAs with different VMAIN settings).

; Mode 7 tile pixel data (8bpp)
ASSET_SECTION ".rodata1"
mode7_tiles:
.incbin "res/mode7bg.pc7"
mode7_tiles_end:
.ends

; Mode 7 tilemap (128x128 = 16384 bytes)
ASSET_SECTION ".rodata2"
mode7_map:
.incbin "res/mode7bg.mp7"
mode7_map_end:

mode7_pal:
.incbin "res/mode7bg.pal"
mode7_pal_end:
.ends
