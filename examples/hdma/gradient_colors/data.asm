;==============================================================================
; GradientColors - Graphics Data
; Began as the PVSnesLib example; the gradient is ours
;
; HDMA gradient table for mode 3 (HDMA_MODE_2REG_2X) writing to CGADD/CGDATA.
; Each entry: [count] [cgaddr] [cgaddr] [color_lo] [color_hi]
;
; Mode 3 writes 4 bytes: dest, dest, dest+1, dest+1
;   dest = $2121 (CGADD): sets CGRAM address to 0 (backdrop color)
;   dest+1 = $2122 (CGDATA): writes 15-bit SNES color (low + high)
;
; SNES color format: 0bbbbbgg gggrrrrr (15-bit)
;==============================================================================

ASSET_SECTION ".rodata2"

;------------------------------------------------------------------------------
; HDMA Gradient Table: the city's sky at dusk
;
; Each 5-byte entry: [lines] [$00] [$00] [color_lo] [color_hi]
; The two $00 bytes set CGADD=0 (backdrop), then the color is written.
; 28 bands of 8 lines (224): the night at the top, then indigo, violet and
; magenta, to the amber glow of the streets at the bottom.
;------------------------------------------------------------------------------
hdmaGradientList:
    .db $08,$00,$00,$62,$1C      ; lines   0-  7: r 2 g 3 b 7
    .db $08,$00,$00,$63,$20      ; lines   8- 15: r 3 g 3 b 8
    .db $08,$00,$00,$63,$24      ; lines  16- 23: r 3 g 3 b 9
    .db $08,$00,$00,$64,$24      ; lines  24- 31: r 4 g 3 b 9
    .db $08,$00,$00,$64,$28      ; lines  32- 39: r 4 g 3 b10
    .db $08,$00,$00,$85,$2C      ; lines  40- 47: r 5 g 4 b11
    .db $08,$00,$00,$86,$30      ; lines  48- 55: r 6 g 4 b12
    .db $08,$00,$00,$86,$34      ; lines  56- 63: r 6 g 4 b13
    .db $08,$00,$00,$87,$34      ; lines  64- 71: r 7 g 4 b13
    .db $08,$00,$00,$87,$38      ; lines  72- 79: r 7 g 4 b14
    .db $08,$00,$00,$88,$3C      ; lines  80- 87: r 8 g 4 b15
    .db $08,$00,$00,$89,$3C      ; lines  88- 95: r 9 g 4 b15
    .db $08,$00,$00,$8A,$3C      ; lines  96-103: r10 g 4 b15
    .db $08,$00,$00,$AC,$3C      ; lines 104-111: r12 g 5 b15
    .db $08,$00,$00,$AD,$40      ; lines 112-119: r13 g 5 b16
    .db $08,$00,$00,$AE,$40      ; lines 120-127: r14 g 5 b16
    .db $08,$00,$00,$D0,$40      ; lines 128-135: r16 g 6 b16
    .db $08,$00,$00,$D1,$40      ; lines 136-143: r17 g 6 b16
    .db $08,$00,$00,$D2,$40      ; lines 144-151: r18 g 6 b16
    .db $08,$00,$00,$F3,$3C      ; lines 152-159: r19 g 7 b15
    .db $08,$00,$00,$14,$39      ; lines 160-167: r20 g 8 b14
    .db $08,$00,$00,$35,$35      ; lines 168-175: r21 g 9 b13
    .db $08,$00,$00,$56,$31      ; lines 176-183: r22 g10 b12
    .db $08,$00,$00,$58,$29      ; lines 184-191: r24 g10 b10
    .db $08,$00,$00,$79,$25      ; lines 192-199: r25 g11 b 9
    .db $08,$00,$00,$9A,$21      ; lines 200-207: r26 g12 b 8
    .db $08,$00,$00,$BB,$1D      ; lines 208-215: r27 g13 b 7
    .db $08,$00,$00,$DC,$19      ; lines 216-223: r28 g14 b 6
    .db $00                     ; End of table

.ends
