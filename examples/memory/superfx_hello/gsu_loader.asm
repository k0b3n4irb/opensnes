;==============================================================================
; GSU Code Loader — embeds GSU binary and provides launch/wait functions
;==============================================================================

.ifdef SUPERFX

;------------------------------------------------------------------------------
; GSU program binary (assembled by wla-superfx, linked to flat binary)
;------------------------------------------------------------------------------
.SECTION ".gsu_code" SUPERFREE
gsu_program:
    .incbin "gsu_hello.sfx.bin"
gsu_program_end:
.ENDS

;------------------------------------------------------------------------------
; RAM for GSU result
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_vars" BANK 0 SLOT 1
gsu_result: dsb 2
.ENDS

;------------------------------------------------------------------------------
; launchGSU — Configure buses, start GSU, wait for completion, read result
;------------------------------------------------------------------------------
; After return: gsu_result contains the 16-bit value from GSU R0.
; No parameters. Called from C via: extern void launchGSU(void);
;------------------------------------------------------------------------------
.SECTION ".gsu_launcher" SEMIFREE

.ACCU 16
.INDEX 16

launchGSU:
    php
    sep #$20
    .ACCU 8

    ; Give ROM + RAM buses to GSU
    lda #$18                     ; SCMR: RAN=1 + RON=1 (GSU owns both)
    sta.l $303A                  ; SCMR

    ; Set program bank (linker resolves :gsu_program to the ROM bank)
    lda #:gsu_program
    sta.l $3034                  ; PBR

    ; Start GSU: write R15 (high byte write triggers execution)
    rep #$20
    .ACCU 16
    lda #gsu_program             ; 16-bit ROM offset (LoROM: $8000+)
    sta.l $301E                  ; R15 → GSU starts!

    ; Poll SFR until GO flag clears (GSU finished)
    sep #$20
    .ACCU 8
-   lda.l $3030                  ; SFR low byte
    and #$20                     ; GO flag (bit 5)
    bne -

    ; Reclaim buses for SNES CPU
    lda #$00
    sta.l $303A                  ; SCMR: SNES CPU owns everything

    ; Read GSU R0 result and store for C code
    rep #$20
    .ACCU 16
    lda.l $3000                  ; GSU R0
    sta.l gsu_result

    plp
    rtl

.ENDS

.endif
