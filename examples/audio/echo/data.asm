;----------------------------------------------------------------------
; echo — one short "pop" BRR one-shot, loaded at runtime by
; audioLoadSample() (the pop sample is shared with soundboard).
;----------------------------------------------------------------------

.section ".rodata1" superfree

brr_pop:    .incbin "res/pp.brr"
brr_pop_end:

.ends
