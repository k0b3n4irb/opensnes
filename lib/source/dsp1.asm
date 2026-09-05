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
    dsp1_o3 dsb 2
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

;------------------------------------------------------------------------------
; void dsp1Project(s16 x, s16 y, s16 z)   (command $06, world -> screen)
;   dsp1_o0 = H (screen X), dsp1_o1 = V (screen Y), dsp1_o2 = M (scale/depth)
;   Requires a prior dsp1Parameter to define the projection plane.
;------------------------------------------------------------------------------
dsp1Project:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$06                ; command $06 = Project
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
    lda.l $308000           ; H lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; H hi
    sta.l dsp1_o0+1
    jsr dsp1_rqm
    lda.l $308000           ; V lo
    sta.l dsp1_o1
    jsr dsp1_rqm
    lda.l $308000           ; V hi
    sta.l dsp1_o1+1
    jsr dsp1_rqm
    lda.l $308000           ; M lo
    sta.l dsp1_o2
    jsr dsp1_rqm
    lda.l $308000           ; M hi
    sta.l dsp1_o2+1
    plp
    rtl

;------------------------------------------------------------------------------
; void dsp1Parameter(s16 fx, s16 fy, s16 fz, s16 lfe, s16 les, u16 aas, u16 azs)
;   (command $02) — projection-plane setup. In 7 words, out 4 words:
;   dsp1_o0 = Cx, dsp1_o1 = Cy (raster coefficients), dsp1_o2/o3 = reserved
;   words whose meaning is unconfirmed (see dsp1_reference.md, marker <>).
;------------------------------------------------------------------------------
dsp1Parameter:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$02                ; command $02 = Parameter
    sta.l $308000
    jsr dsp1_rqm
    lda 17,s                ; fx
    sta.l $308000
    jsr dsp1_rqm
    lda 18,s                ; fx
    sta.l $308000
    jsr dsp1_rqm
    lda 15,s                ; fy
    sta.l $308000
    jsr dsp1_rqm
    lda 16,s                ; fy
    sta.l $308000
    jsr dsp1_rqm
    lda 13,s                ; fz
    sta.l $308000
    jsr dsp1_rqm
    lda 14,s                ; fz
    sta.l $308000
    jsr dsp1_rqm
    lda 11,s                ; lfe
    sta.l $308000
    jsr dsp1_rqm
    lda 12,s                ; lfe
    sta.l $308000
    jsr dsp1_rqm
    lda 9,s                 ; les
    sta.l $308000
    jsr dsp1_rqm
    lda 10,s                ; les
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; aas
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; aas
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; azs
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; azs
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; Cx lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; Cx hi
    sta.l dsp1_o0+1
    jsr dsp1_rqm
    lda.l $308000           ; Cy lo
    sta.l dsp1_o1
    jsr dsp1_rqm
    lda.l $308000           ; Cy hi
    sta.l dsp1_o1+1
    jsr dsp1_rqm
    lda.l $308000           ; out word 3 lo
    sta.l dsp1_o2
    jsr dsp1_rqm
    lda.l $308000           ; out word 3 hi
    sta.l dsp1_o2+1
    jsr dsp1_rqm
    lda.l $308000           ; out word 4 lo
    sta.l dsp1_o3
    jsr dsp1_rqm
    lda.l $308000           ; out word 4 hi
    sta.l dsp1_o3+1
    plp
    rtl

;------------------------------------------------------------------------------
; u16 dsp1Distance(s16 x, s16 y, s16 z)  ->  A   (command $28, sqrt(x²+y²+z²))
;------------------------------------------------------------------------------
dsp1Distance:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$28                ; command $28 = Distance
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
; s16 dsp1Range(s16 x, s16 y, s16 z, u16 r)  ->  A   (command $18)
;   returns (x²+y²+z²) - r² : negative/zero = inside the sphere.
;------------------------------------------------------------------------------
dsp1Range:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$18                ; command $18 = Range
    sta.l $308000
    jsr dsp1_rqm
    lda 11,s                ; x
    sta.l $308000
    jsr dsp1_rqm
    lda 12,s                ; x
    sta.l $308000
    jsr dsp1_rqm
    lda 9,s                 ; y
    sta.l $308000
    jsr dsp1_rqm
    lda 10,s                ; y
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; z
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; z
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; r
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; r
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
; void dsp1Target(s16 h, s16 v)   (command $0E, screen -> ground plane)
;   dsp1_o0 = ground X, dsp1_o1 = ground Y (same convention as Cx/Cy).
;   (h, v) are raster-style screen coordinates relative to the imaginary
;   centre (v down-positive). Requires a prior dsp1Parameter.
;------------------------------------------------------------------------------
dsp1Target:
    php
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$0E                ; command $0E = Target
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; h
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; h
    sta.l $308000
    jsr dsp1_rqm
    lda 5,s                 ; v
    sta.l $308000
    jsr dsp1_rqm
    lda 6,s                 ; v
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; x lo
    sta.l dsp1_o0
    jsr dsp1_rqm
    lda.l $308000           ; x hi
    sta.l dsp1_o0+1
    jsr dsp1_rqm
    lda.l $308000           ; y lo
    sta.l dsp1_o1
    jsr dsp1_rqm
    lda.l $308000           ; y hi
    sta.l dsp1_o1+1
    plp
    rtl

