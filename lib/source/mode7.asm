;==============================================================================
; Mode 7 Library Functions
; OpenSNES - MIT License
;
; Based on PVSnesLib Mode 7 implementation by Alekmaul (zlib license)
;==============================================================================

.INCLUDE "lib_memmap.inc"

;------------------------------------------------------------------------------
; RAM variables for Mode 7
;------------------------------------------------------------------------------
.RAMSECTION ".mode7vars" BANK 0 SLOT 1
    m7_scale_x:     dsw 1       ; Scale X ($0100 = 1.0)
    m7_scale_y:     dsw 1       ; Scale Y ($0100 = 1.0)
    m7_sin:         dsb 1       ; Current sin value
    m7_cos:         dsb 1       ; Current cos value
    m7_mat_a:       dsw 1       ; Matrix A
    m7_mat_b:       dsw 1       ; Matrix B
    m7_mat_c:       dsw 1       ; Matrix C
    m7_mat_d:       dsw 1       ; Matrix D
.ENDS

.SECTION ".mode7_data" SUPERFREE

;------------------------------------------------------------------------------
; Sin/Cos lookup table (256 entries, signed 8-bit)
; Index 0 = sin(0), Index 64 = sin(90) = cos(0), etc.
;------------------------------------------------------------------------------
m7_sincos_table:
    .db $00,$03,$06,$09,$0c,$0f,$12,$15,$18,$1b,$1e,$21,$24,$27,$2a,$2d
    .db $30,$33,$36,$39,$3b,$3e,$41,$43,$46,$49,$4b,$4e,$50,$52,$55,$57
    .db $59,$5b,$5e,$60,$62,$64,$66,$67,$69,$6b,$6c,$6e,$70,$71,$72,$74
    .db $75,$76,$77,$78,$79,$7a,$7b,$7b,$7c,$7d,$7d,$7e,$7e,$7e,$7e,$7e
    .db $7f,$7e,$7e,$7e,$7e,$7e,$7d,$7d,$7c,$7b,$7b,$7a,$79,$78,$77,$76
    .db $75,$74,$72,$71,$70,$6e,$6c,$6b,$69,$67,$66,$64,$62,$60,$5e,$5b
    .db $59,$57,$55,$52,$50,$4e,$4b,$49,$46,$43,$41,$3e,$3b,$39,$36,$33
    .db $30,$2d,$2a,$27,$24,$21,$1e,$1b,$18,$15,$12,$0f,$0c,$09,$06,$03
    .db $00,$fd,$fa,$f7,$f4,$f1,$ee,$eb,$e8,$e5,$e2,$df,$dc,$d9,$d6,$d3
    .db $d0,$cd,$ca,$c7,$c5,$c2,$bf,$bd,$ba,$b7,$b5,$b2,$b0,$ae,$ab,$a9
    .db $a7,$a5,$a2,$a0,$9e,$9c,$9a,$99,$97,$95,$94,$92,$90,$8f,$8e,$8c
    .db $8b,$8a,$89,$88,$87,$86,$85,$85,$84,$83,$83,$82,$82,$82,$82,$82
    .db $81,$82,$82,$82,$82,$82,$83,$83,$84,$85,$85,$86,$87,$88,$89,$8a
    .db $8b,$8c,$8e,$8f,$90,$92,$94,$95,$97,$99,$9a,$9c,$9e,$a0,$a2,$a5
    .db $a7,$a9,$ab,$ae,$b0,$b2,$b5,$b7,$ba,$bd,$bf,$c2,$c5,$c7,$ca,$cd
    .db $d0,$d3,$d6,$d9,$dc,$df,$e2,$e5,$e8,$eb,$ee,$f1,$f4,$f7,$fa,$fd

.ENDS

.SECTION ".mode7_code" SUPERFREE

;------------------------------------------------------------------------------
; mode7Init - Initialize Mode 7 variables and registers
;------------------------------------------------------------------------------
mode7Init:
    php
    rep #$20

    ; Default scale = 1.0
    lda #$0100
    sta.l m7_scale_x
    sta.l m7_scale_y

    ; Initialize matrix to identity (no rotation, 1x scale)
    sep #$20
    lda #$00
    sta.l $211B         ; M7A low
    lda #$01
    sta.l $211B         ; M7A high = $0100
    lda #$00
    sta.l $211C         ; M7B = 0
    sta.l $211C
    sta.l $211D         ; M7C = 0
    sta.l $211D
    sta.l $211E         ; M7D low
    lda #$01
    sta.l $211E         ; M7D high = $0100

    ; Set center point (screen center = 128, 128)
    lda #128
    sta.l $211F         ; M7X low
    lda #0
    sta.l $211F         ; M7X high
    lda #128
    sta.l $2120         ; M7Y low
    lda #0
    sta.l $2120         ; M7Y high

    ; Set scroll to center on Mode 7 plane
    lda #0
    sta.l $210D         ; M7HOFS low
    sta.l $210D         ; M7HOFS high
    lda #$80            ; 0x180 & 0xFF
    sta.l $210E         ; M7VOFS low
    lda #$01            ; 0x180 >> 8
    sta.l $210E         ; M7VOFS high

    plp
    rtl

