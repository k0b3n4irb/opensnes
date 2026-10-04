;==============================================================================
; SA-1 Boot — Sine-wave murmuration: 128 dots in Lissajous patterns
;==============================================================================
; Each dot = sum of 2 sine harmonics per axis, with prime-number phase
; multipliers creating organic flock-like spread across the screen.
;
; X = sin[bird*3 + frame]/2 + sin[bird*7 + frame*2]/4 + 32
; Y = sin[bird*5 + frame+64]/2 + sin[bird*11 + frame*3]/4 + 16
;
; All memory: sta.l / lda.l (bypass undefined DB register).
; Sine lookup: lda.l SINE,x (opcode $BF — full 24-bit, no DB needed).
;
; Where the SA-1 runs (2026-09-27): the loop and the sine table are copied
; into I-RAM at boot and run from there, so the SA-1 does not share the ROM
; with the main CPU. Built with SA1_CODE_IN := ROM (see the Makefile), both
; stay in ROM, as this example did until then: compare the two with
; `luna profile … --from-frame 60 --until-frame 180` (docs/tutorials/sa1.md).
; The loop is position-independent: brl instead of jmp, and SINE is the
; table's address in whichever memory holds it.
;==============================================================================

.ifdef SA1

.SECTION ".sa1_boot" BANK 0 SLOT "ROM" SEMIFREE   ; the SA-1 reset vector is 16-bit: the program must sit in ROM bank 0 (2026-10-03; SUPERFREE let a big program drift to bank 1 and the SA-1 never booted)

.ACCU 16
.INDEX 16

; --- Constants ---
.EQU NBIRDS      128
.EQU BUF         $3010
.EQU IRAM_CODE   $3300          ; the copied loop ($3300-$35FF)
.EQU IRAM_SINE   $3600          ; the copied sine table ($3600-$36FF)

.ifdef SA1_CODE_IN_ROM
.DEFINE SINE sine_table
.else
.DEFINE SINE $003600
.endif

; --- I-RAM layout ---
; $3000 = magic byte ($A5 = ready)
; $3001 = sync flag (1=data ready, 0=consumed)
; $3002 = frame counter
; $3003 = frame*2 (precomputed)
; $3004 = frame+64 (precomputed, cosine offset)
; $3005 = frame*3 (precomputed)
; $3006 = bird loop counter
; $3007 = (unused)
; $3008 = temp: partial/final screen X
; $3009 = temp: partial/final screen Y
; $300A = temp: bird copy for multiply
; $300B = temp: bird*2 for multiply
; $300C-$300F = (unused)
; $3010-$320F = bird position buffer (128 * 4 bytes)
; $3300-$35FF = the main loop, copied from ROM (I-RAM build)
; $3600-$36FF = the sine table, copied from ROM (I-RAM build)
; stack grows down from $37FF (a few bytes)

