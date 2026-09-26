;==============================================================================
; Continuous Scroll Example - Graphics Data
;
; This file contains only data definitions. All graphics loading is done
; in C using library functions (bgInitTileSet, dmaCopyVram, oamInitGfxSet).
;==============================================================================

;------------------------------------------------------------------------------
; BG1 - Main scrolling background
;------------------------------------------------------------------------------
ASSET_SECTION ".rodata1"

bg1_tiles: .incbin "res/BG1.pic"
bg1_tiles_end:

bg1_pal: .incbin "res/BG1.pal"
bg1_pal_end:

bg1_map: .incbin "res/BG1.map"
bg1_map_end:

.ends

;------------------------------------------------------------------------------
; BG2 - Sub scrolling background (parallax)
;------------------------------------------------------------------------------
ASSET_SECTION ".rodata2"

bg2_tiles: .incbin "res/BG2.pic"
bg2_tiles_end:

bg2_pal: .incbin "res/BG2.pal"
bg2_pal_end:

bg2_map: .incbin "res/BG2.map"
bg2_map_end:

.ends

;------------------------------------------------------------------------------
; BG3 - Static HUD/overlay
;------------------------------------------------------------------------------
ASSET_SECTION ".rodata3"

bg3_tiles: .incbin "res/BG3.pic"
bg3_tiles_end:

bg3_pal: .incbin "res/BG3.pal"
bg3_pal_end:

bg3_map: .incbin "res/BG3.map"
bg3_map_end:

.ends

;------------------------------------------------------------------------------
; Character sprite
;------------------------------------------------------------------------------
ASSET_SECTION ".rodata4"

char_tiles: .incbin "res/character.pic"
char_tiles_end:

char_pal: .incbin "res/character.pal"
char_pal_end:

.ends
