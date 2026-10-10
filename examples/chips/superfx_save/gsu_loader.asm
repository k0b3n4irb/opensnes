; The GSU program binary, in the ROM where a job would read it. Not run here.
.ifdef SUPERFX
GSU_SECTION ".gsu_code"
gsu_program:
    .incbin "gsu_idle.sfx.bin"
gsu_program_end:
.ENDS
.endif
