;==============================================================================
; HDMA Gradient - Graphics Data
; Ported from PVSnesLib to OpenSNES
;
; Mode 3 (256 colors / 8bpp) with tile data >32KB.
; The .pic file is 39744 bytes (621 tiles x 64 bytes/tile for 8bpp).
; Must be split across two sections since a single LoROM bank is 32KB max.
;
; First section: bytes 0-$7FFF (32768 bytes)
; Second section: bytes $8000-$9B3F (6976 bytes = 39744 - 32768)
;==============================================================================

; First part of tiles (32KB)
.section ".rodata1" superfree
tiles_part1:
.incbin "res/pvsneslib.pic" SKIP 0 READ $8000
tiles_part1_end:
.ends

; Second part of tiles (remaining ~7KB)
.section ".rodata2" superfree
tiles_part2:
.incbin "res/pvsneslib.pic" SKIP $8000
tiles_part2_end:
.ends

; Tilemap and palette
.section ".rodata3" superfree
tilemap:
.incbin "res/pvsneslib.map"
tilemap_end:

palette:
.incbin "res/pvsneslib.pal"
palette_end:
.ends
