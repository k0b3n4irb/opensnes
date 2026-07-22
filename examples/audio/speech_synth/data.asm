;----------------------------------------------------------------------
; speech_synth — flat APU image (code + phoneme directory + bank),
; uploaded to $0200 by apuUpload(). Phonemes are original
; (gen_phonemes.py, committed).
;----------------------------------------------------------------------

.section ".rodata1" superfree

spc_image:      .incbin "player.spc700.bin"
spc_image_end:

.ends
