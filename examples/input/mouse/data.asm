;==============================================================================
; Mouse Example - Graphics Data
;==============================================================================

ASSET_SECTION ".rodata1"

; 16x16 cursor sprite tiles (4bpp)
cursor_tiles:
.incbin "res/cursor.pic"
cursor_tiles_end:

; Cursor palette
cursor_pal:
.incbin "res/cursor.pal"
cursor_pal_end:

.ends
