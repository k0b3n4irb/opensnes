;==============================================================================
; Scrolling Demo - Graphics Data
;
; This file contains only data definitions. All graphics loading is done
; in C using library functions (bgInitTileSet, dmaCopyVram).
;==============================================================================

.section ".rodata1" superfree

;------------------------------------------------------------------------------
; Foreground layer (shader) - scrolls at full speed
;------------------------------------------------------------------------------
shader_tiles: .incbin "res/shader.pic"
shader_tiles_end:

shader_map: .incbin "res/shader.map"
shader_map_end:

shader_pal: .incbin "res/shader.pal"
shader_pal_end:

.ends

.section ".rodata2" superfree

;------------------------------------------------------------------------------
; Background layer (pvsneslib logo) - scrolls at half speed for parallax
;------------------------------------------------------------------------------
bg_tiles: .incbin "res/pvsneslib.pic"
bg_tiles_end:

bg_map: .incbin "res/pvsneslib.map"
bg_map_end:

bg_pal: .incbin "res/pvsneslib.pal"
bg_pal_end:

.ends
