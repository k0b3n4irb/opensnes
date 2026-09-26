;==============================================================================
; Fading Effect - Graphics Data
;
; This file contains only data definitions. All graphics loading is done
; in C using library functions (bgInitTileSet, dmaCopyVram).
;==============================================================================

ASSET_SECTION ".rodata1"

;------------------------------------------------------------------------------
; Background tiles (4bpp, 16 colors)
;------------------------------------------------------------------------------
tiles: .incbin "res/opensnes.pic"
tiles_end:

;------------------------------------------------------------------------------
; Background tilemap
;------------------------------------------------------------------------------
tilemap: .incbin "res/opensnes.map"
tilemap_end:

;------------------------------------------------------------------------------
; Background palette (16 colors)
;------------------------------------------------------------------------------
palette: .incbin "res/opensnes.pal"
palette_end:

.ends
