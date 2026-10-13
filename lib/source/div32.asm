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
; Algorithm: restoring shift-subtract, at most 16 iterations a word
; (div32_ucore, shared by the two entries):
;   * divisor under 65536 (the common case: a 32-bit quantity divided by a
;     16-bit one): the high word of the dividend is divided first, then
;     the low word with that remainder, the remainder kept in A. When the
;     high word is smaller than the divisor — the quotient fits 16 bits —
;     the first step is skipped: 16 iterations of about 30 cycles.
;   * divisor of 65536 or more: the quotient fits 16 bits, so the
;     remainder starts as the dividend's high word and 16 iterations of
;     the 32-bit trial subtraction remain.
;   * divisor 0: quotient $FFFFFFFF, remainder = the dividend, as the
;     32-iteration loop this replaces gave (C leaves it undefined).
;   * Signed: track signs at entry, abs() in place, run the unsigned core,
;     then negate q (if signs differ) and r (if dividend was negative,
;     per C99 truncation-toward-zero semantics).
;
; Until 2026-10-11 both entries ran 32 iterations of a 64-bit shift and a
; 32-bit trial subtraction whatever the operands.
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

; Pinned to bank 7 — same rationale as mul32.asm. ~270 bytes here would
; otherwise spill bank 0 string literals in the tighter examples.
;
; No `.EXPORT` directives here — that's for `.DEFINE`d names only, not labels.
; Labels in a FREE section are visible across object files automatically.
; The qbe backend emits `jsl tcc_udivmod32` / `jsl tcc_sdivmod32` directly,
; and the linker resolves them via this section's symbol table with the
; correct BANK 7 prefix.
;
; The previous setup used `.DEFINE __X tcc_X ; .EXPORT __X`, but WLA-DX's
; `.DEFINE` captures only the 16-bit offset at parse time — section bank
; placement isn't resolved until link, so `__X` would resolve to $00:8000
; and collide with whatever bank-0 helper happened to land there. This
; was the bug that broke tetris via `__mul32` colliding with `tcc_mul16`
; (see commit 1208c53).
; Any bank but $00, highest first — the same placement as the asset
; sections (templates/assets.inc). Reached by `jsl` only, so any bank works;
; it was pinned `BANK 7 FREE`, which left no room once assets moved to the
; asset banks (2026-09-23: four examples failed to link, "No room for
; section .div32 in ROM bank 7"). The literal list is deliberate: a .DEFINE
; is not expanded inside BANKS, and banks 1-7 exist in every memory map.
.SECTION ".div32" SEMISUPERFREE BANKS 7-1

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

    jsr div32_ucore

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

    ; --- Unsigned divide on absolute values ---
    jsr div32_ucore

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


;------------------------------------------------------------------------------
; div32_ucore - the unsigned division both entries share
;------------------------------------------------------------------------------
; In:  div32_ql:qh = dividend, div32_div_lo:div_hi = divisor. A and X 16-bit.
; Out: div32_ql:qh = quotient, div32_rl:rh = remainder. A, X clobbered.
;------------------------------------------------------------------------------
div32_ucore:
    lda.w div32_div_hi
    beq +
    brl @big                     ; past the unrolled steps: out of a short branch's reach
+

    ; --- divisor under 65536: the remainder fits 16 bits, kept in A ---
    stz.w div32_rh
    lda.w div32_div_lo
    bne +
    brl @zero                    ; past the unrolled steps as well
+
    lda.w div32_qh
    cmp.w div32_div_lo
    bcs @hi_init
    stz.w div32_qh               ; high word < divisor: it IS the remainder
    bra @lo_init
@hi_init:
    lda.w #0
    ldx.w #16
@hi:
    asl.w div32_qh
    rol a
    bcs @hi_sub                  ; 17 bits: above any 16-bit divisor
    cmp.w div32_div_lo
    bcc @hi_no
@hi_sub:
    sbc.w div32_div_lo           ; carry is set on both ways in
    inc.w div32_qh
@hi_no:
    dex
    bne @hi
@lo_init:
    ; The sixteen steps unrolled (2026-10-13): the loop's dex / bne cost
    ; five cycles of about thirty a step, and this path is the one a
    ; sequencer takes at every period change (a constant over the period).
    ; The quotient bit is
    ; the carry the compare leaves (set when the divisor was subtracted),
    ; rolled into the low word by the next step's `rol`: no `inc` in
    ; memory. The first `rol` enters with the carry clear, the last bit is
    ; rolled in after the steps. 16 x 12 bytes.
    clc
.REPT 16
    rol.w div32_ql               ; the previous step's bit in, the top bit out
    rol a
    bcs +                        ; 17 bits: above any 16-bit divisor
    cmp.w div32_div_lo
    bcc ++                       ; carry clear: no subtraction, bit 0
+   sbc.w div32_div_lo           ; carry set on both ways in
    sec                          ; bit 1 — the 17-bit way may have borrowed
++
.ENDR
    rol.w div32_ql               ; the sixteenth bit
    sta.w div32_rl
    rts

@zero:
    lda.w div32_ql
    sta.w div32_rl
    lda.w div32_qh
    sta.w div32_rh
    lda.w #$FFFF
    sta.w div32_ql
    sta.w div32_qh
    rts

    ; --- divisor of 65536 or more: the quotient fits 16 bits ---
@big:
    lda.w div32_qh
    sta.w div32_rl
    stz.w div32_rh
    stz.w div32_qh
    ldx.w #16
@b:
    asl.w div32_ql
    rol.w div32_rl
    rol.w div32_rh               ; no carry out: the remainder is under 2^32
    lda.w div32_rl
    cmp.w div32_div_lo
    lda.w div32_rh
    sbc.w div32_div_hi
    bcc @b_no
    sta.w div32_rh               ; A is already the high word of r - divisor
    lda.w div32_rl
    sec
    sbc.w div32_div_lo
    sta.w div32_rl
    inc.w div32_ql
@b_no:
    dex
    bne @b
    rts

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
