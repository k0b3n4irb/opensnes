;==============================================================================
; SA-1 Boot — Default minimal stub
;==============================================================================
; Boots the SA-1 coprocessor, writes the ready magic byte to I-RAM $3000,
; then enters an idle loop. Override this file by placing your own
; sa1_boot.asm in the example directory.
;
; I-RAM layout:
;   $3000: Ready flag ($A5 = booted)
;   $3001+: Available for application use
;==============================================================================

.ifdef SA1

.SECTION ".sa1_boot" BANK 0 SLOT 0 SEMIFREE   ; the SA-1 reset vector is 16-bit: the program must sit in ROM bank 0 (2026-10-03; SUPERFREE let a big program drift to bank 1 and the SA-1 never booted)

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
    sta.l $003000

    ; Idle loop (override sa1_boot.asm for application-specific code)
-   wai
    bra -

.ENDS

.endif
