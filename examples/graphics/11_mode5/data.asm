;==============================================================================
; Mode 5 Example - Hi-res 512x256
;
; This file contains only data definitions. All graphics loading is done
; in C using library functions (bgInitTileSet, dmaCopyVram).
;==============================================================================

.section ".rodata1" superfree

;------------------------------------------------------------------------------
; Background tiles (4bpp, 16 colors)
;------------------------------------------------------------------------------
tiles: .incbin "res/pvsneslib.pic"
tiles_end:

;------------------------------------------------------------------------------
; Background tilemap
;------------------------------------------------------------------------------
tilemap: .incbin "res/pvsneslib.map"
tilemap_end:

;------------------------------------------------------------------------------
; Background palette (16 colors)
;------------------------------------------------------------------------------
palette: .incbin "res/pvsneslib.pal"
palette_end:

.ends
