;----------------------------------------------------------------------
; Mode 7 rotating perspective — ground assets + krom's matrix tables
;
; Ground art is original (9 prefab tiles composed into a 128x128 Mode 7
; world by a tile-conscious generator — res/ground.png). The three table
; banks are krom (Peter Lemon)'s exact HDMA tables from the Perspective
; demo, extracted verbatim (48 angles x 224 scanlines of
; trig(2*PI*a/48)*20480/y in 8.8 fixed point, full [count][val16] HDMA
; entries + terminator; 673 bytes per angle). One SUPERFREE section per
; blob keeps each within a single LoROM bank.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

ground_pc7:     .incbin "res/ground.pc7"
ground_pc7_end:

ground_mp7:     .incbin "res/ground.mp7"
ground_mp7_end:

ground_pal:     .incbin "res/ground.pal"
ground_pal_end:

.ends

ASSET_SECTION ".rodata_m7cos"

m7cos:          .incbin "res/m7cos.bin"

.ends

ASSET_SECTION ".rodata_m7sin"

m7sin:          .incbin "res/m7sin.bin"

.ends

ASSET_SECTION ".rodata_m7nsin"

m7nsin:         .incbin "res/m7nsin.bin"

.ends
