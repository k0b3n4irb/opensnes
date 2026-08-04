;----------------------------------------------------------------------
; sfx_from_wav — two one-shot BRR samples for audioLoadSample().
;
; The .brr files are NOT committed: the build system generates each from
; the matching res/*.wav with wav2brr (the %.brr: %.wav rule in
; make/common.mk), because this .incbin makes the .brr a prerequisite.
;----------------------------------------------------------------------

.section ".rodata1" superfree

blip_brr:       .incbin "res/blip.brr"
blip_brr_end:

coin_brr:       .incbin "res/coin.brr"
coin_brr_end:

.ends
