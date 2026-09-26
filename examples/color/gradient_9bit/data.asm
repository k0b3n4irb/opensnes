;----------------------------------------------------------------------
; "9-bit" gradient — krom (Peter Lemon)'s exact HDMA tables, extracted
; verbatim from RedSpace9BitHDMA.asm (the dither patterns ARE the
; technique: 224 x [1][CGADD word 0][color word] and 224 x
; [1][brightness], each with jittered values that average into
; sub-5-bit gradient steps).
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

color_table:        .incbin "res/color_table.bin"
brightness_table:   .incbin "res/brightness_table.bin"

.ends
