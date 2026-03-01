;==============================================================================
; HDMAGradient - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; 256-color (8bpp) Mode 3 image. Tile data is ~39KB (>32KB),
; split into 2 SUPERFREE sections for the DMA transfer.
;==============================================================================

; First 32KB of tile data
.section ".rodata_tiles1" superfree

pvsneslib_tiles:
.incbin "pvsneslib.pic" READ 32768

.ends

; Remaining tile data (39744 - 32768 = 6976 bytes)
.section ".rodata_tiles2" superfree

pvsneslib_tiles2:
.incbin "pvsneslib.pic" SKIP 32768
pvsneslib_tiles2_end:

.ends

; Tilemap and palette
.section ".rodata_map" superfree

pvsneslib_map:
.incbin "pvsneslib.map"
pvsneslib_map_end:

pvsneslib_palette:
.incbin "pvsneslib.pal"
pvsneslib_palette_end:

.ends
