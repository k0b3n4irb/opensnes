;----------------------------------------------------------------------
; play_noise — flat APU image (code only: the drum kit has no samples),
; uploaded to $0200 by apuUpload().
;----------------------------------------------------------------------

.section ".rodata1" superfree

spc_image:      .incbin "player.spc700.bin"
spc_image_end:

.ends
