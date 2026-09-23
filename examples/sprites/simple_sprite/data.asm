; Simple Sprite - Sprite Data
; 32x32 sprite tiles and palette

ASSET_SECTION ".rodata1"

sprite32:
.incbin "res/sprite32.pic"
sprite32_end:

palsprite32:
.incbin "res/sprite32.pal"

.ends
