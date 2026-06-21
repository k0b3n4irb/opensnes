;----------------------------------------------------------------------
; Mode 3 background data (256 colors, 8bpp)
;
; Tileset is >32KB — split across two SUPERFREE sections so the linker
; can place each half in any free bank. Post-A6+A7, C pointers carry
; the bank byte and dmaCopyVram reads it directly; no ASM loader needed.
;----------------------------------------------------------------------

.section ".rodata1" superfree

; First 32KB of tiles (may end up in bank $01+)
tiles:      .incbin "res/bg.pic" skip 0 read 32768
tiles_end:

.ends

.section ".rodata2" superfree

; Remaining tiles
tiles2:     .incbin "res/bg.pic" skip 32768
tiles2_end:

; Tilemap + palette
tilemap:    .incbin "res/bg.map"
tilemap_end:

palette:    .incbin "res/bg.pal"
palette_end:

.ends
