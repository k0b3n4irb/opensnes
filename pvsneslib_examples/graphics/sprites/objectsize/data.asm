;==============================================================================
; ObjectSize - Sprite Data
; Ported from PVSnesLib to OpenSNES
;==============================================================================

.section ".rodata1" superfree

sprite8:
.incbin "res/sprite8.pic"
sprite8_end:

palsprite8:
.incbin "res/sprite8.pal"

sprite16:
.incbin "res/sprite16.pic"
sprite16_end:

palsprite16:
.incbin "res/sprite16.pal"

sprite32:
.incbin "res/sprite32.pic"
sprite32_end:

palsprite32:
.incbin "res/sprite32.pal"

sprite64:
.incbin "res/sprite64.pic"
sprite64_end:

palsprite64:
.incbin "res/sprite64.pal"

.ends
