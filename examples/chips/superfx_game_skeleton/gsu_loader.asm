;==============================================================================
; The GSU renderer binary for main.c
;==============================================================================
.ifdef SUPERFX

GSU_SECTION ".gsu_cube"
gsu_cube:
    .incbin "gsu_cube.sfx.bin"
gsu_cube_end:
.ENDS

.endif
