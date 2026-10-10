;----------------------------------------------------------------------
; HDMA Wave — Mode 3 background data (256 colors, 8bpp) + krom's table
;
; Original procedural water-caustics art (res/water.bmp, generated —
; no krom assets). Tileset >32KB: split across two SUPERFREE sections
; (LoROM bank limit); post-A6 C pointers carry the bank byte and
; dmaCopyVram reads it directly.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata2"

; krom's exact HDMA wave table, extracted verbatim from WaveHDMA.asm
; (896 entries [1][off16] + terminator; his generator's quasi-period is
; ~25.8 lines, hence the 672-entry seamless wrap). Data of the original
; demo's technique, credited — the ART assets remain original.
wavetable:  .incbin "res/wavetable.bin"
wavetable_end:

.ends
