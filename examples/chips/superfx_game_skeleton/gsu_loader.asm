;==============================================================================
; The GSU renderer binary and the edge upload for main.c
;==============================================================================
.ifdef SUPERFX

GSU_SECTION ".gsu_cube"
gsu_cube:
    .incbin "gsu_cube.sfx.bin"
gsu_cube_end:
.ENDS

.RAMSECTION ".skeleton_edges" BANK 0 SLOT 1
edge_buffer: dsb 48         ; 12 edges x 4 bytes (x0, y0, x1, y1)
.ENDS

;------------------------------------------------------------------------------
; writeEdgesToSRAM — edge_buffer (48 bytes) -> Game Pak RAM $70:8000, past
; the two framebuffers ($70:0000 and $70:4000). Call it while the CPU owns
; the RAM: no job running, or after gsuWait().
;------------------------------------------------------------------------------
.SECTION ".skeleton_edges_copy" SEMIFREE
.ACCU 16
.INDEX 16
writeEdgesToSRAM:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l edge_buffer,x
    sta.l $708000,x
    inx
    cpx #48
    bne -
    plp
    rtl
.ENDS

.endif
