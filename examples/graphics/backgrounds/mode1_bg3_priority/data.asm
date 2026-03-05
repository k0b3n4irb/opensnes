;==============================================================================
; Mode 1 BG3 High Priority - Graphics Data
;==============================================================================

;------------------------------------------------------------------------------
; BG1 tiles and palette (4bpp, palette slot 2)
;------------------------------------------------------------------------------
.section ".rodata1" superfree

bg1_tiles: .incbin "res/BG1.pic"
bg1_tiles_end:

bg1_pal: .incbin "res/BG1.pal"
bg1_pal_end:

.ends

;------------------------------------------------------------------------------
; BG2 tiles and palette (4bpp, palette slot 4)
;------------------------------------------------------------------------------
.section ".rodata2" superfree

bg2_tiles: .incbin "res/BG2.pic"
bg2_tiles_end:

bg2_pal: .incbin "res/BG2.pal"
bg2_pal_end:

.ends

;------------------------------------------------------------------------------
; BG3 tiles and palette (2bpp, palette slot 0)
;------------------------------------------------------------------------------
.section ".rodata3" superfree

bg3_tiles: .incbin "res/BG3.pic"
bg3_tiles_end:

bg3_pal: .incbin "res/BG3.pal"
bg3_pal_end:

.ends

;------------------------------------------------------------------------------
; Tilemaps (all 32x32 = 2048 bytes each)
;------------------------------------------------------------------------------
.section ".rodata4" superfree

bg1_map: .incbin "res/BG1.map"
bg1_map_end:

bg2_map: .incbin "res/BG2.map"
bg2_map_end:

bg3_map: .incbin "res/BG3.map"
bg3_map_end:

.ends
