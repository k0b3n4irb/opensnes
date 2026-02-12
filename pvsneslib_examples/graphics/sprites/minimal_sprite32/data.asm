;==============================================================================
; Minimal 32x32 Sprite Test - Sprite Data
; Uses pre-converted assets from objectsize example
;==============================================================================

.section ".rodata1" superfree

sprite32:
.incbin "../objectsize/res/sprite32.pic"
sprite32_end:

palsprite32:
.incbin "../objectsize/res/sprite32.pal"

.ends
