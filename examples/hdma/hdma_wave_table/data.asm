;----------------------------------------------------------------------
; HDMA Wave Table — Mode 3 background data (256 colors, 8bpp)
;
; Original procedural water-caustics art (res/water.bmp, generated —
; no krom assets). Tileset >32KB: split across two SUPERFREE sections
; (LoROM bank limit); post-A6 C pointers carry the bank byte and
; dmaCopyVram reads it directly.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

tiles:      .incbin "res/water.pic" skip 0 read 32768
tiles_end:

.ends

ASSET_SECTION ".rodata2"

tiles2:     .incbin "res/water.pic" skip 32768
tiles2_end:

tilemap:    .incbin "res/water.map"
tilemap_end:

palette:    .incbin "res/water.pal"
palette_end:

; krom's exact HDMA wave table, extracted verbatim from WaveHDMA.asm
; (896 entries [1][off16] + terminator; his generator's quasi-period is
; ~25.8 lines, hence the 672-entry seamless wrap). Data of the original
; demo's technique, credited — the ART assets remain original.
wavetable:  .incbin "res/wavetable.bin"
wavetable_end:

.ends
