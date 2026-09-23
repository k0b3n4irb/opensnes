;----------------------------------------------------------------------
; echo — one short "pop" BRR one-shot, loaded at runtime by
; audioLoadSample() (the pop sample is shared with soundboard).
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

brr_pop:    .incbin "res/pp.brr"
brr_pop_end:

.ends