SA1Start:
    sei
    clc
    xce
    rep #$30
    .ACCU 16
    .INDEX 16
    lda #$37FF
    tcs                         ; stack top of I-RAM
    lda #$3000
    tcd                         ; direct page in I-RAM
    sep #$20
    .ACCU 8

    ; Enable SA-1 I-RAM writes (bit=1 = WRITABLE)
    lda #$FF
    sta.l $00222A               ; CIWP = $FF

    ; Enable SA-1 BW-RAM writes too: CBWE ($2227) bit 7, 1 = write enabled
    ; (fullsnes; Nintendo manual 4.1.23). crt0 sets the SNES side (SBWE) and
    ; the protection only holds while BOTH are clear (ares bwram.cpp, from
    ; Kirby's Dream Land 3), so this is the form the manual asks for, not a
    ; fix: SA-1 writes to BW-RAM already went through.
    lda #$80
    sta.l $002227               ; CBWE = write enabled

    ; Clear state
    lda #$00
    sta.l $3002                 ; frame = 0
    sta.l $3001                 ; sync = 0

.ifndef SA1_CODE_IN_ROM
    ; Copy the loop and the sine table into I-RAM, then run from there
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l _main,x
    sta.l IRAM_CODE,x
    inx
    cpx #(_main_end - _main)
    bne -
    ldx #$0000
-   lda.l sine_table,x
    sta.l IRAM_SINE,x
    inx
    cpx #256
    bne -
.endif

    ; Signal SNES CPU that SA-1 is alive
    lda #$A5
    sta.l $3000                 ; magic byte

.ifndef SA1_CODE_IN_ROM
    jml IRAM_CODE
.endif

; ======================================================================
; Main loop: wait for sync, compute frame, signal ready
; ======================================================================
_main:
    sep #$20
    .ACCU 8

    ; Wait for SNES CPU to clear sync flag
-   lda.l $3001
    bne -

    ; Advance frame counter
    lda.l $3002
    inc a
    sta.l $3002

    ; Precompute frame multiples (saves 384 instructions per frame)
    asl a                       ; frame*2
    sta.l $3003
    clc
    adc.l $3002                 ; frame*2 + frame = frame*3
    sta.l $3005
    lda.l $3002
    clc
    adc #64                     ; frame + 64 (cosine offset)
    sta.l $3004

    ; Reset bird counter
    lda #$00
    sta.l $3006

; ======================================================================
; Bird loop: 4 sine lookups per bird, ~60 instructions each
; ======================================================================
_bird:

    ; ------------------------------------------------------------------
    ; X = sin[bird*3 + frame]/2 + sin[bird*7 + frame*2]/4 + 32
    ; ------------------------------------------------------------------

    ; --- Lookup 1: sin[bird*3 + frame] ---
    lda.l $3006                 ; bird
    sta.l $3008                 ; save bird
    asl a                       ; bird*2
    clc
    adc.l $3008                 ; bird*2 + bird = bird*3
    clc
    adc.l $3002                 ; + frame
    ; A = idx1 (wraps naturally in 8-bit)
    rep #$20
    .ACCU 16
    and #$00FF                  ; zero-extend for 16-bit X
    tax
    sep #$20
    .ACCU 8
    lda.l SINE,x                ; sin[idx1] — opcode $BF, no DB!
    lsr a                       ; /2 → range 0-127
    sta.l $3008                 ; partial X

    ; --- Lookup 2: sin[bird*7 + frame*2] ---
    lda.l $3006                 ; bird
    sta.l $3009                 ; save bird
    asl a                       ; bird*2
    asl a                       ; bird*4
    asl a                       ; bird*8
    sec
    sbc.l $3009                 ; bird*8 - bird = bird*7
    clc
    adc.l $3003                 ; + frame*2 (precomputed)
    rep #$20
    .ACCU 16
    and #$00FF
    tax
    sep #$20
    .ACCU 8
    lda.l SINE,x                ; sin[idx2]
    lsr a                       ; /2
    lsr a                       ; /4 → range 0-63
    clc
    adc.l $3008                 ; + sin[idx1]/2 → range 0-190
    clc
    adc #32                     ; + 32 → range 32-222
    sta.l $3008                 ; final screen X

    ; ------------------------------------------------------------------
    ; Y = sin[bird*5 + frame+64]/2 + sin[bird*11 + frame*3]/4 + 16
    ; ------------------------------------------------------------------

    ; --- Lookup 3: sin[bird*5 + frame+64] ---
    lda.l $3006                 ; bird
    sta.l $3009                 ; save bird
    asl a                       ; bird*2
    asl a                       ; bird*4
    clc
    adc.l $3009                 ; bird*4 + bird = bird*5
    clc
    adc.l $3004                 ; + frame+64 (precomputed cosine)
    rep #$20
    .ACCU 16
    and #$00FF
    tax
    sep #$20
    .ACCU 8
    lda.l SINE,x                ; sin[idx3] (cosine due to +64)
    lsr a                       ; /2 → range 0-127
    sta.l $3009                 ; partial Y

    ; --- Lookup 4: sin[bird*11 + frame*3] ---
    lda.l $3006                 ; bird
    sta.l $300A                 ; save bird
    asl a                       ; bird*2
    sta.l $300B                 ; save bird*2
    asl a                       ; bird*4
    asl a                       ; bird*8
    clc
    adc.l $300B                 ; bird*8 + bird*2 = bird*10
    clc
    adc.l $300A                 ; bird*10 + bird = bird*11
    clc
    adc.l $3005                 ; + frame*3 (precomputed)
    rep #$20
    .ACCU 16
    and #$00FF
    tax
    sep #$20
    .ACCU 8
    lda.l SINE,x                ; sin[idx4]
    lsr a                       ; /2
    lsr a                       ; /4 → range 0-63
    clc
    adc.l $3009                 ; + sin[idx3]/2 → range 0-190
    clc
    adc #16                     ; + 16 → range 16-206
    sta.l $3009                 ; final screen Y

    ; ------------------------------------------------------------------
    ; Write to buffer: BUF[bird*4] = {X, Y, 0, 0}
    ; ------------------------------------------------------------------
    rep #$20
    .ACCU 16
    lda.l $3006                 ; bird index
    and #$00FF                  ; zero-extend
    asl a
    asl a                       ; bird*4
    tax                         ; X = buffer offset (16-bit)

    sep #$20
    .ACCU 8
    lda.l $3008                 ; screen X
    sta.l BUF,x
    lda.l $3009                 ; screen Y
    sta.l BUF+1,x
    lda #$00
    sta.l BUF+2,x              ; unused
    sta.l BUF+3,x              ; unused

    ; ------------------------------------------------------------------
    ; Next bird (loop body > 128 bytes → can't use bne, use beq+brl;
    ; brl, not jmp, so the loop runs wherever it was copied)
    ; ------------------------------------------------------------------
    lda.l $3006
    inc a
    sta.l $3006
    cmp #NBIRDS
    beq +
    brl _bird
+

    ; Signal SNES CPU that buffer is ready
    lda #$01
    sta.l $3001
    brl _main
_main_end:

; ======================================================================
; 256-byte sine table: sin[i] = round(128 + 127 * sin(2*pi*i/256))
; Range: 1-255. Center: 128. Peak: 255 at i=64. Trough: 1 at i=192.
; ======================================================================
sine_table:
    .db 128,131,134,137,140,144,147,150,153,156,159,162,165,168,171,174
    .db 177,179,182,185,188,191,193,196,199,201,204,206,209,211,213,216
    .db 218,220,222,224,226,228,230,232,234,235,237,239,240,241,243,244
    .db 245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255
    .db 255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246
    .db 245,244,243,241,240,239,237,235,234,232,230,228,226,224,222,220
    .db 218,216,213,211,209,206,204,201,199,196,193,191,188,185,182,179
    .db 177,174,171,168,165,162,159,156,153,150,147,144,140,137,134,131
    .db 128,125,122,119,116,112,109,106,103,100, 97, 94, 91, 88, 85, 82
    .db  79, 77, 74, 71, 68, 65, 63, 60, 57, 55, 52, 50, 47, 45, 43, 40
    .db  38, 36, 34, 32, 30, 28, 26, 24, 22, 21, 19, 17, 16, 15, 13, 12
    .db  11, 10,  8,  7,  6,  6,  5,  4,  3,  3,  2,  2,  2,  1,  1,  1
    .db   1,  1,  1,  1,  2,  2,  2,  3,  3,  4,  5,  6,  6,  7,  8, 10
    .db  11, 12, 13, 15, 16, 17, 19, 21, 22, 24, 26, 28, 30, 32, 34, 36
    .db  38, 40, 43, 45, 47, 50, 52, 55, 57, 60, 63, 65, 68, 71, 74, 77
    .db  79, 82, 85, 88, 91, 94, 97,100,103,106,109,112,116,119,122,125

.ENDS

.endif
