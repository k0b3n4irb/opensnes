;==============================================================================
; Mouse Example - Graphics Data
;==============================================================================

.section ".rodata1" superfree

; 16x16 cursor sprite tiles (4bpp)
cursor_tiles:
.incbin "res/cursor.pic"
cursor_tiles_end:

; Cursor palette
cursor_pal:
.incbin "res/cursor.pal"
cursor_pal_end:

.ends
