;==============================================================================
; luna stress probe — open-bus / MDR reads via ABSOLUTE-LONG addressing.
;==============================================================================
; The C compiler cannot emit a non-bank-$00 pointer read (structural bank-$00
; limitation), so it can't set the open-bus MDR to a chosen bank byte. This
; hand-written asm reads the $2100 MMIO mirror through several banks with
; explicit `lda.l bb:2100`, so the last operand byte fetched (the bank `bb`)
; is the MDR. On hardware the open-bus read then returns `bb`.
;
;   res[0] = lda.l $3F:2100  -> expect $3F
;   res[1] = lda.l $01:2100  -> expect $01
;   res[2] = lda.l $20:2133  -> expect $20   (SETINI, write-only)
;   res[3] = lda.l $10:21FF  -> expect $10   (unmapped $21xx)
;   res[4] = lda.l $00:2100  -> expect $00   (control)
;   res[5] = lda.l $3F:2134  -> real MPYL (readable, not open bus) = $00 at boot
;
; Called from C as `void ob_probe(void)`.
;==============================================================================

; (memmap.inc + assets.inc are injected by the build's wrap_asm — do not
;  include them here, or .MEMORYMAP is defined twice.)

.SECTION ".text.ob_probe" SUPERFREE

ob_probe:
    phb                 ; save caller DBR
    php
    sep #$20            ; 8-bit accumulator
    .ACCU 8
    lda #$00            ; DBR = $00 so `sta.w res` targets bank-$00 WRAM
    pha
    plb

    lda.l $3F2100
    sta.w res+0
    lda.l $012100
    sta.w res+1
    lda.l $202133
    sta.w res+2
    lda.l $1021FF
    sta.w res+3
    lda.l $002100
    sta.w res+4
    lda.l $3F2134
    sta.w res+5

    lda #$DE            ; marker
    sta.w res+12
    lda #$C0
    sta.w res+13

    plp
    plb                 ; restore caller DBR
    rtl

.ENDS
