;----------------------------------------------------------------------
; speech_synth — APU-side program: the SNES says "OPEN SNES"
;
; Port of krom (Peter Lemon)'s SpeechSynth method: speech = sequencing
; a PHONEME BANK on one DSP voice — per phoneme: SRCN (sample select),
; ADSR (plosives one-shot fast-release, vowels sustained), pitch for
; prosody, and a timed wait. krom's bank has 32 phonemes and spells
; "PETER LEMON"; ours is a 5-phoneme original bank (formant-synthesized)
; spelling "OPEN SNES".
;
; Memory layout rule: code page is $0200-$07FF,
; directory at $0800, samples from $0900 — the sequencer will NOT crash
; into its own directory.
;----------------------------------------------------------------------

.include "memmap_spc700.inc"

.MACRO WDSP ARGS reg, val
    mov $F2, #reg
    mov $F3, #val
.ENDM

; phoneme indices in the directory
.DEFINE PH_AU 0
.DEFINE PH_PP 1
.DEFINE PH_EA 2
.DEFINE PH_NN 3
.DEFINE PH_SS 4

.BANK 0 SLOT 0
.ORG $0200

start:
    ; --- SPC_INIT ---
    WDSP $6C, $20       ; FLG: echo writes off
    WDSP $4C, $00       ; KON reset
    WDSP $5C, $FF       ; KOFF all
    WDSP $2D, $00       ; PMON off
    WDSP $3D, $00       ; NON off
    WDSP $4D, $00       ; EON off
    WDSP $2C, 0         ; EVOLL
    WDSP $3C, 0         ; EVOLR

    ; --- krom's SpeechSynth master config: dry, loud ---
    WDSP $5D, $08       ; DIR: directory page $0800
    WDSP $5C, $00       ; KOFF release
    WDSP $0C, 127       ; MVOLL
    WDSP $1C, 127       ; MVOLR
    WDSP $6C, %00100000 ; FLG: keep echo writes disabled (dry voice)
    WDSP $00, 127       ; V0VOLL
    WDSP $01, 127       ; V0VOLR
    WDSP $07, 127       ; V0GAIN

    ; timer 0: 4 ms tick (krom's SPCWaitMS uses 1 ms grain; 4 ms keeps
    ; byte-sized durations: 32 ms = 8 ticks, 256 ms = 64 ticks)
    mov $FA, #32
    mov $F1, #$01

speak_loop:
    mov x, #0
next_phoneme:
    mov a, !phrase+0+x  ; SRCN (or $FF = end of phrase)
    cmp a, #$FF
    beq phrase_done
    mov $F2, #$04       ; V0SRCN
    mov $F3, a
    mov a, !phrase+1+x  ; ADSR1
    mov $F2, #$05
    mov $F3, a
    mov a, !phrase+2+x  ; ADSR2
    mov $F2, #$06
    mov $F3, a
    mov a, !phrase+3+x  ; pitch high byte (prosody; low byte = 0)
    mov $F2, #$03
    mov $F3, a
    mov $F2, #$02       ; V0PITCHL = 0
    mov $F3, #$00
    ; krom's recipe verbatim: a BARE KON per phoneme — the ADSR values
    ; carry the articulation. (A KOFF pulse here truncates the previous
    ; waveform mid-sample: audible click + crackle, owner-tested.)
    mov $F2, #$4C       ; KON: speak the phoneme
    mov $F3, #%00000001
    mov a, !phrase+4+x  ; duration in ~32 ms ticks
    mov y, a
wait_ph:
    mov a, $FD          ; T0OUT (read clears)
    beq wait_ph
    dbnz y, wait_ph
    inc x
    inc x
    inc x
    inc x
    inc x
    bra next_phoneme
phrase_done:
    ; krom: the phrase just ends with a long wait — NO KOFF (the ADSR
    ; decays the last phoneme naturally; a KOFF here was the 'toc')
    mov y, #128         ; ~512 ms
wait_gap:
    mov a, $FD
    beq wait_gap
    dbnz y, wait_gap
    bra speak_loop

;----------------------------------------------------------------------
; The phrase: "OPEN SNES" = AU P EA N (pause) S N EA S
; Per entry: SRCN, ADSR1, ADSR2, PITCHH, ticks(~32 ms)
; ADSR values from krom: vowels %11110111/%11111100 (sustained decay),
; plosives/fricatives %11111111/%11100000 (fast one-shot)
;----------------------------------------------------------------------
phrase:
    ; krom's pacing: consonants BRIEF (his 32 ms), vowels LONG (his
    ; 256 ms) — an 8:1 ratio. The first table had 200 ms fricatives
    ; drowning the vowels ('psich psich', owner-tested).
    .db PH_AU, %11110111, %11111100, $10, 64  ; O   (256 ms)
    .db PH_PP, %11111111, %11100000, $10, 8   ; P   (32 ms)
    .db PH_EA, %11110111, %11111100, $11, 64  ; E
    .db PH_NN, %11110111, %11111100, $10, 48  ; N
    .db PH_SS, %11111111, %11100000, $10, 16  ; S   (64 ms)
    .db PH_NN, %11110111, %11111100, $10, 32  ; N
    .db PH_EA, %11110111, %11111100, $0F, 64  ; E
    .db PH_SS, %11111111, %11100000, $10, 16  ; S
    .db $FF                                    ; end of phrase

;----------------------------------------------------------------------
; Sample directory at $0800: [start][loop] — vowels/nasal loop from
; block 1 (krom's +9 layout); one-shots have no loop.
;----------------------------------------------------------------------
.ORG $0800
sample_dir:
    .dw ph_au, ph_au + 9
    .dw ph_pp, 0
    .dw ph_ea, ph_ea + 9
    .dw ph_nn, ph_nn + 9
    .dw ph_ss, 0

;----------------------------------------------------------------------
; Phoneme bank (original, gen_phonemes.py)
;----------------------------------------------------------------------
.ORG $0900
ph_au:  .incbin "res/au.brr"
ph_pp:  .incbin "res/pp.brr"
ph_ea:  .incbin "res/ea.brr"
ph_nn:  .incbin "res/nn.brr"
ph_ss:  .incbin "res/ss.brr"
