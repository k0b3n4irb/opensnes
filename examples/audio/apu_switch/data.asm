;----------------------------------------------------------------------
; apu_switch — two flat APU images, hot-swapped at runtime by
; apuReset() + apuUpload()/apuExecute().
;----------------------------------------------------------------------

.section ".rodata1" superfree

spc_drums:      .incbin "drums.spc700.bin"
spc_drums_end:

.ends

.section ".rodata2" superfree

spc_cello:      .incbin "cello.spc700.bin"
spc_cello_end:

.ends
