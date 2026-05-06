;==============================================================================
; DynamicMap - ROM Data
;
; Sprite tile graphics (8bpp, 256 colors) and C64 sprite data.
;==============================================================================

;------------------------------------------------------------------------------
; Sprite graphics for 32x32 mode (8x8 tile size, 256 bytes per 16x16 sprite)
;------------------------------------------------------------------------------
.section ".rodata1" superfree

sprite16:
.incbin "res/sprite16.pic"
sprite16_end:

palsprite16:
.incbin "res/sprite16.pal"
palsprite16_end:

.ends

;------------------------------------------------------------------------------
; Sprite graphics for 64x64 mode (16x16 tile size, 2048 bytes per sprite)
;------------------------------------------------------------------------------
.section ".rodata2" superfree

sprite16_64x64:
.incbin "res/sprite16_64x64.pic"
sprite16_64x64_end:

palsprite16_64x64:
.incbin "res/sprite16_64x64.pal"
palsprite16_64x64_end:

.ends
