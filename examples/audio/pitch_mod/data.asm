;----------------------------------------------------------------------
; pitch_mod — flat APU image (code + directory + LFO block + cello BRR),
; uploaded to $0200 by apuUpload().
;----------------------------------------------------------------------

.section ".rodata1" superfree

spc_image:      .incbin "player.spc700.bin"
spc_image_end:

.ends
