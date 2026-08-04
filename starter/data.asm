;----------------------------------------------------------------------
; Asset data linked into the ROM.
;
; res/player.png is converted to res/player.pic (tiles) + res/player.pal
; (palette) by gfx4snes at build time (see the Makefile), and .incbin'd
; here. The labels become the C symbols main.c declares as `extern`.
;
; Add your own graphics by dropping a .png in res/, giving it a gfx4snes
; rule in the Makefile, and .incbin'ing its .pic/.pal below.
;----------------------------------------------------------------------

.section ".rodata1" superfree

player:
.incbin "res/player.pic"
player_end:

player_pal:
.incbin "res/player.pal"

.ends
