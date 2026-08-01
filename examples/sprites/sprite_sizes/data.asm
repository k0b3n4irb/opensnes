;==============================================================================
; Object Size - Sprite Data
;==============================================================================

;------------------------------------------------------------------------------
; 8x8 sprite (4bpp)
;------------------------------------------------------------------------------
.section ".rodata1" superfree

sprite8: .incbin "res/sprite8.pic"
sprite8_end:

palsprite8: .incbin "res/sprite8.pal"
palsprite8_end:

.ends

;------------------------------------------------------------------------------
; 16x16 sprite (4bpp)
;------------------------------------------------------------------------------
.section ".rodata2" superfree

sprite16: .incbin "res/sprite16.pic"
sprite16_end:

palsprite16: .incbin "res/sprite16.pal"
palsprite16_end:

.ends

;------------------------------------------------------------------------------
; 32x32 sprite (4bpp)
;------------------------------------------------------------------------------
.section ".rodata3" superfree

sprite32: .incbin "res/sprite32.pic"
sprite32_end:

palsprite32: .incbin "res/sprite32.pal"
palsprite32_end:

.ends

;------------------------------------------------------------------------------
; 64x64 sprite (4bpp)
;------------------------------------------------------------------------------
.section ".rodata4" superfree

sprite64: .incbin "res/sprite64.pic"
sprite64_end:

palsprite64: .incbin "res/sprite64.pal"
palsprite64_end:

.ends
