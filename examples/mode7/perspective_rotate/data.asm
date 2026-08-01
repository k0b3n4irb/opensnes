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

.section ".rodata1" superfree

ground_pc7:     .incbin "res/ground.pc7"
ground_pc7_end:

ground_mp7:     .incbin "res/ground.mp7"
ground_mp7_end:

ground_pal:     .incbin "res/ground.pal"
ground_pal_end:

.ends

.section ".rodata_m7cos" superfree

m7cos:          .incbin "res/m7cos.bin"

.ends

.section ".rodata_m7sin" superfree

m7sin:          .incbin "res/m7sin.bin"

.ends

.section ".rodata_m7nsin" superfree

m7nsin:         .incbin "res/m7nsin.bin"

.ends
