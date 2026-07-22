;----------------------------------------------------------------------
; apu_switch — program A: the play_noise drum kit, made hot-swappable
;
; Same DSP recipe as examples/audio/play_noise (NON noise drums through
; a single-tap FIR echo), with ONE addition: APU_CHECK_RESET inside the
; wait loop. Every ~ms the SPC700 peeks CPUIO0; when the 65816's
; apuReset() writes APU_RESET_MAGIC, the program silences the DSP and
; jumps back to the IPL boot ROM so the next program can be uploaded.
;
; Memory layout rule: code page is $0200-$07FF; the echo buffer lives
; at $8800-$FFFF (ESA $88, cleared at init) — no BRR data anywhere.
;----------------------------------------------------------------------

.include "memmap_spc700.inc"

.MACRO WDSP ARGS reg, val
    mov $F2, #reg
    mov $F3, #val
.ENDM

; krom's SPCWaitMS family + the hot-swap poll: T0DIV counts 8 kHz
; clocks, so div 8 = 1 ms grain, 16 = 2 ms, 32 = 4 ms; y = tick count.
.MACRO WAITMS ARGS ticks, div
    mov y, #ticks
    mov $FA, #div       ; T0DIV
    mov $F1, #$01       ; CONTROL: start timer 0
_wait\@:
    APU_CHECK_RESET     ; hot-swap requested? (templates/memmap_spc700.inc)
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

    WDSP $5C, $00       ; KOFF release
    WDSP $0C, 63        ; MVOLL
    WDSP $1C, 63        ; MVOLR

    ; clear the echo buffer region ($8800-$FFFF, $78 pages) so the FIR
    ; ring starts silent — krom's SPCRAMClear, self-modifying hi byte
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

    ; --- echo + noise routing (krom's exact values) ---
    WDSP $6D, $88       ; ESA: echo buffer at $8800
    WDSP $7D, 5         ; EDL: echo delay
    WDSP $4D, %00000001 ; EON: voice 0 into the echo
    WDSP $3D, %00000001 ; NON: voice 0 IS the noise generator
    WDSP $6C, 0         ; FLG: enable echo writes (noise clock set per drum)
    WDSP $0D, 80        ; EFB: echo feedback
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

    ; Hot-swap hygiene: the DSP arrives DIRTY (whatever the previous
    ; program left). Zero the regs of the voice the cello used so no
    ; residual pitch/volume/envelope bleeds into the kit.
    WDSP $10, 0         ; V1VOLL
    WDSP $11, 0         ; V1VOLR
    WDSP $15, $00       ; V1ADSR1
    WDSP $16, $00       ; V1ADSR2
    WDSP $17, 0         ; V1GAIN

    WDSP $00, 127       ; V0VOLL
    WDSP $01, 127       ; V0VOLR
    WDSP $07, 127       ; V0GAIN

drum_loop:
    ; kick — low noise clock, punchy envelope, 240*2 ms
    WDSP $6C, 14        ; FLG: noise clock 14 (low rumble), echo on
    WDSP $05, %10001110 ; V0ADSR1
    WDSP $06, %11110110 ; V0ADSR2
    WDSP $4C, %00000001 ; KON (bare — the recipe)
    WAITMS 240, 16

    ; closed hi-hat — 16 kHz hiss, fast one-shot, 240 ms
    WDSP $6C, 30
    WDSP $05, %10101111
    WDSP $06, %11111100
    WDSP $4C, %00000001
    WAITMS 240, 8

    ; open hi-hat — same hiss, long release, 240 ms
    WDSP $6C, 30
    WDSP $05, %10001100
    WDSP $06, %10011100
    WDSP $4C, %00000001
    WAITMS 240, 8

    ; snare — 8 kHz noise, hard attack + decay, 240*4 ms
    WDSP $6C, 29
    WDSP $05, %11111010
    WDSP $06, %11111000
    WDSP $4C, %00000001
    WAITMS 240, 32

    jmp !drum_loop
