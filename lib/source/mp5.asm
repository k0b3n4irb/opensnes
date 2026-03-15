;==============================================================================
; OpenSNES MultiPlayer5 (Multitap) Functions
;
; Detection and control functions for the SNES MultiPlayer5 adapter.
; The actual pad reading (pads 2-4) is done in the NMI handler (crt0.asm)
; when snes_mplay5 is set.
;
; Based on PVSnesLib's detectMPlay5 (input.asm) and the original SNES
; development manual (chapter 9.6).
;
; cc65816 calling convention: arguments pushed LEFT-TO-RIGHT.
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

.SECTION ".mp5_text" SUPERFREE

;------------------------------------------------------------------------------
; u8 detectMPlay5(void)
;
; Detect MultiPlayer5 adapter using signature detection from the original
; SNES development manual (chapter 9.6).
;
; Protocol:
;   1. Strobe ON: read REG_JOYB bit 1 eight times → must be all 1s ($FF)
;   2. Strobe OFF: read REG_JOYB bit 1 eight times → must NOT be all 1s
;   If both conditions met, MultiPlayer5 is connected.
;
; Sets snes_mplay5 = 1 if detected, 0 if not.
;
; Returns: 1 if detected, 0 if not
;------------------------------------------------------------------------------
detectMPlay5:
    php
    phb
    phx

    sep #$20                ; 8-bit A
    lda.b #$00              ; Set data bank to $00
    pha
    plb

    stz.w snes_mplay5       ; Assume not connected
    stz.w mp5read

    ; Phase 1: Strobe ON — read 8 bits from REG_JOYB bit 1
    ldx #8
    lda.b #1
    sta.w $4016             ; REG_JOYA = 1 (strobe ON)

@check_strobe_on:
    lda.w $4017             ; REG_JOYB
    lsr                     ; bit 1 → bit 0
    and #$01                ; isolate bit
    ora.w mp5read           ; accumulate
    dex
    beq +
    asl
    sta.w mp5read
    bra @check_strobe_on
+   sta.w mp5read

    stz.w $4016             ; REG_JOYA = 0 (strobe OFF)

    lda.w mp5read           ; Must be $FF for MP5
    cmp #$FF
    bne @no_mp5

    ; Phase 2: Strobe OFF — read 8 bits, must NOT be $FF
    ldx #8
    stz.w mp5read

@check_strobe_off:
    lda.w $4017             ; REG_JOYB
    lsr                     ; bit 1 → bit 0
    and #$01
    ora.w mp5read
    dex
    beq +
    asl
    sta.w mp5read
    bra @check_strobe_off
+   sta.w mp5read

    lda.w mp5read           ; Must NOT be $FF
    cmp #$FF
    beq @no_mp5

    ; MultiPlayer5 detected!
    lda.b #$01
    sta.w snes_mplay5

@no_mp5:
    plx
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; u8 mp5IsConnected(void)
;
; Returns: 1 if MultiPlayer5 is active, 0 if not
;------------------------------------------------------------------------------
mp5IsConnected:
    php
    sep #$20
    lda.l snes_mplay5
    and #$01
    rep #$20                ; Return value in 16-bit A
    and #$00FF
    plp
    rtl

;------------------------------------------------------------------------------
; void mp5Enable(void)
;
; Manually enable MultiPlayer5 reading in the NMI handler.
; Use this if you know a multitap is connected without calling detectMPlay5.
;------------------------------------------------------------------------------
mp5Enable:
    php
    sep #$20
    lda #$01
    sta.l snes_mplay5
    plp
    rtl

;------------------------------------------------------------------------------
; void mp5Disable(void)
;
; Disable MultiPlayer5 reading in the NMI handler.
; After calling this, only pads 0-1 are read.
;------------------------------------------------------------------------------
mp5Disable:
    php
    sep #$20
    lda #$00
    sta.l snes_mplay5
    plp
    rtl

.ENDS
