;==============================================================================
; SNESMOD SPC700 driver — embedded image
;==============================================================================
; The driver is built from source (sm_spc.spc700.asm, Mukunda Johnson's
; SNESMOD driver in WLA-DX syntax) by the lib Makefile via wla-spc700, as
; the audio engine's driver is. snesmod.asm includes this file and uploads
; SM_SPC..SM_SPC_end to the SPC700 at $0400.
;
; The generated .bin is written next to this file and gitignored.
;==============================================================================

.SECTION ".sm_spc" SUPERFREE

SM_SPC:
	.incbin "sm_spc.spc700.bin"
SM_SPC_end:

.ENDS