;------------------------------------------------------------------------------
; mode7SetScale - Set scale values
; Input: 5,s = scale_x, 7,s = scale_y
;------------------------------------------------------------------------------
mode7SetScale:
    php
    rep #$20

    lda 5,s
    sta.l m7_scale_x
    lda 7,s
    sta.l m7_scale_y

    plp
    rtl

;------------------------------------------------------------------------------
; mode7SetAngle - Set rotation angle and compute matrix
; Input: 6,s = angle (8-bit, after phb)
;------------------------------------------------------------------------------
mode7SetAngle:
    php
    phb

    rep #$10
    sep #$20            ; 8-bit A, 16-bit X/Y

    ; Set data bank to sin table bank
    lda #:m7_sincos_table
    pha
    plb

    ; Clear high byte of A
    lda #0
    xba

    ; Get angle parameter (5,s + 1 for phb = 6,s)
    lda 6,s
    rep #$20
    tax                 ; X = angle for indexing

    ; Get sin value
    sep #$20
    lda m7_sincos_table.w,x
    sta.l m7_sin

    ; Get cos value (angle + 64)
    lda #0
    xba
    txa
    clc
    adc #64
    rep #$20
    tax
    sep #$20
    lda m7_sincos_table.w,x
    sta.l m7_cos

    plb                 ; Restore data bank

    ;--------------------------------------------------
    ; Calculate matrix using hardware multiplier
    ; M7A register doubles as multiplication input
    ;--------------------------------------------------

    ; M7B = -sin * scale_x
    rep #$20
    lda.l m7_scale_x
    sep #$20
    sta.l $211B         ; M7A low (multiplicand low)
    xba
    sta.l $211B         ; M7A high (multiplicand high)
    lda.l m7_sin
    eor #$FF
    inc a               ; Negate sin
    sta.l $211C         ; M7B (multiplier) - triggers multiply
    rep #$20
    lda.l $2135         ; Read MPYM+MPYH (high 16 bits of result)
    sta.l m7_mat_b

    ; M7C = sin * scale_y
    lda.l m7_scale_y
    sep #$20
    sta.l $211B
    xba
    sta.l $211B
    lda.l m7_sin
    sta.l $211C
    rep #$20
    lda.l $2135
    sta.l m7_mat_c

    ; M7A = cos * scale_x
    lda.l m7_scale_x
    sep #$20
    sta.l $211B
    xba
    sta.l $211B
    lda.l m7_cos
    sta.l $211C
    rep #$20
    lda.l $2135
    sta.l m7_mat_a

    ; M7D = cos * scale_y
    lda.l m7_scale_y
    sep #$20
    sta.l $211B
    xba
    sta.l $211B
    lda.l m7_cos
    sta.l $211C
    rep #$20
    lda.l $2135
    sta.l m7_mat_d

    ;--------------------------------------------------
    ; Write matrix to PPU
    ;--------------------------------------------------
    sep #$20

    lda.l m7_mat_a
    sta.l $211B
    lda.l m7_mat_a+1
    sta.l $211B

    lda.l m7_mat_b
    sta.l $211C
    lda.l m7_mat_b+1
    sta.l $211C

    lda.l m7_mat_c
    sta.l $211D
    lda.l m7_mat_c+1
    sta.l $211D

    lda.l m7_mat_d
    sta.l $211E
    lda.l m7_mat_d+1
    sta.l $211E

    plp
    rtl

;------------------------------------------------------------------------------
; mode7SetCenter - Set center of rotation
; Input: 5,s = x, 7,s = y
;------------------------------------------------------------------------------
mode7SetCenter:
    php
    sep #$20

    lda 5,s             ; X low
    sta.l $211F
    lda 6,s             ; X high
    sta.l $211F

    lda 7,s             ; Y low
    sta.l $2120
    lda 8,s             ; Y high
    sta.l $2120

    plp
    rtl

