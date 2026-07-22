;----------------------------------------------------------------------
; soundboard — four BRR samples, loaded at runtime by audioLoadSample()
; (streamed into APU RAM through the audio v2 driver — no .spc700.asm
; in this example at all).
;----------------------------------------------------------------------

.section ".rodata1" superfree

brr_cello:      .incbin "res/cello.brr"
brr_cello_end:

.ends

.section ".rodata2" superfree

brr_au:         .incbin "res/au.brr"
brr_au_end:
brr_ss:         .incbin "res/ss.brr"
brr_ss_end:
brr_pp:         .incbin "res/pp.brr"
brr_pp_end:

.ends
