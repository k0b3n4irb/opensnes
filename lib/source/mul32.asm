;==============================================================================
; OpenSNES C Runtime Library — 32-bit multiply (low 32 bits of product)
;==============================================================================
; Provides `__mul32` for the cc65816 backend's Omul Kl handler.
;
; SEPARATE compilation unit from runtime.asm so that ROMs which do NOT
; perform 32-bit multiplies pay zero ROM/RAM cost. The QBE Omul Kl path
; emits `jsl __mul32` + `lda.l mul32_hi` — once user code triggers that
; path (Session 7+ in the A1-followup chantier, when cproc starts emitting
; Kl IR for `long`), this object becomes part of the link via an explicit
; LINK_OBJS entry in make/common.mk.
;
; Until then this file builds to `mul32-asm.o` but is NOT linked, so it
; cannot change any shipping ROM's bytes or the bank-0 RAM layout.
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

; Pinned to bank 7 (highest LoROM/HiROM bank, empty in every example as of
; the A1-followup chantier) so the ~200-byte helper doesn't displace const
; strings out of bank 0 in examples that were already brushing the limit.
.EXPORT tcc_mul32
.SECTION ".mul32" BANK 7 FREE

.ACCU 16
.INDEX 16

;------------------------------------------------------------------------------
; tcc_mul32 - 32-bit multiplication (low 32 bits of product)
;
; CRITICAL: this section exports `tcc_mul32` directly — the qbe backend
; emits `jsl tcc_mul32` so the call target carries the full 24-bit address
; (bank 7 + offset $8000). The previous setup used
;   .DEFINE __mul32 tcc_mul32 ; .EXPORT __mul32
; which captured only the 16-bit offset at .DEFINE time (sections aren't
; placed until link). `__mul32` resolved to $00:8000 — colliding with
; `tcc_mul16` which also exports $8000 from BANK 0. Every Kl mul call
; from C jumped to mul16 instead and computed the wrong 16-bit product.
; Was tetris's root cause: every `board[r][c]` indexed `board[0][c]`
; because `r * 10` always returned 0.
;------------------------------------------------------------------------------
; For two's-complement multiply, the low N bits of a*b are identical whether
; a,b are interpreted as signed or unsigned, so no sign handling is needed
; for the truncated 32-bit result.
;
; Input (stack, after PHP + JSL):
;   5,6,s   = a_lo (16-bit, low half of first arg, pushed last)
;   7,8,s   = a_hi (16-bit, high half of first arg)
;   9,10,s  = b_lo (16-bit, low half of second arg)
;   11,12,s = b_hi (16-bit, high half of second arg)
;
; Output:
;   A        = result low 16 bits
;   mul32_hi = result high 16 bits (caller reads this via `lda.l mul32_hi`)
;
; Algorithm: 10 hardware 8x8 multiplies:
;   - 4 partial products of a_lo * b_lo (full 32-bit)
;   - 3 for low-16 of a_hi * b_lo (a_hi.hi * b_lo.hi past bit 31, skipped)
;   - 3 for low-16 of a_lo * b_hi
;------------------------------------------------------------------------------
tcc_mul32:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    ;------------------------------------------------------------------
    ; STEP 1: a_lo * b_lo  -> (mul32_lo, mul32_hi) as full 32-bit
    ;
    ;   p0 = a_lo.lo * b_lo.lo  (bits  0..15)
    ;   p1 = a_lo.hi * b_lo.lo  (bits  8..23)
    ;   p2 = a_lo.lo * b_lo.hi  (bits  8..23)
    ;   p3 = a_lo.hi * b_lo.hi  (bits 16..31)
    ;------------------------------------------------------------------

    ; --- p0 = a_lo.lo * b_lo.lo ---
    sep #$20
    .ACCU 8
    lda 5,s
    sta.l $4202
    lda 9,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    sta.w mul32_lo
    stz.w mul32_hi
    sep #$20
    .ACCU 8

    ; --- p1 = a_lo.hi * b_lo.lo  ->  add (p1 << 8) ---
    lda 6,s
    sta.l $4202
    lda 9,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    xba
    pha
    and.w #$FF00
    clc
    adc.w mul32_lo
    sta.w mul32_lo
    pla
    and.w #$00FF
    adc.w mul32_hi
    sta.w mul32_hi
    sep #$20
    .ACCU 8

    ; --- p2 = a_lo.lo * b_lo.hi  ->  add (p2 << 8) ---
    lda 5,s
    sta.l $4202
    lda 10,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    xba
    pha
    and.w #$FF00
    clc
    adc.w mul32_lo
    sta.w mul32_lo
    pla
    and.w #$00FF
    adc.w mul32_hi
    sta.w mul32_hi
    sep #$20
    .ACCU 8

    ; --- p3 = a_lo.hi * b_lo.hi  ->  add (p3 to high; p3 << 16 = high word only) ---
    lda 6,s
    sta.l $4202
    lda 10,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    clc
    adc.w mul32_hi
    sta.w mul32_hi

    ;------------------------------------------------------------------
    ; STEP 2: low-16 of a_hi * b_lo  ->  add to mul32_hi
    ;
    ;   q0 = a_hi.lo * b_lo.lo  (full 16, adds at bit 0 of cross)
    ;   q1 = a_hi.hi * b_lo.lo  (low byte adds at bit 8)
    ;   q2 = a_hi.lo * b_lo.hi  (low byte adds at bit 8)
    ;   q3 ignored (past bit 15 — irrelevant for truncated result)
    ;------------------------------------------------------------------

    sep #$20
    .ACCU 8
    lda 7,s
    sta.l $4202
    lda 9,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    clc
    adc.w mul32_hi
    sta.w mul32_hi
    sep #$20
    .ACCU 8

    lda 8,s
    sta.l $4202
    lda 9,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    xba
    and.w #$FF00
    clc
    adc.w mul32_hi
    sta.w mul32_hi
    sep #$20
    .ACCU 8

    lda 7,s
    sta.l $4202
    lda 10,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    xba
    and.w #$FF00
    clc
    adc.w mul32_hi
    sta.w mul32_hi

    ;------------------------------------------------------------------
    ; STEP 3: low-16 of a_lo * b_hi  ->  add to mul32_hi
    ;------------------------------------------------------------------

    sep #$20
    .ACCU 8
    lda 5,s
    sta.l $4202
    lda 11,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    clc
    adc.w mul32_hi
    sta.w mul32_hi
    sep #$20
    .ACCU 8

    lda 6,s
    sta.l $4202
    lda 11,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    xba
    and.w #$FF00
    clc
    adc.w mul32_hi
    sta.w mul32_hi
    sep #$20
    .ACCU 8

    lda 5,s
    sta.l $4202
    lda 12,s
    sta.l $4203
    nop
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    xba
    and.w #$FF00
    clc
    adc.w mul32_hi
    sta.w mul32_hi

    ;------------------------------------------------------------------
    ; Return: A = mul32_lo (low 16); mul32_hi already holds high 16.
    ;------------------------------------------------------------------
    lda.w mul32_lo

    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; Scratch RAM for tcc_mul32 (4 bytes, bank-0 fixed location)
;------------------------------------------------------------------------------
.RAMSECTION ".mul32_ram" BANK 0 SLOT 1
    mul32_lo:   dsb 2
    mul32_hi:   dsb 2
.ENDS
