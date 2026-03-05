;==============================================================================
; Mode 1 Mixed Scroll - Graphics Data
;
; Two background layers:
;   BG1 (shader):   tiles at VRAM $4000, map at $1800, palette slot 1
;   BG2 (pvsneslib): tiles at VRAM $5000, map at $1400, palette slot 0
;==============================================================================

;------------------------------------------------------------------------------
; BG2 (pvsneslib logo) - palette slot 0
;------------------------------------------------------------------------------
.section ".rodata1" superfree

bg2_tiles: .incbin "res/pvsneslib.pic"
bg2_tiles_end:

bg2_map: .incbin "res/pvsneslib.map"
bg2_map_end:

bg2_pal: .incbin "res/pvsneslib.pal"
bg2_pal_end:

.ends

;------------------------------------------------------------------------------
; BG1 (shader pattern) - palette slot 1
;------------------------------------------------------------------------------
.section ".rodata2" superfree

bg1_tiles: .incbin "res/shader.pic"
bg1_tiles_end:

bg1_map: .incbin "res/shader.map"
bg1_map_end:

bg1_pal: .incbin "res/shader.pal"
bg1_pal_end:

.ends
