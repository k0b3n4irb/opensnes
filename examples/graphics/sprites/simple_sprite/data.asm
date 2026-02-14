; Simple Sprite - Sprite Data
; 32x32 sprite tiles and palette

.section ".rodata1" superfree

sprite32:
.incbin "res/sprite32.pic"
sprite32_end:

palsprite32:
.incbin "res/sprite32.pal"

.ends
