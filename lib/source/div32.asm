;==============================================================================
; OpenSNES C Runtime Library — 32-bit divide / modulo
;==============================================================================
; Provides `__udivmod32` (unsigned) and `__sdivmod32` (signed) for the
; cc65816 backend's Odiv / Oudiv / Orem / Ourem Kl handlers.
;
; SEPARATE compilation unit — like mul32.asm, this object is built but
; NOT in the default LINK_OBJS. Until cproc emits Kl IR for `long`
; (Session 7+ of the A1-followup chantier), no example references
; __[s]divmod32 and the helpers do not contribute to any shipping ROM.
;
; Algorithm: restoring shift-subtract bit-by-bit, 32 iterations.
;   * The quotient is built up in the same slot as the dividend
;     (classic "dividend doubles as quotient" trick — each shift moves
;     a dividend bit into the remainder MSB and leaves room at bit 0
;     for the new quotient bit).
;   * Signed: track signs at entry, abs() in place, run unsigned core,
;     then negate q (if signs differ) and r (if dividend was negative,
;     per C99 truncation-toward-zero semantics).
;
; Cycle budget: ~1500 cycles per __udivmod32 call (32 iters × ~45 cyc),
; ~1700 for __sdivmod32 (adds ~200 for sign handling).
;
; Author: OpenSNES Team
; License: MIT
;==============================================================================

.ifdef SA1
.include "memmap_sa1.inc"
.else
.ifdef HIROM
.include "memmap_hirom.inc"
.else
.include "memmap.inc"
.endif
.endif

.DEFINE __udivmod32 tcc_udivmod32
.DEFINE __sdivmod32 tcc_sdivmod32
.EXPORT __udivmod32
.EXPORT __sdivmod32

; Pinned to bank 7 — same rationale as mul32.asm. ~270 bytes here would
; otherwise spill bank 0 string literals in the tighter examples.
.SECTION ".div32" BANK 7 FREE

.ACCU 16
.INDEX 16

;------------------------------------------------------------------------------
; tcc_udivmod32 - Unsigned 32-bit divide+modulo
;------------------------------------------------------------------------------
; Input (stack, after PHP + JSL):
;   5,6,s   = dividend low 16  (a_lo, pushed last)
;   7,8,s   = dividend high 16 (a_hi)
;   9,10,s  = divisor low 16   (b_lo)
;   11,12,s = divisor high 16  (b_hi)
;
; Output:
;   A         = quotient low 16 (also stored in div32_ql)
;   div32_ql  = quotient low 16
;   div32_qh  = quotient high 16
;   div32_rl  = remainder low 16
;   div32_rh  = remainder high 16
;
; Caller is expected to `lda.l div32_qh` (or rl/rh) after the cleanup to
; recover the high half of the result it cares about.
;
; Divisor 0: behavior is undefined per C semantics. The loop produces
; quotient = $FFFFFFFF and a meaningless remainder; we don't trap.
;------------------------------------------------------------------------------
tcc_udivmod32:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    ; Copy dividend → ql:qh (loop uses ql:qh as the shifting workspace;
    ; after 32 iterations they hold the quotient).
    lda 5,s
    sta.w div32_ql
    lda 7,s
    sta.w div32_qh

    ; Copy divisor → div_lo:div_hi (read-only inside the loop)
    lda 9,s
    sta.w div32_div_lo
    lda 11,s
    sta.w div32_div_hi

    ; Initialize remainder to 0
    stz.w div32_rl
    stz.w div32_rh

    ldx.w #32
@uloop:
    ; 64-bit left shift: rh:rl:qh:ql <<= 1
    asl.w div32_ql
    rol.w div32_qh
    rol.w div32_rl
    rol.w div32_rh

    ; Trial subtract: r - divisor; check final carry (no borrow = r >= divisor)
    lda.w div32_rl
    cmp.w div32_div_lo
    lda.w div32_rh
    sbc.w div32_div_hi
    bcc @uno_sub                 ; borrow → r < divisor, skip

    ; r >= divisor: subtract for real and set quotient bit 0
    lda.w div32_rl
    sec
    sbc.w div32_div_lo
    sta.w div32_rl
    lda.w div32_rh
    sbc.w div32_div_hi
    sta.w div32_rh
    inc.w div32_ql               ; bit 0 was 0 from the shift; set it now
