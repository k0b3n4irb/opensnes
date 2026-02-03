;==============================================================================
; Simple Sprite - Graphics Data
;
; This file contains only data definitions. All graphics loading is done
; in C using library functions (oamInitGfxSet).
;==============================================================================

.section ".rodata1" superfree

;------------------------------------------------------------------------------
; Sprite tiles (4bpp, 32x32 sprite)
;------------------------------------------------------------------------------
sprite_tiles: .incbin "res/sprites.pic"
sprite_tiles_end:

;------------------------------------------------------------------------------
; Sprite palette (16 colors)
;------------------------------------------------------------------------------
sprite_pal: .incbin "res/sprites.pal"
sprite_pal_end:

.ends
