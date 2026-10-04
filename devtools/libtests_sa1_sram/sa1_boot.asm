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

; libtest_sa1_sram: the SDK boot stub plus one BW-RAM write (see below).
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

    ; Enable SA-1 BW-RAM writes too: CBWE ($2227) bit 7, 1 = write enabled
    ; (fullsnes; Nintendo manual 4.1.23). crt0 sets the SNES side (SBWE) and
    ; the protection only holds while BOTH are clear (ares bwram.cpp, from
    ; Kirby's Dream Land 3), so this is the form the manual asks for, not a
    ; fix: SA-1 writes to BW-RAM already went through.
    lda #$80
    sta.l $002227               ; CBWE = write enabled

    ; Fixture only: the SA-1 writes BW-RAM and the SNES side reads the byte
    ; back with sramLoadOffset(., 1, $100) (r_sa1_bw). No negative control
    ; is possible here: the protection holds only when SBWE and CBWE are
    ; both clear, and crt0 sets SBWE (ares bwram.cpp; luna agrees).
    lda #$5A
    sta.l $400100

    ; Signal ready
    lda #$A5
    sta.l $003000

    ; Idle loop (override sa1_boot.asm for application-specific code)
-   wai
    bra -

.ENDS

.endif