@uno_sub:
    dex
    bne @uloop

    lda.w div32_ql               ; return A = quotient low
    plp
    rtl

;------------------------------------------------------------------------------
; tcc_sdivmod32 - Signed 32-bit divide+modulo
;------------------------------------------------------------------------------
; Same stack ABI and result slots as tcc_udivmod32. Sign rules:
;   * Quotient sign = dividend_sign XOR divisor_sign
;   * Remainder sign = dividend_sign  (C99 truncation toward zero)
;
; Most-negative-input edge: abs($80000000) computed as 2's-complement
; negation yields $80000000 again, which is a valid u32 magnitude.
; The unsigned core handles this naturally.
;------------------------------------------------------------------------------
tcc_sdivmod32:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    ; --- Read dividend; record sign in div32_dvd_sign; store |dividend| → ql:qh
    lda 5,s
    sta.w div32_ql
    lda 7,s
    sta.w div32_qh
    bpl @sdvd_pos
        ; Negate ql:qh (two's complement on 32 bits)
        lda.w div32_ql
        eor #$FFFF
        clc
        adc #1
        sta.w div32_ql
        lda.w div32_qh
        eor #$FFFF
        adc #0
        sta.w div32_qh
        lda #$FFFF
        sta.w div32_dvd_sign
        bra @sdvd_done
@sdvd_pos:
        stz.w div32_dvd_sign
@sdvd_done:

    ; --- Read divisor; record |divisor| → div_lo:div_hi; compute quot_sign
    lda 9,s
    sta.w div32_div_lo
    lda 11,s
    sta.w div32_div_hi
    bpl @sdsr_pos
        lda.w div32_div_lo
        eor #$FFFF
        clc
        adc #1
        sta.w div32_div_lo
        lda.w div32_div_hi
        eor #$FFFF
        adc #0
        sta.w div32_div_hi
        ; signs differ → negative quotient
        lda.w div32_dvd_sign
        eor #$FFFF
        sta.w div32_quot_sign
        bra @sdsr_done
@sdsr_pos:
        ; divisor positive → quotient sign = dividend sign
        lda.w div32_dvd_sign
        sta.w div32_quot_sign
@sdsr_done:

    ; --- Unsigned divide loop on absolute values ---
    stz.w div32_rl
    stz.w div32_rh
    ldx.w #32
@sloop:
    asl.w div32_ql
    rol.w div32_qh
    rol.w div32_rl
    rol.w div32_rh

    lda.w div32_rl
    cmp.w div32_div_lo
    lda.w div32_rh
    sbc.w div32_div_hi
    bcc @sno_sub

    lda.w div32_rl
    sec
    sbc.w div32_div_lo
    sta.w div32_rl
    lda.w div32_rh
    sbc.w div32_div_hi
    sta.w div32_rh
    inc.w div32_ql
@sno_sub:
    dex
    bne @sloop

    ; --- Apply signs ---
    lda.w div32_quot_sign
    bpl @squot_ok
        ; Negate quotient
        lda.w div32_ql
        eor #$FFFF
        clc
        adc #1
        sta.w div32_ql
        lda.w div32_qh
        eor #$FFFF
        adc #0
        sta.w div32_qh
@squot_ok:

    lda.w div32_dvd_sign         ; remainder sign follows dividend
    bpl @srem_ok
        lda.w div32_rl
        eor #$FFFF
        clc
        adc #1
        sta.w div32_rl
        lda.w div32_rh
        eor #$FFFF
        adc #0
        sta.w div32_rh
@srem_ok:

    lda.w div32_ql               ; return A = quotient low
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; Scratch RAM for the 32-bit divmod helpers (14 bytes, bank-0)
;------------------------------------------------------------------------------
.RAMSECTION ".div32_ram" BANK 0 SLOT 1
    div32_ql:           dsb 2    ; quotient low (and dividend workspace)
    div32_qh:           dsb 2    ; quotient high
    div32_rl:           dsb 2    ; remainder low
    div32_rh:           dsb 2    ; remainder high
    div32_div_lo:       dsb 2    ; |divisor| low
    div32_div_hi:       dsb 2    ; |divisor| high
    div32_dvd_sign:     dsb 2    ; $FFFF if dividend was negative, $0000 else
    div32_quot_sign:    dsb 2    ; $FFFF if quotient should be negative
.ENDS
