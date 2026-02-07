;==============================================================================
; OpenSNES C Runtime Library
;==============================================================================
; Provides runtime functions for the cc65816 compiler (cproc+QBE).
; These are called by generated code for operations the 65816 can't do natively.
;
; Author: OpenSNES Team
; License: MIT
;==============================================================================

; Export runtime function names for the linker
; (Labels starting with _ are local in WLA-DX, so we create global aliases)
.DEFINE __mul16 tcc_mul16
.DEFINE __div16 tcc_div16
.DEFINE __mod16 tcc_mod16
.EXPORT __mul16
.EXPORT __div16
.EXPORT __mod16

.SECTION ".runtime" SEMIFREE

;------------------------------------------------------------------------------
; tcc_mul16 - Signed 16-bit multiplication
;------------------------------------------------------------------------------
; Input:  tcc__r0 = multiplicand
;         tcc__r1 = multiplier
; Output: tcc__r0 = low 16 bits of result
;
; Uses hardware multiplication registers:
;   $4202 = WRMPYA (multiplicand, 8-bit)
;   $4203 = WRMPYB (multiplier, 8-bit - triggers multiply)
;   $4216/$4217 = RDMPYL/H (result, 16-bit)
;
; For 16x16 bit multiplication, we use the identity:
;   (a_hi*256 + a_lo) * (b_hi*256 + b_lo) =
;   a_lo*b_lo + (a_hi*b_lo + a_lo*b_hi)*256 + a_hi*b_hi*65536
;
; Since we only need the low 16 bits:
;   result_lo = a_lo*b_lo + (a_hi*b_lo + a_lo*b_hi)*256 (low byte only of this)
;------------------------------------------------------------------------------
tcc_mul16:
    php
    rep #$30            ; 16-bit A, X/Y

    ; Compiler calling convention: args pushed on stack
    ; Stack layout after jsl + php:
    ;   SP+1: P register (1 byte)
    ;   SP+2,3,4: return address (3 bytes)
    ;   SP+5,6: arg1 = multiplicand (pushed second)
    ;   SP+7,8: arg2 = multiplier (pushed first)

    sep #$20            ; 8-bit A

    ; --- a_lo * b_lo ---
    lda 5,s             ; a_lo (multiplicand low byte)
    sta $4202           ; WRMPYA
    lda 7,s             ; b_lo (multiplier low byte)
    sta $4203           ; WRMPYB - triggers multiply

    nop                 ; Wait 8 cycles for result
    nop
    nop
    nop

    rep #$20
    lda $4216           ; Result low 16 bits
    sta tcc__r2         ; Save partial result

    sep #$20

    ; --- a_hi * b_lo ---
    lda 6,s             ; a_hi (multiplicand high byte)
    sta $4202           ; WRMPYA
    lda 7,s             ; b_lo (multiplier low byte)
    sta $4203           ; WRMPYB

    nop
    nop
    nop
    nop

    ; Add to result (shifted left 8 = high byte goes to result)
    rep #$20
    lda $4216
    xba                 ; Swap bytes (multiply by 256)
    and #$FF00          ; Keep only high byte contribution
    clc
    adc tcc__r2
    sta tcc__r2

    sep #$20

    ; --- a_lo * b_hi ---
    lda 5,s             ; a_lo (multiplicand low byte)
    sta $4202           ; WRMPYA
    lda 8,s             ; b_hi (multiplier high byte)
    sta $4203           ; WRMPYB

    nop
    nop
    nop
    nop

    rep #$20
    lda $4216
    xba                 ; Multiply by 256
    and #$FF00
    clc
    adc tcc__r2         ; Final result in A (returned to caller)

    plp
    rtl

;------------------------------------------------------------------------------
; tcc_div16 - Unsigned 16-bit division
;------------------------------------------------------------------------------
; Input:  tcc__r0 = dividend
;         tcc__r1 = divisor (must be <= 255 for hardware, else software)
; Output: tcc__r0 = quotient
;         tcc__r1 = remainder
;
; Uses hardware division ($4204-$4206, result at $4214-$4217)
;------------------------------------------------------------------------------
tcc_div16:
    php
    rep #$30

    ; Check if divisor fits in 8 bits
    lda tcc__r1
    and #$FF00
    bne @software_div

    ; Hardware division (8-bit divisor only)
    sep #$20
    lda tcc__r0
    sta $4204           ; WRDIVL
    lda tcc__r0+1
    sta $4205           ; WRDIVH
    lda tcc__r1
    sta $4206           ; WRDIVB - triggers division

    ; Wait 16 cycles
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    rep #$20
    lda $4214           ; Quotient
    sta tcc__r0
    lda $4216           ; Remainder
    sta tcc__r1

    plp
    rtl

@software_div:
    ; Software division for 16-bit divisor
    ; Simple shift-and-subtract algorithm
    lda #0
    sta tcc__r2         ; Quotient
    sta tcc__r3         ; Remainder

    ldx #16             ; 16 bits

@div_loop:
    ; Shift dividend left into remainder
    asl tcc__r0
    rol tcc__r3

    ; Shift quotient left
    asl tcc__r2

    ; Try to subtract divisor from remainder
    lda tcc__r3
    sec
    sbc tcc__r1
    bcc @no_sub

    ; Subtraction succeeded
    sta tcc__r3
    inc tcc__r2         ; Set quotient bit

@no_sub:
    dex
    bne @div_loop

    ; Move results
    lda tcc__r2
    sta tcc__r0         ; Quotient
    lda tcc__r3
    sta tcc__r1         ; Remainder

    plp
    rtl

;------------------------------------------------------------------------------
; tcc_mod16 - Unsigned 16-bit modulo
;------------------------------------------------------------------------------
; Input:  tcc__r0 = dividend
;         tcc__r1 = divisor
; Output: tcc__r0 = remainder
;------------------------------------------------------------------------------
tcc_mod16:
    jsl tcc_div16
    lda tcc__r1         ; Remainder is in tcc__r1
    sta tcc__r0
    rtl

.ENDS
