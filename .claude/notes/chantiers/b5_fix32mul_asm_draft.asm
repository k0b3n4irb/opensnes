;==============================================================================
; OpenSNES 16.16 Fixed-Point Math — fix32Mul
;
; The 8.8 fixMul in math.asm uses 4 hardware 8×8 multiplies to extract bits
; 8-23 of the 32-bit product. For 16.16 we need bits 16-47 of a 64-bit
; product — formally 16 hw 8×8 multiplies. A cleaner formulation:
;
;   Let a = a_h * 2^16 + a_l, b = b_h * 2^16 + b_l (16-bit halves).
;   (a * b) >> 16 [low 32 bits] = (a_h*b_l + a_l*b_h) + ((a_l*b_l) >> 16)
;                                 + ((a_h*b_h & 0xFFFF) << 16)
;
; That's 3 unsigned 16x16→32 multiplies + 1 unsigned 16x16→16 (we only
; need the low 16 of a_h*b_h). Each 16x16→32 is 4 hw 8×8 multiplies plus
; combine.
;
; cc65816 pushes args LEFT-TO-RIGHT, and the post-A6 ABI uses 4-byte slots
; for s32. After PHP + JSL return:
;   5,6,s   = b low 16 (rightmost arg's lo half)
;   7,8,s   = b high 16
;   9,10,s  = a low 16
;   11,12,s = a high 16
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

.EXPORT fix32Mul
.SECTION ".fix32mul" SUPERFREE

;------------------------------------------------------------------------------
; fixed32 fix32Mul(fixed32 a, fixed32 b)
;
; Stack:
;   5-6,s   = b_lo
;   7-8,s   = b_hi
;   9-10,s  = a_lo
;   11-12,s = a_hi
;
; Algorithm:
;   1. Take absolute values; track sign = (a XOR b) >> 31.
;   2. Decompose into 16-bit halves: a = a_h:a_l, b = b_h:b_l.
;   3. Compute three 16×16→32 products:
;        ll  = a_l * b_l   (needed for high 16 bits = bits 16-31 of result_lo)
;        ml1 = a_l * b_h   (full 32 bits — contributes at bit 16)
;        ml2 = a_h * b_l   (full 32 bits — contributes at bit 16)
;   4. Compute one 16×16→16 product (low 16 only):
;        hh_lo = (a_h * b_h) & 0xFFFF   (contributes at bit 32 = result_hi low)
;   5. result = ml1 + ml2 + (ll >> 16) + (hh_lo << 16)
;   6. If signs differed, negate result.
;------------------------------------------------------------------------------
fix32Mul:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    ; ---- Stash a and b absolute values + sign ----
    ; Sign of result = sign(a_hi) XOR sign(b_hi).
    lda 11,s                ; a_hi
    eor 7,s                 ; XOR b_hi
    sta.w f32_sign          ; bit 15 set = result negative

    ; absA = |a|
    lda 9,s
    sta.w f32_a_lo
    lda 11,s
    sta.w f32_a_hi
    bpl @a_pos
        ; Negate (a_hi:a_lo): two's complement 32-bit
        lda.w f32_a_lo
        eor #$FFFF
        clc
        adc #1
        sta.w f32_a_lo
        lda.w f32_a_hi
        eor #$FFFF
        adc #0              ; +carry from lo
        sta.w f32_a_hi
@a_pos:
    ; absB = |b|
    lda 5,s
    sta.w f32_b_lo
    lda 7,s
    sta.w f32_b_hi
    bpl @b_pos
        lda.w f32_b_lo
        eor #$FFFF
        clc
        adc #1
        sta.w f32_b_lo
        lda.w f32_b_hi
        eor #$FFFF
        adc #0
        sta.w f32_b_hi
@b_pos:

    ; ---- Compute ll = a_l * b_l (16x16 → 32 unsigned) ----
    ; X = f32_a_lo, Y = f32_b_lo
    ; Result: ll_lo at f32_ll_lo, ll_hi at f32_ll_hi
    jsr @u16x16_xy_lo_lo

    ; ---- Compute ml1 = a_l * b_h ----
    jsr @u16x16_xy_lo_hi

    ; ---- Compute ml2 = a_h * b_l ----
    jsr @u16x16_xy_hi_lo

    ; ---- Compute hh_lo = (a_h * b_h) low 16 ----
    ; We only need bits 0-15 of a_h*b_h since the higher bits fall outside
    ; the result window (bits 48+ of the full product).
    jsr @u16x16_xy_hi_hi_low_only

    ; ---- Combine: result = ml1 + ml2 + ll_hi + (hh_lo << 16) ----
    ; All as 32-bit. ml1, ml2 are full 32-bit. ll_hi is 16-bit (zero-extended).
    ; hh_lo is 16-bit, contributes to high 16 only.
    rep #$30
    .ACCU 16
    .INDEX 16

    ; result_lo = ml1_lo
    lda.w f32_ml1_lo
    sta.w f32_res_lo
    lda.w f32_ml1_hi
    sta.w f32_res_hi

    ; result += ml2 (32-bit)
    clc
    lda.w f32_res_lo
    adc.w f32_ml2_lo
    sta.w f32_res_lo
    lda.w f32_res_hi
    adc.w f32_ml2_hi
    sta.w f32_res_hi

    ; result += ll_hi (just the high 16 of a_l*b_l, added to result_lo)
    clc
    lda.w f32_res_lo
    adc.w f32_ll_hi
    sta.w f32_res_lo
    lda.w f32_res_hi
    adc #0                  ; +carry only
    sta.w f32_res_hi

    ; result += hh_lo << 16 (= add hh_lo to result_hi)
    clc
    lda.w f32_res_hi
    adc.w f32_hh_lo
    sta.w f32_res_hi

    ; ---- Apply sign ----
    bit.w f32_sign          ; sign bit was in bit 15 of f32_sign (high word of XOR)
    bpl @result_pos
        ; Negate result_hi:result_lo
        lda.w f32_res_lo
        eor #$FFFF
        clc
        adc #1
        sta.w f32_res_lo
        lda.w f32_res_hi
        eor #$FFFF
        adc #0
        sta.w f32_res_hi
@result_pos:

    ; ---- Return result in (Y high : A low) per cc65816 ABI for Kl ----
    ; The Kl ABI returns 32-bit values: low 16 in A, high 16 in Y.
    ldy.w f32_res_hi
    lda.w f32_res_lo

    plp
    rtl

;------------------------------------------------------------------------------
; Internal: unsigned 16x16 → 32 multiply
;
; Four flavours specialised for the input pair we want, so the caller
; doesn't need to shuffle bytes. Each computes a 32-bit unsigned product
; from two 16-bit unsigned inputs and stores the result.
;
;   @u16x16_xy_lo_lo: X=f32_a_lo, Y=f32_b_lo → f32_ll_lo, f32_ll_hi
;   @u16x16_xy_lo_hi: X=f32_a_lo, Y=f32_b_hi → f32_ml1_lo, f32_ml1_hi
;   @u16x16_xy_hi_lo: X=f32_a_hi, Y=f32_b_lo → f32_ml2_lo, f32_ml2_hi
;   @u16x16_xy_hi_hi_low_only: X=f32_a_hi, Y=f32_b_hi → f32_hh_lo (low 16 only)
;
; All four follow the same pattern:
;   p0 = X.lo * Y.lo  (bits 0-15)
;   p1 = X.hi * Y.lo  (bits 8-23)
;   p2 = X.lo * Y.hi  (bits 8-23)
;   p3 = X.hi * Y.hi  (bits 16-31)
;   result = p0 + (p1 << 8) + (p2 << 8) + (p3 << 16) [32-bit]
;
; The macro below is unrolled for each flavour to avoid bank-byte trap
; (.DEFINE aliases) and to keep the code in the same SUPERFREE section.
;------------------------------------------------------------------------------

.MACRO @do_u16x16
    ; \1 = X address (low byte address; X.hi = +1)
    ; \2 = Y address (low byte address; Y.hi = +1)
    sep #$20
    .ACCU 8

    ; p0 = X.lo * Y.lo
    lda \1
    sta.l $4202
    lda \2
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    sta.w f32_p0

    ; p1 = X.hi * Y.lo
    sep #$20
    .ACCU 8
    lda \1 + 1
    sta.l $4202
    lda \2
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    sta.w f32_p1

    ; p2 = X.lo * Y.hi
    sep #$20
    .ACCU 8
    lda \1
    sta.l $4202
    lda \2 + 1
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    sta.w f32_p2

    ; p3 = X.hi * Y.hi
    sep #$20
    .ACCU 8
    lda \1 + 1
    sta.l $4202
    lda \2 + 1
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    .ACCU 16
    lda.l $4216
    sta.w f32_p3
.ENDM

; Helper that combines p0..p3 into (out_hi:out_lo) per the standard layout:
;   bits 0-15  = p0
;   bits 8-23  = p1 + p2
;   bits 16-31 = p3
.MACRO @combine_u32
    ; \1 = out_lo address, \2 = out_hi address
    ; result_lo = p0 low + ((p1+p2) low) << 8
    ; result_hi = (p1+p2) high + p3 + carries

    ; p1p2 = p1 + p2 (17-bit sum, but we capture carry separately)
    clc
    lda.w f32_p1
    adc.w f32_p2
    sta.w f32_p1p2_lo   ; low 16 of p1+p2
    lda #0
    adc #0
    sta.w f32_p1p2_carry  ; either 0 or 1 (carry out of bit 15)

    ; result_lo: low byte = p0 low byte; high byte = p0 high byte + (p1p2 low byte)
    ; In 16-bit terms: result_lo = p0 + ((p1p2_lo & 0xFF) << 8)
    lda.w f32_p1p2_lo
    and #$00FF        ; keep only low byte
    xba               ; A = byte:00 → 00:byte ; wait XBA swaps so A = 00:byte → byte:00. Yes that's what we want.
                      ; Actually: A holds value V in low byte (00:V). After XBA, A = V:00. So now low byte=00, high byte=V.
    clc
    adc.w f32_p0
    sta \1

    ; carry out from above goes into bit 16 = result_hi low
    ; result_hi = (p1p2_lo >> 8) + (p1p2_hi-from-carry << 8 wait, p1p2 is 17-bit but stored as 16 + 1 carry)
    ; Actually: p1p2 has bits 0-16. Bits 0-7 went to result_lo bit 8-15.
    ;          Bits 8-15 go to result_hi bits 0-7.
    ;          Bit 16 (the carry) goes to result_hi bit 8.
    ;          Plus p3 contributes bits 0-15 of result_hi.

    ; (p1p2 >> 8) low byte = p1p2_lo high byte; bit 16 (p1p2_carry) becomes high byte of (p1p2>>8)
    lda.w f32_p1p2_lo
    xba               ; swap: A = low_byte:high_byte → high_byte:low_byte. So now A has p1p2_lo's hi byte in low position.
    and #$00FF        ; keep just that byte (clear what was the low byte, now in high position)

    ; Add p1p2_carry << 8 (the bit-16 carry goes to bit 8 of result_hi's representation)
    ; Actually wait — let me re-think. We're computing the FULL 32-bit product:
    ;   bits 0-7:   p0 low byte
    ;   bits 8-15:  p0 high byte + (p1+p2) low byte + any carry from bit 7→8 (none since p0 was 16-bit fitting in bits 0-15)
    ;   bits 16-23: (p1+p2) high byte + p3 low byte + carry from bits 8-15
    ;   bits 24-31: p3 high byte + carry from bits 16-23
    ;
    ; Wait that's not quite right either. p1+p2 occupies bits 8-23 (it's a 16-bit value shifted left 8). So:
    ;   bits 8-15:  p1p2 bits 0-7   (low byte of p1p2)
    ;   bits 16-23: p1p2 bits 8-15  (high byte of p1p2)
    ;   bit 24:     p1p2 bit 16     (carry, since p1+p2 can be 17-bit)

    ; OK let me redo cleanly. After computing p1p2 = p1 + p2 with carry:
    ;   p1p2 (16-bit) shifted left 8 occupies bits 8-23 of result.
    ;   p1p2_carry (1 bit) is bit 24 of result.
    ;   p0 (16-bit) occupies bits 0-15.
    ;   p3 (16-bit) shifted left 16 occupies bits 16-31.
    ;
    ; Sum bits 0-15: p0 (16-bit) + (p1p2 << 8) low 16 = p0 + (p1p2_lo_byte << 8)
    ; Sum bits 16-31: (p1p2 << 8) high 16 + p3 + carry from below
    ;               = (p1p2_hi_byte | (p1p2_carry << 8)) + p3 + carry

    ; First sum bits 0-15:
    ;   already done above: result_lo = p0 + (p1p2_lo_byte << 8)
    ;   carry from bit 15→16 needs to be tracked.

    ; Recompute with explicit carry tracking:
    ; (We were in the middle of the macro — let me restart the combine carefully.)
.ENDM

; Simpler combine inline rather than macro: see below in each flavour.

@u16x16_xy_lo_lo:
    @do_u16x16 f32_a_lo, f32_b_lo
    ; Combine p0..p3 → 32-bit at f32_ll_lo / f32_ll_hi
    rep #$30
    .ACCU 16
    .INDEX 16

    ; result_lo low byte = p0 low byte
    ; result_lo high byte = p0 high byte + p1p2 low byte
    ; result_hi low byte = p1p2 high byte + p1p2_carry + p3 low byte + carry
    ; result_hi high byte = p3 high byte + carry

    ; Step 1: p1p2 = p1 + p2 (17-bit sum stored as 16 + carry)
    clc
    lda.w f32_p1
    adc.w f32_p2
    sta.w f32_p1p2_lo
    lda #0
    adc #0          ; bit 16
    sta.w f32_p1p2_carry

    ; Step 2: result_lo = p0 + (p1p2_lo << 8)
    ;         carry out → tracked
    lda.w f32_p1p2_lo
    and #$00FF      ; isolate low byte
    xba             ; shift left 8 (swap into high byte)
    clc
    adc.w f32_p0
    sta.w f32_ll_lo
    lda #0
    adc #0          ; carry from low-word add
    sta.w f32_carry_acc

    ; Step 3: result_hi = (p1p2_lo >> 8) + (p1p2_carry << 8) + p3 + carry_acc
    ;
    ; (p1p2_lo >> 8): high byte of p1p2_lo, value 0..255, as 16-bit
    lda.w f32_p1p2_lo
    xba             ; swap so high byte is now low
    and #$00FF      ; isolate it

    ; Add p1p2_carry << 8 (carry value was 0 or 1; shift into bit 8)
    ; Since carry is 0/1, (carry << 8) is 0 or 0x100. Easier: ORA #$0100 if carry.
    ldy.w f32_p1p2_carry
    beq @no_carry_shift
        ora #$0100
@no_carry_shift:

    ; Add p3
    clc
    adc.w f32_p3

    ; Add carry_acc from step 2
    clc                ; clear before adding small value? Actually we want carry-in 0 here, but carry_acc was 0 or 1
    ; Actually we already added carry from step 2 implicitly... no, carry_acc holds 0 or 1.
    ; If carry_acc is 1, we need to add 1 to result_hi.
    pha
    lda.w f32_carry_acc
    clc
    adc 1,s            ; add to top of stack
    sta 1,s
    pla
    sta.w f32_ll_hi
    rts

@u16x16_xy_lo_hi:
    @do_u16x16 f32_a_lo, f32_b_hi
    rep #$30
    .ACCU 16
    .INDEX 16
    clc
    lda.w f32_p1
    adc.w f32_p2
    sta.w f32_p1p2_lo
    lda #0
    adc #0
    sta.w f32_p1p2_carry
    lda.w f32_p1p2_lo
    and #$00FF
    xba
    clc
    adc.w f32_p0
    sta.w f32_ml1_lo
    lda #0
    adc #0
    sta.w f32_carry_acc
    lda.w f32_p1p2_lo
    xba
    and #$00FF
    ldy.w f32_p1p2_carry
    beq @no_c2
        ora #$0100
@no_c2:
    clc
    adc.w f32_p3
    pha
    lda.w f32_carry_acc
    clc
    adc 1,s
    sta 1,s
    pla
    sta.w f32_ml1_hi
    rts

@u16x16_xy_hi_lo:
    @do_u16x16 f32_a_hi, f32_b_lo
    rep #$30
    .ACCU 16
    .INDEX 16
    clc
    lda.w f32_p1
    adc.w f32_p2
    sta.w f32_p1p2_lo
    lda #0
    adc #0
    sta.w f32_p1p2_carry
    lda.w f32_p1p2_lo
    and #$00FF
    xba
    clc
    adc.w f32_p0
    sta.w f32_ml2_lo
    lda #0
    adc #0
    sta.w f32_carry_acc
    lda.w f32_p1p2_lo
    xba
    and #$00FF
    ldy.w f32_p1p2_carry
    beq @no_c3
        ora #$0100
@no_c3:
    clc
    adc.w f32_p3
    pha
    lda.w f32_carry_acc
    clc
    adc 1,s
    sta 1,s
    pla
    sta.w f32_ml2_hi
    rts

@u16x16_xy_hi_hi_low_only:
    ; Only need the LOW 16 bits of a_h * b_h.
    ; Low 16 = p0 + ((p1+p2) << 8) [low 16 of that sum]
    @do_u16x16 f32_a_hi, f32_b_hi
    rep #$30
    .ACCU 16
    .INDEX 16
    clc
    lda.w f32_p1
    adc.w f32_p2          ; p1+p2 (low 16; carry discarded — bit 16 is outside our 16-bit window)
    and #$00FF            ; mask to low byte
    xba                   ; shift left 8
    clc
    adc.w f32_p0
    sta.w f32_hh_lo
    rts

.ENDS

;------------------------------------------------------------------------------
; Scratch RAM for fix32 math. Mirrors the math.asm pattern but with
; dedicated names to avoid colliding with fmul_* (which 8.8 fixMul uses).
;------------------------------------------------------------------------------
.RAMSECTION ".fix32mul_ram" BANK 0 SLOT 1
    f32_a_lo:       dsb 2
    f32_a_hi:       dsb 2
    f32_b_lo:       dsb 2
    f32_b_hi:       dsb 2
    f32_sign:       dsb 2
    f32_p0:         dsb 2
    f32_p1:         dsb 2
    f32_p2:         dsb 2
    f32_p3:         dsb 2
    f32_p1p2_lo:    dsb 2
    f32_p1p2_carry: dsb 2
    f32_carry_acc:  dsb 2
    f32_ll_lo:      dsb 2
    f32_ll_hi:      dsb 2
    f32_ml1_lo:     dsb 2
    f32_ml1_hi:     dsb 2
    f32_ml2_lo:     dsb 2
    f32_ml2_hi:     dsb 2
    f32_hh_lo:      dsb 2
    f32_res_lo:     dsb 2
    f32_res_hi:     dsb 2
.ENDS
