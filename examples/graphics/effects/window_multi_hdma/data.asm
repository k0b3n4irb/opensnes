;----------------------------------------------------------------------
; Multi-window HDMA — Mode 3 ring artwork (original, procedural;
; gfx4snes -s 8 -o 256 -u 256 -p -m). The HDMA window table lives in
; main.c (krom's exact 14-band values).
;----------------------------------------------------------------------

.section ".rodata1" superfree

rings_pic:      .incbin "res/rings.pic"
rings_pic_end:

.ends

.section ".rodata2" superfree

rings_map:      .incbin "res/rings.map"
rings_map_end:

rings_pal:      .incbin "res/rings.pal"
rings_pal_end:

.ends
