;==============================================================================
; SPC700 Audio Driver - SFX example
; Uploads driver + sample, waits for play command via port0
;==============================================================================

.SECTION ".spc_code" SUPERFREE

;------------------------------------------------------------------------------
; spc_init - Upload driver and sample to SPC, execute driver
;------------------------------------------------------------------------------
spc_init:
    php
    phb
    phd
    sep #$20
    rep #$10

    ; Set direct page and data bank to 0
    lda #$00
    pha
    plb
    rep #$20
    lda #$0000
    tcd
    sep #$20

    ; Wait for SPC ready ($BBAA in port0-1)
-   ldx $2140
    cpx #$BBAA
    bne -

    ; Start transfer to $0200
    stx $2141
    ldx #$0200
    stx $2142
    lda #$CC
    sta $2140
-   cmp $2140
    bne -

    ; Upload spc_blob (driver + sample directory + BRR sample)
    lda.l spc_blob
    xba
    lda #$00
    ldx #$0001
    bra @start

@loop:
    xba
    lda.l spc_blob,x
    inx
    xba
-   cmp $2140
    bne -
    ina

@start:
    rep #$20
    sta $2140
    sep #$20
    cpx #spc_blob_end - spc_blob
    bcc @loop

-   cmp $2140
    bne -

    ; Execute driver at $0200
    ina
    ina
    ldx #$0200
    stx $2142
    stz $2141
    sta $2140

    ; Wait for driver ready (writes 0 to port0)
-   lda $2140
    bne -

    ; Clear SNES->SPC port0 so driver doesn't see stale command
    stz $2140

    pld
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; spc_play - Send play command to SPC driver
;------------------------------------------------------------------------------
spc_play:
    php
    phb
    sep #$20

    lda #$00
    pha
    plb

    ; Send play command ($01) to port0
    lda #$01
    sta $2140

    ; Wait for acknowledgment (driver echoes command back)
-   cmp $2140
    bne -

    ; Reset to $00
    lda #$00
    sta $2140

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; SPC Blob - uploaded as one contiguous block to $0200
;------------------------------------------------------------------------------
spc_blob:
spc_driver:
    ; Clear direct page $00-$EF
    .db $CD, $00             ; mov X, #$00
    .db $E8, $00             ; mov A, #$00
    .db $AF                  ; mov (X)+, A
    .db $C8, $F0             ; cmp X, #$F0
    .db $D0, $FB             ; bne -5

    ; Signal ready by writing 0 to port0
    .db $8F, $00, $F4        ; mov $F4, #$00

    ; Key off all voices
    .db $8F, $5C, $F2, $8F, $FF, $F3

    ; FLG = $20 (unmute, disable echo writes)
    .db $8F, $6C, $F2, $8F, $20, $F3

    ; Clear effect registers
    .db $8F, $4D, $F2, $8F, $00, $F3  ; EON = 0
    .db $8F, $3D, $F2, $8F, $00, $F3  ; NON = 0
    .db $8F, $2D, $F2, $8F, $00, $F3  ; PMON = 0
    .db $8F, $7D, $F2, $8F, $00, $F3  ; EDL = 0
    .db $8F, $6D, $F2, $8F, $00, $F3  ; ESA = 0
    .db $8F, $0D, $F2, $8F, $00, $F3  ; EFB = 0
    .db $8F, $2C, $F2, $8F, $00, $F3  ; EVOLL = 0
    .db $8F, $3C, $F2, $8F, $00, $F3  ; EVOLR = 0

    ; DIR = $03 (sample directory at $0300)
    .db $8F, $5D, $F2, $8F, $03, $F3

    ; Voice 0 setup
    .db $8F, $00, $F2, $8F, $7F, $F3  ; VOLL = $7F
    .db $8F, $01, $F2, $8F, $7F, $F3  ; VOLR = $7F
    .db $8F, $02, $F2, $8F, $40, $F3  ; PITCHL = $40
    .db $8F, $03, $F2, $8F, $04, $F3  ; PITCHH = $04 (pitch $0440)
    .db $8F, $04, $F2, $8F, $00, $F3  ; SRCN = 0
    .db $8F, $05, $F2, $8F, $FF, $F3  ; ADSR1 = $FF
    .db $8F, $06, $F2, $8F, $E0, $F3  ; ADSR2 = $E0
    .db $8F, $07, $F2, $8F, $00, $F3  ; GAIN = 0

    ; Master volume
    .db $8F, $0C, $F2, $8F, $7F, $F3  ; MVOLL = $7F
    .db $8F, $1C, $F2, $8F, $7F, $F3  ; MVOLR = $7F

    ; Clear KOF
    .db $8F, $5C, $F2, $8F, $00, $F3

    ; Main loop - wait for play command on port0
    ; @main_loop (byte 0):
    .db $E4, $F4             ; mov A, $F4 (read port0)
    .db $F0, $FC             ; beq -4 (loop if 0)

    ; Got non-zero command - echo it back
    .db $C4, $F4             ; mov $F4, A (echo command)

    ; Key on voice 0
    .db $8F, $4C, $F2        ; mov $F2, #$4C (KON register)
    .db $8F, $01, $F3        ; mov $F3, #$01 (voice 0)

    ; Wait for port0 to return to 0
    ; @wait_zero (byte 12):
    .db $E4, $F4             ; mov A, $F4
    .db $D0, $FC             ; bne -4 (loop if not 0)

    ; Echo 0 back
    .db $C4, $F4             ; mov $F4, A

    ; Back to main loop
    .db $2F, $EC             ; bra -20 (back to main_loop)
spc_driver_end:

    ; Pad to $0100 bytes so sample dir lands at $0300
    .dsb $0100 - (spc_driver_end - spc_driver), $00

; Sample directory at $0300
sample_dir:
    .db $04, $03, $04, $03   ; Sample 0: start=$0304, loop=$0304

; BRR sample at $0304
    .INCBIN "tada.brr"

spc_blob_end:

.ENDS
