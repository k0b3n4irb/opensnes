;==============================================================================
; SA-1 Coprocessor Boot Stub
;==============================================================================
;
; Executed by the SA-1 CPU when released from reset.
; ALL memory writes use STA.L (long absolute) to avoid DB dependency.
; The stack-based PHA/PLB to set DB=$00 fails because I-RAM write
; protection may not be cleared in time.
;
;==============================================================================

.ifdef SA1

.SECTION ".sa1_boot" SUPERFREE

.ACCU 16
.INDEX 16

SA1Start:
    ; Switch to native mode (register-only, no memory access)
    sei
    clc
    xce

    ; Set up registers (register-only operations)
    rep #$30
    lda #$37FF
    tcs                     ; SP = $37FF (I-RAM top)
    lda #$3000
    tcd                     ; D = $3000 (I-RAM base)

    ; ALL writes use STA.L to bypass DB (which may be wrong)
    sep #$20
    .ACCU 8

    ; Enable SA-1 I-RAM writes (bit=1 means WRITABLE, not protected!)
    lda #$FF
    sta.l $00222A           ; CIWP = $FF (SA-1 can write all I-RAM blocks)

    ; Write magic value to I-RAM — use long absolute (bank $00)
    lda #$A5
    sta.l $003000           ; I-RAM byte 0 = $A5 (ready signal)

    ; Clear message register — use long absolute
    lda #$00
    sta.l $002209           ; SCNT = $00

    ; Enter idle loop (WAI doesn't need DB)
_sa1_idle:
    wai
    jmp _sa1_idle

.ENDS

.endif ; SA1
