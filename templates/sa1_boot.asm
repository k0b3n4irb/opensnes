;==============================================================================
; SA-1 Coprocessor Boot Stub
;==============================================================================
;
; Executed by the SA-1 CPU (10.74 MHz) when released from reset.
; ALL memory writes use STA.L (long absolute) to bypass unknown DB.
;
; I-RAM layout ($3000-$37FF, shared between both CPUs):
;   $3000: Ready flag ($A5 = booted)
;   $3001: Command byte (0=idle, 1=count)
;   $3002-$3005: 32-bit counter (SA-1 increments continuously)
;
;==============================================================================

.ifdef SA1

.SECTION ".sa1_boot" SUPERFREE

.ACCU 16
.INDEX 16

SA1Start:
    ; Switch to native mode
    sei
    clc
    xce

    ; Set up registers
    rep #$30
    lda #$37FF
    tcs                     ; SP = $37FF
    lda #$3000
    tcd                     ; D = $3000

    ; Enable SA-1 I-RAM writes (bit=1 means WRITABLE)
    sep #$20
    .ACCU 8
    lda #$FF
    sta.l $00222A           ; CIWP = $FF

    ; Signal ready
    lda #$A5
    sta.l $003000           ; Ready flag

    ; Clear counter (32-bit at I-RAM $3002-$3005)
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $003002           ; Counter low word
    sta.l $003004           ; Counter high word

    ; Set command to "count" mode
    sep #$20
    .ACCU 8
    lda #$01
    sta.l $003001           ; Command = 1 (count)

    ;------------------------------------------------------------------
    ; Main SA-1 loop: increment 32-bit counter at 10.74 MHz
    ; This runs ~3× faster than the main CPU could do it
    ;------------------------------------------------------------------
    rep #$20
    .ACCU 16

_sa1_count_loop:
    ; Increment 32-bit counter at I-RAM $3002
    lda.l $003002           ; Load low word
    inc a                   ; Increment
    sta.l $003002           ; Store low word
    bne _sa1_count_loop     ; No carry? Loop

    ; Low word wrapped to 0 — increment high word
    lda.l $003004
    inc a
    sta.l $003004

    jmp _sa1_count_loop

.ENDS

.endif ; SA1
