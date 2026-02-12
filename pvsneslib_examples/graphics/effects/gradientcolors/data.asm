;==============================================================================
; GradientColors - Graphics Data
; Ported from PVSnesLib to OpenSNES
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

.section ".rodata1" superfree

tiles:
.incbin "res/pvsneslib.pic"
tiles_end:

.ends

.section ".rodata2" superfree

tilemap:
.incbin "res/pvsneslib.map"
tilemap_end:

palette:
.incbin "res/pvsneslib.pal"
palette_end:

;------------------------------------------------------------------------------
; HDMA Gradient Table (from PVSnesLib original)
;
; Each 5-byte entry: [lines] [$00] [$00] [color_lo] [color_hi]
; The two $00 bytes set CGADD=0 (backdrop), then the color is written.
; The gradient creates a green-teal wave across the screen.
;------------------------------------------------------------------------------
hdmaGradientList:
    .db $02,$00,$00,$40,$01
    .db $1F,$00,$00,$A2,$15
    .db $08,$00,$00,$24,$2A
    .db $08,$00,$00,$A6,$3E
    .db $08,$00,$00,$09,$57
    .db $08,$00,$00,$8B,$6B
    .db $08,$00,$00,$ED,$7F
    .db $08,$00,$00,$8F,$6B
    .db $08,$00,$00,$12,$57
    .db $08,$00,$00,$94,$42
    .db $08,$00,$00,$17,$2A
    .db $08,$00,$00,$B9,$15
    .db $08,$00,$00,$3B,$01
    .db $20,$00,$00,$B9,$15
    .db $08,$00,$00,$17,$2A
    .db $08,$00,$00,$94,$42
    .db $08,$00,$00,$12,$57
    .db $08,$00,$00,$8F,$6B
    .db $08,$00,$00,$ED,$7F
    .db $08,$00,$00,$8B,$6B
    .db $08,$00,$00,$09,$57
    .db $08,$00,$00,$A6,$3E
    .db $07,$00,$00,$A6,$3E
    .db $00                     ; End of table

.ends