;------------------------------------------------------------------------------
; mode7SetScroll - Set scroll position
; Input: 5,s = x, 7,s = y
;------------------------------------------------------------------------------
mode7SetScroll:
    php
    sep #$20

    lda 5,s             ; X low
    sta.l $210D
    lda 6,s             ; X high
    sta.l $210D

    lda 7,s             ; Y low
    sta.l $210E
    lda 8,s             ; Y high
    sta.l $210E

    plp
    rtl

;------------------------------------------------------------------------------
; mode7Rotate - Set rotation in degrees (0-359)
; Input: 5,s = degrees (16-bit)
; Converts degrees to 0-255 range: angle = degrees * 256 / 360
;------------------------------------------------------------------------------
mode7Rotate:
    php
    rep #$20

    ; degrees * 256 / 360 ≈ degrees * 91 / 128
    ; We use the hardware multiplier for this
    lda 5,s             ; Get degrees
    sta.l $4202         ; WRMPYA (8-bit, but we'll handle 16-bit separately)

    ; For simplicity, we do: (degrees * 182) >> 8
    ; 182 / 256 ≈ 0.711 ≈ 256/360
    sep #$20
    lda 5,s             ; degrees low byte
    sta.l $4202         ; WRMPYA
    lda #182            ; Conversion factor
    sta.l $4203         ; WRMPYB - triggers multiply

    ; Wait 8 cycles for result
    nop
    nop
    nop
    nop

    lda.l $4217         ; RDMPYH (high byte of result)
    pha                 ; Save converted angle

    ; Call mode7SetAngle with the converted angle
    jsl mode7SetAngle

    pla                 ; Clean up stack

    plp
    rtl

;------------------------------------------------------------------------------
; mode7Transform - Set rotation and scale together
; Input: 5,s = degrees, 7,s = scalePercent (100 = 1.0)
;------------------------------------------------------------------------------
mode7Transform:
    php
    rep #$20

    ; Convert percentage to 8.8 fixed point
    ; scalePercent * 256 / 100 = scalePercent * 2.56 ≈ scalePercent * 656 / 256
    ; Simplified: (scalePercent * 256 + 50) / 100
    ; Even simpler approximation: scalePercent * 2 + scalePercent/2 + scalePercent/16

    ; For now, use: scale = (percent << 8) / 100
    ; We'll use a simpler approximation: scale = percent * 2 + percent/2

    lda 7,s             ; scalePercent
    asl a               ; * 2
    sta.l m7_scale_x
    lda 7,s
    lsr a               ; / 2
    clc
    adc.l m7_scale_x    ; * 2.5 (close enough to * 2.56)
    sta.l m7_scale_x
    sta.l m7_scale_y

    plp

    ; Now call mode7Rotate with the degrees parameter
    ; We need to call it with the same stack layout
    jmp mode7Rotate     ; Tail call (degrees is still at 5,s)

;------------------------------------------------------------------------------
; mode7SetPivot - Set pivot point (screen coordinates)
; Input: 5,s = x (8-bit), 6,s = y (8-bit)
;------------------------------------------------------------------------------
mode7SetPivot:
    php
    sep #$20

    ; Set center point
    lda 5,s             ; X
    sta.l $211F         ; M7X low
    lda #0
    sta.l $211F         ; M7X high

    lda 6,s             ; Y
    sta.l $2120         ; M7Y low
    lda #0
    sta.l $2120         ; M7Y high

    plp
    rtl

;------------------------------------------------------------------------------
; mode7SetMatrix - Set matrix values directly
; Input: 5,s = a, 7,s = b, 9,s = c, 11,s = d
;------------------------------------------------------------------------------
mode7SetMatrix:
    php
    sep #$20

    ; Write M7A
    lda 5,s
    sta.l $211B
    lda 6,s
    sta.l $211B

    ; Write M7B
    lda 7,s
    sta.l $211C
    lda 8,s
    sta.l $211C

    ; Write M7C
    lda 9,s
    sta.l $211D
    lda 10,s
    sta.l $211D

    ; Write M7D
    lda 11,s
    sta.l $211E
    lda 12,s
    sta.l $211E

    plp
    rtl

;------------------------------------------------------------------------------
; mode7SetSettings - Set M7SEL register
; Input: 5,s = settings (8-bit)
;------------------------------------------------------------------------------
mode7SetSettings:
    php
    sep #$20

    lda 5,s
    sta.l $211A         ; M7SEL

    plp
    rtl

.ENDS
