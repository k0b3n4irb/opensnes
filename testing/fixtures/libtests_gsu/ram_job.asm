;==============================================================================
; A GSU job run from ROM (SCMR RON = 1) while the CPU keeps working, from
; the RAM code window (RAM_CODE_SIZE in the Makefile, RAM_CODE_SECTION)
;==============================================================================
; ramRunRomJob starts the program set by gsuSetProgram() the way gsuLaunch()
; does, but its wait loop is ordinary linked code — a call into the window,
; labels at $7E:xxxx — instead of a hand-copied position-independent stub.
; While the GSU owns the ROM the loop counts its polls through a subroutine
; (a JSR inside the window) and the NMI keeps counting frames (crt0's WRAM
; NMI, gsu_owns_cart set).
;==============================================================================
.ifdef SUPERFX
.RAMSECTION ".ram_job_results" BANK 0 SLOT 1
r_ram_frames: dsb 2     ; frame_count advance during the job
r_ram_polls:  dsb 2     ; wait-loop iterations (saturates at $FFFF)
.ENDS

RAM_CODE_SECTION "rom_job"
.ACCU 16
.INDEX 16
ramRunRomJob:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    lda #$01
    sta.l gsu_owns_cart     ; from here the ROM is the GSU's
    lda.l gsu_cfgr
    sta.l $3037             ; CFGR
    lda #$01
    sta.l $3039             ; CLSR: 21 MHz
    lda.l gsu_scbr
    sta.l $3038             ; SCBR
    lda.l gsu_scmr
    ora #$18
    sta.l $303A             ; SCMR: RON + RAN
    lda.l gsu_prog_bank
    sta.l $3034             ; PBR
    rep #$20
    .ACCU 16
    lda.l frame_count
    sta.l r_ram_frames
    ldx #$0000
    lda.l gsu_prog_addr
    sta.l $301E             ; R15: the GSU starts
    sep #$20
    .ACCU 8
@wait:
    jsr _ram_job_count
    lda.l $3030
    and #$20                ; SFR GO
    bne @wait
    lda #$00
    sta.l $303A             ; the ROM and RAM back to the CPU
    sta.l gsu_owns_cart
    rep #$20
    .ACCU 16
    lda.l frame_count
    sec
    sbc.l r_ram_frames
    sta.l r_ram_frames
    txa
    sta.l r_ram_polls
    plp
    rtl

_ram_job_count:
    cpx #$FFFF
    beq +
    inx
+   rts
.ENDS
.endif
