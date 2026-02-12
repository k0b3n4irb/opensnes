;==============================================================================
; memory_mapping - Font Data
; Ported from PVSnesLib to OpenSNES
;==============================================================================

.section ".rodata1" superfree

tilfont:
.incbin "res/pvsneslibfont.pic"

palfont:
.incbin "res/pvsneslibfont.pal"

.ends
