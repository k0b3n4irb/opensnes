;==============================================================================
; OpenSNES - audioSetVoiceVolumePitch(): volume and pitch of a voice, one command
;==============================================================================
; u8 audioSetVoiceVolumePitch(u8 voice, u8 volume, u16 pitch)
;
; What a sequencer does to a voice at every tick. In C this cost more than
; the handshake it saves (2026-10-13: 4 680 master cycles for the function,
; 1 970 for the pan split through pointers), so it is written here, as the
; other paths a game runs every frame.
;
; It shares three objects with audio.c, which defines them:
;   audio_pending       the command the driver has not acknowledged (0: none)
;   audio_seq           the sequence bits of the last command, 1..3
;   audio_voice_mirror  eight VoiceMirror of 10 bytes (a _Static_assert in
;                       audio.c pins the size):
;       +1 volume   +2 pan   +4 pitch (16)   +6 vol_lr (left, right)
;       +8 sent (SENT_VOL 1, SENT_PITCH 2)   +9 pan_drv
;
; The protocol is audio.c's cmd_post(): wait (bounded) for the ack of the
; previous command, write p0, p1 low, p1 high to $2141-$2143, then the
; command byte (sequence << 6 | opcode) to $2140, and return without waiting
; for its own ack.
;
;   VPAN $11  p0 = voice, p1 low = weight 0..16      only when the pan changed
;   VSET $10  p0 = volume | voice bit 2 << 7
;             p1 = pitch | voice bits 0-1 << 14
;
; The driver splits the volume by the weight (audio_driver.spc700.asm,
; cmd_vset). The mirror must hold what the driver wrote, so that the next
; audioPlaySampleOn() knows what it need not send again: for hard left, hard
; right and the centre it is computed here without a multiply; for any other
; pan SENT_VOL is cleared instead (the next note sends its volume again).
;
; Calling convention (cc65816): arguments pushed left to right; with the php
; below, pitch is at 5,s, volume at 7,s, voice at 9,s.
;
; tcc__r0 scratch, tcc__r1 the pitch, tcc__r2 the volume, tcc__r3 scratch.
; Ports by long address: the data bank is $7E in an NMI callback.
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

.DEFINE AV_SPIN     20000       ; polls of one ack before giving up (as audio.c)
.DEFINE AV_VOLUME   1
.DEFINE AV_PAN      2
.DEFINE AV_PITCH    4
.DEFINE AV_VOL_L    6
.DEFINE AV_VOL_R    7
.DEFINE AV_SENT     8
.DEFINE AV_PAN_DRV  9

.SECTION ".text.audioSetVoiceVolumePitch" SUPERFREE

audioSetVoiceVolumePitch:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    lda 9,s                     ; voice
    and.w #$00FF
    cmp.w #8
    bcc @voice_ok
    lda.w #2                    ; AUDIO_ERR_INVALID_ID
    plp
    rtl

@voice_ok:
    asl a
    sta.b tcc__r0
    asl a
    asl a
    clc
    adc.b tcc__r0
    tax                         ; X = voice * 10: its mirror

    lda 5,s                     ; pitch
    cmp.w #$4000
    bcc +
    lda.w #$3FFF                ; the DSP pitch is 14-bit
+   sta.b tcc__r1

    sep #$20
    .ACCU 8
    lda 7,s                     ; volume
    bpl +
    lda.b #127                  ; signed on the DSP: 128 and above inverts the phase
+   sta.b tcc__r2

    ; the driver holds a pan for the voice: tell it only when it changed
    lda.w audio_voice_mirror+AV_PAN,x
    cmp.w audio_voice_mirror+AV_PAN_DRV,x
    beq @pan_known
    jsr @wait
    beq +
    brl @timeout
+   lda.w audio_voice_mirror+AV_PAN,x
    sta.w audio_voice_mirror+AV_PAN_DRV,x
    jsr @weight
    sta.l $002142               ; p1 low: the weight
    lda.b #0
    sta.l $002143
    lda 9,s                     ; voice
    sta.l $002141
    lda.b #$11                  ; VPAN
    jsr @send

@pan_known:
    jsr @wait
    beq +
    brl @timeout
+   lda 9,s                     ; voice
    and.b #4
    beq +
    lda.b #$80                  ; its bit 2 above the volume
+   ora.b tcc__r2
    sta.l $002141
    lda.b tcc__r1
    sta.l $002142
    lda 9,s                     ; voice
    and.b #3
    clc
    ror a
    ror a
    ror a                       ; its bits 0-1 above the pitch
    ora.b tcc__r1+1
    sta.l $002143
    lda.b #$10                  ; VSET
    jsr @send

    ; the mirror: what the driver wrote
    lda.b tcc__r2
    sta.w audio_voice_mirror+AV_VOLUME,x
    lda.w audio_voice_mirror+AV_PAN,x
    jsr @weight
    beq @m_left
    cmp.b #16
    beq @m_right
    cmp.b #8
    beq @m_centre
    ; another pan: the split is the driver's multiply; forget the volume
    lda.w audio_voice_mirror+AV_SENT,x
    and.b #$FE
    ora.b #$02
    sta.w audio_voice_mirror+AV_SENT,x
    bra @m_pitch
@m_left:
    lda.b tcc__r2
    sta.w audio_voice_mirror+AV_VOL_L,x
    stz.w audio_voice_mirror+AV_VOL_R,x
    bra @m_sent
@m_right:
    stz.w audio_voice_mirror+AV_VOL_L,x
    lda.b tcc__r2
    sta.w audio_voice_mirror+AV_VOL_R,x
    bra @m_sent
@m_centre:
    lda.b tcc__r2
    lsr a
    sta.w audio_voice_mirror+AV_VOL_R,x     ; right = volume / 2
    sta.b tcc__r3
    lda.b tcc__r2
    sec
    sbc.b tcc__r3
    sta.w audio_voice_mirror+AV_VOL_L,x     ; left = volume - right
@m_sent:
    lda.w audio_voice_mirror+AV_SENT,x
    ora.b #$03
    sta.w audio_voice_mirror+AV_SENT,x
@m_pitch:
    rep #$20
    .ACCU 16
    lda.b tcc__r1
    sta.w audio_voice_mirror+AV_PITCH,x
    lda.w #0                    ; AUDIO_OK
    plp
    rtl

@timeout:
    rep #$20
    .ACCU 16
    lda.w #4                    ; AUDIO_ERR_TIMEOUT
    plp
    rtl

; A = the pan 0..15 -> its right weight 0..16 (8 at the centre), Z set for 0
@weight:
    .ACCU 8
    cmp.b #16
    bcc +
    lda.b #15
+   cmp.b #9                    ; carry set from 9 up: the weight skips 8/16
    adc.b #0
    rts

; Wait (bounded) for the ack of the pending command. Z set: free to send.
@wait:
    .ACCU 8
    lda.w audio_pending
    beq @wait_ok
    stz.w audio_pending
    ldy.w #AV_SPIN
@wait_spin:
    cmp.l $002140
    beq @wait_ok
    dey
    bne @wait_spin
    lda.b #1
    rts
@wait_ok:
    lda.b #0
    rts

; A = the opcode: the next sequence, the command byte to the port
@send:
    .ACCU 8
    sta.b tcc__r3
    lda.w audio_seq
    cmp.b #3
    bcc +
    lda.b #0
+   inc a
    sta.w audio_seq             ; 1, 2, 3, 1...
    clc
    ror a
    ror a
    ror a                       ; sequence << 6
    ora.b tcc__r3
    sta.l $002140
    sta.w audio_pending
    rts

.ENDS
