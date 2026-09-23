;----------------------------------------------------------------------
; apu_switch — two flat APU images, hot-swapped at runtime by
; apuReset() + apuUpload()/apuExecute().
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

spc_drums:      .incbin "drums.spc700.bin"
spc_drums_end:

.ends

ASSET_SECTION ".rodata2"

spc_cello:      .incbin "cello.spc700.bin"
spc_cello_end:

.ends
