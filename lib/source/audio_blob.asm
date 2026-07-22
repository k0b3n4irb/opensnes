;==============================================================================
; Audio System v2 — embedded SPC700 driver image
;==============================================================================
; The driver is BUILT FROM SOURCE (audio_driver.spc700.asm) by the lib
; Makefile via wla-spc700 (no checked-in blob — unlike the snesmod
; sm_spc.asm precedent). audioInit() uploads this image to APU RAM
; $0200 through the apu module and starts it.
;
; The generated .bin is written next to this file and gitignored.
;==============================================================================

.include "memmap.inc"

.SECTION ".audio_blob" SUPERFREE

audio_driver_blob:
    .incbin "audio_driver.spc700.bin"
audio_driver_blob_end:

.ENDS
