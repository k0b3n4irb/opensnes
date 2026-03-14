;==============================================================================
; OpenSNES Fixed-Point Math — Hardware Multiplier Implementations
;
; Uses the SNES hardware 8×8 unsigned multiplier ($4202/$4203/$4216-$4217)
; for fast, correct fixed-point arithmetic.
;
; The compiler's __mul16 only produces a 16-bit result, which overflows
; for 8.8 fixed-point operations. These assembly functions compute the
; full 32-bit product and extract the correct 16-bit result.
;
; cc65816 pushes args LEFT-TO-RIGHT:
;   fixMul(a, b) → push a, push b → b at lower offset, a at higher
;   After PHP (1 byte) + JSL return (3 bytes):
;     5-6,s = b (rightmost arg, pushed first)
;     7-8,s = a (leftmost arg, pushed last)
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

.SECTION ".math_fixmul" SUPERFREE

;------------------------------------------------------------------------------
; fixed fixMul(fixed a, fixed b)
;
; 8.8 signed fixed-point multiply using SNES hardware multiplier.
;
; Algorithm:
;   1. Make both operands positive, track sign
;   2. Four 8×8 unsigned partial products:
;        P0 = a_lo * b_lo   (bits 0-15)
;        P1 = a_hi * b_lo   (bits 8-23)
;        P2 = a_lo * b_hi   (bits 8-23)
;        P3 = a_hi * b_hi   (bits 16-31)
;   3. Combine: result = (P0>>8) + P1 + P2 + (P3<<8)
;   4. Negate if signs differ
;
; Stack: 5-6,s = b, 7-8,s = a (after PHP + JSL return)
;------------------------------------------------------------------------------
fixMul:
    php
    rep #$30                    ; 16-bit A, X, Y

    ; Load arguments
    lda 7,s                     ; a
    sta.w fmul_a
    lda 5,s                     ; b
    sta.w fmul_b

    ; Determine result sign: XOR sign bits → bit 15 = result negative
    eor.w fmul_a
    sta.w fmul_sign

    ; Make 'a' positive
    lda.w fmul_a
    bpl @a_pos
        eor #$FFFF
        inc a
        sta.w fmul_a
@a_pos:
    ; Make 'b' positive
    lda.w fmul_b
    bpl @b_pos
        eor #$FFFF
        inc a
        sta.w fmul_b
@b_pos:

    sep #$20                    ; 8-bit A for hardware multiplier

    ;-- P0 = a_lo * b_lo --
    lda.w fmul_a               ; a low byte
    sta.l $4202                 ; WRMPYA
    lda.w fmul_b               ; b low byte
    sta.l $4203                 ; WRMPYB (8-cycle delay starts)
    nop                         ; \
    nop                         ;  | fill 8 cycles
    nop                         ; /
    rep #$20
    lda.l $4216                 ; P0 result
    sta.w fmul_p0

    ;-- P1 = a_hi * b_lo --
    sep #$20
    lda.w fmul_a + 1           ; a high byte
    sta.l $4202
    lda.w fmul_b               ; b low byte
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    lda.l $4216                 ; P1 result
    sta.w fmul_p1

    ;-- P2 = a_lo * b_hi --
    sep #$20
    lda.w fmul_a               ; a low byte
    sta.l $4202
    lda.w fmul_b + 1           ; b high byte
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    lda.l $4216                 ; P2 result
    sta.w fmul_p2

    ;-- P3 = a_hi * b_hi --
    sep #$20
    lda.w fmul_a + 1           ; a high byte
    sta.l $4202
    lda.w fmul_b + 1           ; b high byte
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    lda.l $4216                 ; P3 result
    sta.w fmul_p3

    ;-- Combine: result = (P0 >> 8) + P1 + P2 + (P3 << 8) --
    ;
    ; This gives us bits 8-23 of the full 32-bit product,
    ; which is the 8.8 fixed-point result.

    ; Start with P0 >> 8 (high byte of P0, zero-extended)
    lda.w fmul_p0 + 1          ; load from high byte address
    and #$00FF                  ; zero-extend to 16-bit

    ; Add P1
    clc
    adc.w fmul_p1

    ; Add P2
    adc.w fmul_p2

    ; Add P3 << 8 (low byte of P3 shifted to high byte position)
    pha                         ; save partial result on stack
    lda.w fmul_p3
    xba                         ; swap bytes: low→high, high→low
    and #$FF00                  ; mask off the garbage low byte
    clc
    adc 1,s                     ; add to partial result on stack
    sta 1,s                     ; store combined result
    pla                         ; A = final unsigned 8.8 result

    ; Apply sign: negate if input signs differed
    bit.w fmul_sign            ; test bit 15 (sign flag in N)
    bpl @result_pos
        eor #$FFFF
        inc a
@result_pos:

    plp
    rtl

;------------------------------------------------------------------------------
; fixed fixLerp(fixed a, fixed b, u8 t)
;
; Linear interpolation: a + (b - a) * t / 256
; where t is 0-255 (0.0 to ~1.0).
;
; The multiply (b-a)*t can overflow 16 bits, so we use the hardware
; multiplier for a correct 16×8 → 24-bit result, then >>8.
;
; Stack after PHP + JSL:
;   5-6,s = t (u8 in 16-bit slot, rightmost)
;   7-8,s = b (leftmost+1)
;   9-10,s = a (leftmost)
;------------------------------------------------------------------------------
fixLerp:
    php
    rep #$30

    ; diff = b - a
    lda 7,s                     ; b
    sec
    sbc 9,s                     ; a
    sta.w fmul_a               ; diff (signed 16-bit)

    ; Track sign of diff
    sta.w fmul_sign
    bpl @diff_pos
        eor #$FFFF
        inc a
        sta.w fmul_a           ; |diff|
@diff_pos:

    ; t is in 5,s (8-bit value)
    sep #$20

    ;-- P0 = |diff_lo| * t --
    lda.w fmul_a               ; diff low byte
    sta.l $4202
    lda 5,s                     ; t
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    lda.l $4216
    sta.w fmul_p0

    ;-- P1 = |diff_hi| * t --
    sep #$20
    lda.w fmul_a + 1           ; diff high byte
    sta.l $4202
    lda 5,s                     ; t
    sta.l $4203
    nop
    nop
    nop
    rep #$20
    lda.l $4216
    sta.w fmul_p1

    ; result = (P0 >> 8) + P1 = bits 8-23 of |diff| * t
    lda.w fmul_p0 + 1
    and #$00FF
    clc
    adc.w fmul_p1

    ; Apply sign of diff
    bit.w fmul_sign
    bpl @lerp_pos
        eor #$FFFF
        inc a
@lerp_pos:

    ; result = a + scaled_diff
    clc
    adc 9,s                     ; + a
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; Scratch RAM for fixed-point math
;------------------------------------------------------------------------------
.RAMSECTION ".math_fixmul_ram" BANK 0 SLOT 1
    fmul_a:     dsb 2
    fmul_b:     dsb 2
    fmul_sign:  dsb 2
    fmul_p0:    dsb 2
    fmul_p1:    dsb 2
    fmul_p2:    dsb 2
    fmul_p3:    dsb 2
.ENDS
