;----------------------------------------------------------------------
; HiColor — Mode 3 BG2 4bpp data with per-tile-row palettes
;
; Original sunset art (res/sunset.png, procedurally generated — no krom
; assets), converted by devtools/hicolor64.py to krom's exact asset
; contract: 896 sequential 4bpp tiles (28672 bytes) + 28 rows x 64
; colors of palette data (3584 bytes) streamed to CGRAM per scanline.
; The tilemap is generated in C (fixed pattern — see main.c).
;----------------------------------------------------------------------

.section ".rodata1" superfree

sunset_pic:     .incbin "res/sunset.pic"
sunset_pic_end:

.ends

.section ".rodata2" superfree

sunset_pal:     .incbin "res/sunset.pal"
sunset_pal_end:

.ends