;------------------------------------------------------------------------------
; void dsp1Raster(u8 *ab, u8 *cd, s16 vs, u16 count)   (command $0A, stream)
;   Streams `count` per-raster Mode 7 matrices starting at raster `vs`.
;   Each raster's A,B go to ab[4i..4i+3] (A lo, A hi, B lo, B hi) and C,D to
;   cd[4i..4i+3] — the payload layout of an HDMA_MODE_2REG_2X repeat block
;   targeting M7A/M7B and M7C/M7D. Terminates per the manual: read A,B,C of
;   raster vs+count, then write $8000 in place of D (a write on any other
;   slot is swallowed by the firmware — measured 2026-09-03, see
;   .claude/notes/tech/dsp1_reference.md §14). $0A is used, not $1A: $1A
;   never returns to command mode on the DSP-1B firmware.
;
;   Stack after PHP (post-A6 4-byte pointer slots):
;     5-6,s   = count        7-8,s   = vs
;     9-10,s  = cd low 16    11,s    = cd bank    12,s = pad
;     13-14,s = ab low 16    15,s    = ab bank    16,s = pad
;   Far pointers are parked in tcc__r0 (ab) / tcc__r1 (cd) as 24-bit
;   direct-page pointers for `sta [dp],y`; count is exhausted in tcc__r2.
;------------------------------------------------------------------------------
dsp1Raster:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 13,s                ; ab (low 16)
    sta.l tcc__r0
    lda 15,s                ; ab (bank byte, high byte = pad)
    sta.l tcc__r0+2
    lda 9,s                 ; cd (low 16)
    sta.l tcc__r1
    lda 11,s                ; cd (bank byte, high byte = pad)
    sta.l tcc__r1+2
    lda 5,s                 ; count
    sta.l tcc__r2
    bne +                   ; count == 0: nothing to stream
    brl dsp1Raster_done
+
    ldy #0                  ; Y = byte index into both tables
    sep #$20
    .ACCU 8
    jsr dsp1_rqm
    lda #$0A                ; command $0A = Raster
    sta.l $308000
    jsr dsp1_rqm
    lda 7,s                 ; vs
    sta.l $308000
    jsr dsp1_rqm
    lda 8,s                 ; vs
    sta.l $308000
    rep #$20
    .ACCU 16
    ; Hot loop, 16-bit A. RQM is polled once per WORD (SR is mirrored at
    ; $30C001, so bit 15 of the 16-bit read is RQM) and the word is read
    ; with one 16-bit access ($308000 = DR lo, $308001 = DR mirror -> hi):
    ; RQM stays set until the second byte of a word is taken, so a per-word
    ; poll is the protocol, per-byte polling was redundant. ~36 us/raster.
dsp1Raster_line:
-   lda.l $30C000           ; RQM (bit 15 of the mirrored pair)
    bpl -
    lda.l $308000           ; A
    sta [tcc__r0],y
-   lda.l $30C000
    bpl -
    lda.l $308000           ; B
    iny
    iny
    sta [tcc__r0],y
    dey
    dey
-   lda.l $30C000
    bpl -
    lda.l $308000           ; C
    sta [tcc__r1],y
-   lda.l $30C000
    bpl -
    lda.l $308000           ; D
    iny
    iny
    sta [tcc__r1],y
    iny
    iny
    dec.b tcc__r2           ; count (direct page, register area at $0000)
    bne dsp1Raster_line
    ; Terminate: consume A, B, C of the next raster, then $8000 instead of D.
    ldx #3
dsp1Raster_drain:
-   lda.l $30C000
    bpl -
    lda.l $308000
    dex
    bne dsp1Raster_drain
-   lda.l $30C000
    bpl -
    lda #$8000              ; sentinel, written lo then hi by the 16-bit store
    sta.l $308000
dsp1Raster_done:
    plp
    rtl

;------------------------------------------------------------------------------
; u16 dsp1Present(void)  ->  A   (1 = DSP-1 responding, 0 = absent/inert)
;   Known-answer test: Multiply $4000 x $4000 must return $2000 (0.5*0.5=0.25
;   in 1.15). On a board without the chip (or an emulator without firmware)
;   the open-bus/inert reads cannot produce the exact product.
;   Bounded poll: unlike dsp1_rqm, gives up after ~64K reads so a missing
;   chip returns 0 instead of hanging.
;------------------------------------------------------------------------------
dsp1Present:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldy #0
-   lda.l $30C000
    bmi +
    dey
    bne -
    bra dsp1Present_fail    ; RQM never came up: no chip
+   lda #$00                ; command $00 = Multiply
    sta.l $308000
    jsr dsp1_rqm
    lda #$00                ; a lo ($4000 LSB first)
    sta.l $308000
    jsr dsp1_rqm
    lda #$40                ; a hi
    sta.l $308000
    jsr dsp1_rqm
    lda #$00                ; b lo
    sta.l $308000
    jsr dsp1_rqm
    lda #$40                ; b hi
    sta.l $308000
    jsr dsp1_rqm
    lda.l $308000           ; product lo — expect $00
    cmp #$00
    bne dsp1Present_fail
    jsr dsp1_rqm
    lda.l $308000           ; product hi — expect $20
    cmp #$20
    bne dsp1Present_fail
    plp
    rep #$20
    .ACCU 16
    lda #1
    rtl
dsp1Present_fail:
    plp
    rep #$20
    .ACCU 16
    lda #0
    rtl

.ENDS
