;==============================================================================
; SA-1 Boot — 32-bit counter at 10.74 MHz
;==============================================================================
; Increments a 32-bit counter in I-RAM continuously.
; The main CPU reads the counter each frame to demonstrate SA-1 speed.
;
; I-RAM layout:
;   $3000: Ready flag ($A5 = booted)
;   $3001: Command byte (unused, reserved)
;   $3002-$3003: Counter low word
;   $3004-$3005: Counter high word
;==============================================================================

.ifdef SA1

.SECTION ".sa1_boot" SUPERFREE

.ACCU 16
.INDEX 16

SA1Start:
    sei
    clc
    xce
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #$37FF
    tcs                         ; stack top of I-RAM
    lda #$3000
    tcd                         ; direct page in I-RAM
    sep #$20
    .ACCU 8

    ; Enable SA-1 I-RAM writes (bit=1 = WRITABLE)
    lda #$FF
    sta.l $00222A               ; CIWP = $FF

    ; Signal ready
    lda #$A5
    sta.l $003000               ; Ready flag

    ; Clear counter (32-bit at I-RAM $3002-$3005)
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $003002               ; Counter low word
    sta.l $003004               ; Counter high word

    ; Set command to "count" mode
    sep #$20
    .ACCU 8
    lda #$01
    sta.l $003001               ; Command = 1 (count)

    ;------------------------------------------------------------------
    ; Main SA-1 loop: increment 32-bit counter at 10.74 MHz
    ;------------------------------------------------------------------
    rep #$20
    .ACCU 16

_sa1_count_loop:
    lda.l $003002               ; Load low word
    inc a                       ; Increment
    sta.l $003002               ; Store low word
    bne _sa1_count_loop         ; No carry? Loop

    ; Low word wrapped to 0 — increment high word
    lda.l $003004
    inc a
    sta.l $003004

    jmp _sa1_count_loop

.ENDS

.endif
