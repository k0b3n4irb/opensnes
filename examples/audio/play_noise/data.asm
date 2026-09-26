;----------------------------------------------------------------------
; play_noise — flat APU image (code only: the drum kit has no samples),
; uploaded to $0200 by apuUpload().
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

spc_image:      .incbin "player.spc700.bin"
spc_image_end:

.ends
