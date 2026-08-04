;==============================================================================
; DSP-1 coprocessor driver (NEC µPD77C25, stock Sony firmware).
;
; LoROM board mapping: DR = $30:8000 (data port), SR = $30:C000 (status).
; Handshake: SR bit 7 = RQM; poll until set before every byte access.
; Words are transferred LSB-first (verified empirically on luna, 2026-08-02).
; Multi-word results are written to dsp1_o0/o1/o2 (see dsp1.h).
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

.RAMSECTION ".dsp1_state" BANK 0 SLOT 1
    dsp1_o0 dsb 2
    dsp1_o1 dsb 2
    dsp1_o2 dsb 2
.ENDS

.SECTION ".dsp1_asm" SUPERFREE

;------------------------------------------------------------------------------
; internal: spin until RQM (SR bit 7) = 1. Assumes 8-bit A. Clobbers A.
;------------------------------------------------------------------------------
dsp1_rqm:
-   lda.l $30C000
    bpl -
    rts

;------------------------------------------------------------------------------
; void dsp1Init(void)
;   Resynchronise the DSP-1 to a known command-wait state. Writes the $80
;   Sync/Reset byte repeatedly: $80 flushes any pending command (in_count=0,
;   waiting4command), so hammering it more times than the longest command's
;   byte sequence guarantees one lands as a command byte even if a prior
;   command was interrupted mid-parameter. 128 matches the stock-game boot
;   handshake. Call once before the first DSP-1 command.
;------------------------------------------------------------------------------
dsp1Init:
    php
    sep #$30
    .ACCU 8
    .INDEX 8
    ldx #128
dsp1Init_loop:
    jsr dsp1_rqm            ; poll RQM (8-bit A; leaves X untouched)
    lda #$80               ; Sync/Reset command
    sta.l $308000
    dex
    bne dsp1Init_loop
    plp
    rtl

;------------------------------------------------------------------------------
; u16 dsp1Multiply(u16 a, u16 b)  ->  A   (command $00, 1.15 product)
;------------------------------------------------------------------------------
dsp1Multiply:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$00                ; command $00 = Multiply
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; a
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; a
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; b
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; b
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; result lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; result hi
    sta.l dsp1_o0+1
    plp
    rep #$20
    .ACCU 16
    lda.l dsp1_o0           ; return value in A
    rtl

;------------------------------------------------------------------------------
; void dsp1Triangle(u16 angle, s16 radius)   (command $04)
;   dsp1_o0 = radius*sin(angle),  dsp1_o1 = radius*cos(angle)
;------------------------------------------------------------------------------
dsp1Triangle:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$04                ; command $04 = Triangle (sin/cos)
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; angle
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; angle
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; radius
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; radius
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; sin lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; sin hi
    sta.l dsp1_o0+1
    jsr dsp1_rqm
    lda.l $308000           ; cos lo
    sta.l dsp1_o1
    jsr dsp1_rqm
    lda.l $308000           ; cos hi
    sta.l dsp1_o1+1
    plp
    rtl

;------------------------------------------------------------------------------
; void dsp1Rotate(u16 angle, s16 x, s16 y)   (command $0C, 2D rotate)
;   dsp1_o0 = x',  dsp1_o1 = y'
;------------------------------------------------------------------------------
dsp1Rotate:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$0C                ; command $0C = Rotate
    sta.l $308000
    jsr dsp1_rqm
    lda 9,s                 ; angle
    sta.l $308000
    jsr dsp1_rqm
    lda 10,s                ; angle
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; x
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; x
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; y
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; y
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; x' lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; x' hi
    sta.l dsp1_o0+1
    jsr dsp1_rqm
    lda.l $308000           ; y' lo
    sta.l dsp1_o1
    jsr dsp1_rqm
    lda.l $308000           ; y' hi
    sta.l dsp1_o1+1
    plp
    rtl

;------------------------------------------------------------------------------
; void dsp1Attitude(u16 scale, u16 az, u16 ay, u16 ax)   (command $01)
;   builds a rotation matrix into slot A; no output.
;------------------------------------------------------------------------------
dsp1Attitude:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$01                ; command $01 = Attitude A
    sta.l $308000
    jsr dsp1_rqm
    lda 11,s                ; scale
    sta.l $308000
    jsr dsp1_rqm
    lda 12,s                ; scale
    sta.l $308000
    jsr dsp1_rqm
    lda 9,s                 ; az
    sta.l $308000
    jsr dsp1_rqm
    lda 10,s                ; az
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; ay
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; ay
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; ax
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; ax
    sta.l $308000
    plp
    rtl

;------------------------------------------------------------------------------
; void dsp1Objective(s16 x, s16 y, s16 z)   (command $0D, local->world via A)
;   dsp1_o0 = x', dsp1_o1 = y', dsp1_o2 = z'
;------------------------------------------------------------------------------
dsp1Objective:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$0D                ; command $0D = Objective A
    sta.l $308000
    jsr dsp1_rqm
    lda 9,s                 ; x
    sta.l $308000
    jsr dsp1_rqm
    lda 10,s                ; x
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; y
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; y
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; z
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; z
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; x' lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; x' hi
    sta.l dsp1_o0+1
    jsr dsp1_rqm
    lda.l $308000           ; y' lo
    sta.l dsp1_o1
    jsr dsp1_rqm
    lda.l $308000           ; y' hi
    sta.l dsp1_o1+1
    jsr dsp1_rqm
    lda.l $308000           ; z' lo
    sta.l dsp1_o2
    jsr dsp1_rqm
    lda.l $308000           ; z' hi
    sta.l dsp1_o2+1
    plp
    rtl

.ENDS
