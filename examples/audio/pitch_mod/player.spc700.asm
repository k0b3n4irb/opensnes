;----------------------------------------------------------------------
; pitch_mod — APU-side program: hardware vibrato via pitch modulation
;
; Port of krom (Peter Lemon)'s PitchMod method: the S-DSP can multiply
; a voice's pitch by the OUTPUT of the previous voice (PMON). Voice 1
; plays a looping cello note at C5; voice 0 plays a tiny single-block
; square wave at pitch $0003 — far below audibility, it is a slow LFO.
; With PMON bit 1 set, voice 0's waveform wobbles voice 1's pitch:
; hardware vibrato, no CPU involved.
;
; Sequence: the cello enters dry; after ~1 s the LFO voice is keyed on
; and the vibrato blooms. The note rings out like a bow stroke (the
; sample's own ADSR — see below) and is re-bowed every 7 s. A long FIR
; echo (EDL 15, feedback 100) does the concert hall.
;
; Envelope note: krom's original sets a flat GAIN 127 — which would
; sustain forever — but his code has no final idle loop, so after the
; last KON the SPC700 falls through the sample directory and executes
; the cello's BRR bytes as opcodes; the decay you hear in his demo is
; that runaway code eventually scribbling on the DSP. We use the ADSR
; the sample was shipped with (its filename encodes ADSR $FF/$E8:
; instant attack, sustain level 7/8, gentle SR-8 fade) so the same
; bow-stroke decay happens by design, then re-key for the next stroke.
;
; Memory layout rule: code page $0200-$07FF, sample directory at $0800,
; BRR data from $0900; echo buffer at $8800-$FFFF.
;----------------------------------------------------------------------

.include "memmap_spc700.inc"

.MACRO WDSP ARGS reg, val
    mov $F2, #reg
    mov $F3, #val
.ENDM

.MACRO WAITMS ARGS ticks, div
    mov y, #ticks
    mov $FA, #div       ; T0DIV (8 kHz clocks per tick)
    mov $F1, #$01       ; CONTROL: start timer 0
_wait\@:
    mov a, $FD          ; T0OUT (read clears)
    beq _wait\@
    dbnz y, _wait\@
.ENDM

.BANK 0 SLOT 0
.ORG $0200

start:
    ; --- SPC_INIT (krom's) ---
    WDSP $6C, $20       ; FLG: echo writes off during setup
    WDSP $4C, $00       ; KON reset
    WDSP $5C, $FF       ; KOFF all
    WDSP $2D, $00       ; PMON off
    WDSP $3D, $00       ; NON off
    WDSP $4D, $00       ; EON off
    WDSP $2C, 0         ; EVOLL
    WDSP $3C, 0         ; EVOLR

    WDSP $5D, $08       ; DIR: sample directory page $0800
    WDSP $5C, $00       ; KOFF release
    WDSP $0C, 127       ; MVOLL
    WDSP $1C, 127       ; MVOLR

    ; clear the echo buffer region ($8800-$FFFF, $78 pages)
    mov a, #$00
    mov x, #$78
    mov y, #$00
clear_page:
    mov !$8800+y, a
    inc y
    bne clear_page
    inc !clear_page+2   ; bump the sta's address hi byte to the next page
    dec x
    bne clear_page

    ; --- long hall echo on the cello (krom's exact values) ---
    WDSP $6D, $88       ; ESA: echo buffer at $8800
    WDSP $7D, 15        ; EDL: maximum delay (~240 ms)
    WDSP $4D, %00000010 ; EON: voice 1 into the echo
    WDSP $6C, 0         ; FLG: enable echo writes
    WDSP $0D, 100       ; EFB: echo feedback
    WDSP $0F, 127       ; FIR0 (single-tap filter)
    WDSP $1F, 0         ; FIR1
    WDSP $2F, 0         ; FIR2
    WDSP $3F, 0         ; FIR3
    WDSP $4F, 0         ; FIR4
    WDSP $5F, 0         ; FIR5
    WDSP $6F, 0         ; FIR6
    WDSP $7F, 0         ; FIR7
    WDSP $2C, 25        ; EVOLL
    WDSP $3C, 25        ; EVOLR

    WDSP $04, 0         ; V0SRCN: LFO square block
    WDSP $14, 1         ; V1SRCN: cello
    WDSP $2D, %00000010 ; PMON: voice 0's output modulates voice 1's pitch

    WDSP $00, 0         ; V0VOLL — the LFO is never heard...
    WDSP $01, 0         ; V0VOLR
    WDSP $07, 2         ; V0GAIN: ...its tiny AMPLITUDE is the vibrato depth

    WDSP $10, 127       ; V1VOLL
    WDSP $11, 127       ; V1VOLR
    WDSP $15, $FF       ; V1ADSR1: enable, attack 15 (instant), decay 7
    WDSP $16, $E8       ; V1ADSR2: sustain level 7/8, sustain rate 8 (~7 s fade)

    WDSP $02, $03       ; V0PITCHL: pitch $0003 — a slow LFO, not a tone
    WDSP $03, $00       ; V0PITCHH

    WDSP $12, $BB       ; V1PITCHL: C5 (krom's SetPitch: $8BB0 >> 4)
    WDSP $13, $08       ; V1PITCHH

    WDSP $4C, %00000010 ; KON: cello enters dry (first bow stroke)
    WAITMS 250, 32      ; 1000 ms

    WDSP $4C, %00000001 ; KON: LFO starts — vibrato blooms

bow_loop:
    WAITMS 250, 224     ; 7000 ms — let the stroke ring out (SR 8 fade)
    WDSP $4C, %00000010 ; re-bow: KON restarts the sample AND the envelope
    bra bow_loop

;----------------------------------------------------------------------
; Sample directory at $0800: [start][loop]
;----------------------------------------------------------------------
.ORG $0800
sample_dir:
    .dw lfo_block, lfo_block        ; DIR 0: single looping BRR block
    .dw cello, cello + 4167         ; DIR 1: cello (krom's loop point)

;----------------------------------------------------------------------
; Sample data from $0900
;----------------------------------------------------------------------
.ORG $0900
lfo_block:
    ; krom's 9-byte LFO wave: header $C3 (range $C, filter 0, loop+end),
    ; nibbles +7/-7 — a square wave the Gaussian interpolator smooths
    .db $C3, $77, $99, $77, $99, $77, $99, $77, $99
cello:
    .incbin "res/cello.brr"
