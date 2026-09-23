;----------------------------------------------------------------------
; Mode 0 — 4 background layers (2bpp each)
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

t0:     .incbin "res/bg0.pic"
t0_end:
p0:     .incbin "res/bg0.pal"
bgm0:   .incbin "res/bg0.map"
bgm0_end:

t1:     .incbin "res/bg1.pic"
t1_end:
p1:     .incbin "res/bg1.pal"
bgm1:   .incbin "res/bg1.map"
bgm1_end:

.ends

ASSET_SECTION ".rodata2"

t2:     .incbin "res/bg2.pic"
t2_end:
p2:     .incbin "res/bg2.pal"
bgm2:   .incbin "res/bg2.map"
bgm2_end:

t3:     .incbin "res/bg3.pic"
t3_end:
p3:     .incbin "res/bg3.pal"
bgm3:   .incbin "res/bg3.map"
bgm3_end:

.ends
