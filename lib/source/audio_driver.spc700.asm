;==============================================================================
; OpenSNES Audio System v2 — resident SPC700 driver
;==============================================================================
; Command-driven sample-playback engine: the 65816 C layer (audio.c)
; sends one command at a time over the APU I/O ports; this loop decodes
; it, pokes the S-DSP, and acks. No sequencer, no mixing — the DSP's 8
; hardware voices ARE the engine.
;
; Protocol (see .claude/notes/chantiers/audio_v2.md):
;   $F4 in  = command byte: (seq << 6) | opcode, seq cycles 1..3 so two
;             consecutive commands always differ; 0 = idle; $FE is
;             reserved for APU_CHECK_RESET (apuReset() hot-swap works
;             over this driver).
;   $F5 in  = param0 (8-bit)   $F6/$F7 in  = param1 (16-bit LE)
;   $F4 out = ack (echo of the command byte, written AFTER execution)
;   $F5 out = result0          $F6/$F7 out = result1 (16-bit LE)
;
; ARAM map: driver $0200-$09FF (2 KB budget, enforced by lib/Makefile),
; sample directory $0A00 (64 x 4 bytes), samples $0B00 and up.
;
; Register discipline: the main loop owns A/X/Y freely; no interrupts,
; no timers — a pure reactive poll (~30 cycles/iteration idle).
;==============================================================================

.include "memmap_spc700.inc"

.MACRO WDSP ARGS reg, val
    mov $F2, #reg
    mov $F3, #val
.ENDM

; Zero-page driver state
.DEFINE ZP_LAST_CMD $00     ; last executed command byte (dedup)
.DEFINE ZP_CMD      $01     ; current command byte (for the ack)
.DEFINE ZP_P0       $02     ; latched param0
.DEFINE ZP_P1L      $03     ; latched param1 low
.DEFINE ZP_P1H      $04     ; latched param1 high
.DEFINE ZP_SIZE_L   $05     ; LOAD_SIZE: bytes remaining (16-bit)
.DEFINE ZP_SIZE_H   $06
.DEFINE ZP_DST_L    $07     ; LOAD destination pointer (16-bit, ZP pair
.DEFINE ZP_DST_H    $08     ;  for [ZP]+Y indirect stores)
.DEFINE ZP_LOAD_L   $09     ; start address of the LAST completed LOAD
.DEFINE ZP_LOAD_H   $0A     ;  (DIR_SET builds the directory entry from it)
.DEFINE ZP_IDX      $0B     ; LOAD: expected stream index (no CMP A,X on SPC700)
.DEFINE ZP_TAIL     $0C     ; LOAD: bytes in the last, partial exchange (0-2)
.DEFINE ZP_VBASE    $0D     ; VSET: the voice's DSP register base (voice << 4)
.DEFINE ZP_VOL      $0E     ; VSET: the volume asked for
.DEFINE ZP_VR       $0F     ; VSET: the right volume, volume * weight / 16
.DEFINE ZP_EDL      $18     ; ECHO_CFG: the delay whose ring may still be running (15 at boot: unknown)
.DEFINE ZP_PAGES    $19     ; ECHO_CFG: pages of the ring, delay * 8 (0: no ring yet)
.DEFINE ZP_EVL      $1A     ; the echo volumes ECHO_CFG was given: ECHO_ON gives them
.DEFINE ZP_EVR      $1B     ;  back after a mask of 0 muted them
.DEFINE ZP_ECHO_LIVE $1C    ; 1 while the ring is written (FLG bit 5 clear)
.DEFINE ZP_PAN      $10     ; $10-$17: the pan weight of each voice, 0 (left) to 16 (right), 8 at boot

.DEFINE DRIVER_VERSION 2

.BANK 0 SLOT 0
.ORG $0200

start:
    ; --- one-time DSP safe state ---------------------------------------
    WDSP $6C, $20       ; FLG: echo writes off, mute off, noise clock 0
    WDSP $5C, $FF       ; KOFF: release everything a previous program left
    WDSP $4C, $00       ; KON clear
    WDSP $2D, $00       ; PMON off
    WDSP $3D, $00       ; NON off
    WDSP $4D, $00       ; EON off
    WDSP $2C, $00       ; EVOLL 0
    WDSP $3C, $00       ; EVOLR 0
    ; The echo filter passes the echo through until audioSetEchoFilter()
    ; says otherwise: tap 0 at 127, the others 0. Left at the 0 the registers
    ; hold after a reset, an echo configured and enabled was silent, and
    ; nothing said why (2026-10-13).
    WDSP $0F, $7F
    WDSP $1F, $00
    WDSP $2F, $00
    WDSP $3F, $00
    WDSP $4F, $00
    WDSP $5F, $00
    WDSP $6F, $00
    WDSP $7F, $00
    WDSP $0C, 127       ; MVOL: header default (audioSetVolume can change)
    WDSP $1C, 127
    WDSP $5D, $0A       ; DIR: sample directory page $0A00
    WDSP $5C, $00       ; end the blanket KOFF

    mov a, #8           ; every voice at the centre until VPAN says otherwise
    mov x, #7
-   mov ZP_PAN+x, a
    dec x
    bpl -
    mov a, #15          ; EDL is not ours yet: take the longest ring for the one running
    mov ZP_EDL, a
    mov a, #$00
    mov ZP_PAGES, a
    mov ZP_ECHO_LIVE, a
    mov ZP_LAST_CMD, a
    mov $F4, a          ; ack port = 0: "no command executed yet"

;------------------------------------------------------------------------------
; Main command loop
;------------------------------------------------------------------------------
main_loop:
    APU_CHECK_RESET     ; $FE => silence DSP, ack, back to IPL ($FFC0)

    mov a, $F4          ; command byte from the CPU
    beq main_loop       ; 0 = idle
    cmp a, ZP_LAST_CMD
    beq main_loop       ; input latch still holds the handled command

    ; latch command + params (params were written BEFORE the command
    ; byte on the CPU side, so they are stable now)
    mov ZP_CMD, a
    mov ZP_LAST_CMD, a
    mov a, $F5
    mov ZP_P0, a
    mov a, $F6
    mov ZP_P1L, a
    mov a, $F7
    mov ZP_P1H, a

    ; opcode = cmd & $3F, dispatched through a jump table (a beq chain
    ; would exceed the +-128 branch range past a few handlers)
    mov a, ZP_CMD
    and a, #$3F
    cmp a, #$13
    bcs cmd_unknown     ; opcode >= $13: not in the table
    asl a
    mov x, a
    jmp [!cmd_table+x]

cmd_unknown:
    ; ack anyway so the CPU never times out on a version skew —
    ; result0 = $FF flags the unknown opcode
    mov a, #$FF
    mov $F5, a
    jmp !ack

cmd_table:
    .dw cmd_unknown     ; $00 (idle byte never dispatches, but (seq<<6)|0 could)
    .dw cmd_mvol        ; $01
    .dw cmd_kon         ; $02
    .dw cmd_koff        ; $03
    .dw cmd_vvol        ; $04
    .dw cmd_vpitch      ; $05
    .dw cmd_vadsr       ; $06
    .dw cmd_vgain       ; $07
    .dw cmd_echo_cfg    ; $08
    .dw cmd_echo_fir    ; $09
    .dw cmd_echo_on     ; $0A
    .dw cmd_dir_set     ; $0B
    .dw cmd_load        ; $0C
    .dw cmd_envx        ; $0D
    .dw cmd_ping        ; $0E
    .dw cmd_load_size   ; $0F
    .dw cmd_vset        ; $10
    .dw cmd_vpan        ; $11
    .dw cmd_load_base   ; $12

;------------------------------------------------------------------------------
; $01 MVOL — p0 = master volume, both channels
;------------------------------------------------------------------------------
cmd_mvol:
    mov $F2, #$0C       ; MVOLL
    mov a, ZP_P0
    mov $F3, a
    mov $F2, #$1C       ; MVOLR
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $02 KON — p0 = voice (0-7), p1lo = directory index.
; Sets VxSRCN then keys the voice on. Volume/pitch/envelope are
; separate commands (the C layer sends them first).
;------------------------------------------------------------------------------
cmd_kon:
    mov a, ZP_P0
    xcn a               ; voice << 4 (voice <= 7 so the high nibble is 0)
    or a, #$04          ; VxSRCN
    mov $F2, a
    mov a, ZP_P1L
    mov $F3, a
    mov x, ZP_P0
    mov a, !bit_mask+x
    mov $F2, #$4C       ; KON
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $03 KOFF — p0 = voice MASK (CPU computes 1<<voice, $FF = all).
; Pulse: set the mask, let the DSP latch it, clear it again.
;------------------------------------------------------------------------------
cmd_koff:
    mov $F2, #$5C       ; KOFF
    mov a, ZP_P0
    mov $F3, a
    mov y, #96          ; ~2 DSP sample periods so the release latches
-   dbnz y, -
    mov a, #$00
    mov $F3, a          ; KOFF clear (F2 still $5C)
    jmp !ack

;------------------------------------------------------------------------------
; $04 VVOL — p0 = voice, p1lo = volL, p1hi = volR
;------------------------------------------------------------------------------
cmd_vvol:
    mov a, ZP_P0
    xcn a
    mov $F2, a          ; VxVOLL (base $00)
    mov a, ZP_P1L
    mov $F3, a
    mov a, ZP_P0
    xcn a
    or a, #$01          ; VxVOLR
    mov $F2, a
    mov a, ZP_P1H
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $05 VPITCH — p0 = voice, p1 = 14-bit DSP pitch ($1000 = 1.0x)
;------------------------------------------------------------------------------
cmd_vpitch:
    mov a, ZP_P0
    xcn a
    or a, #$02          ; VxPITCHL
    mov $F2, a
    mov a, ZP_P1L
    mov $F3, a
    mov a, ZP_P0
    xcn a
    or a, #$03          ; VxPITCHH
    mov $F2, a
    mov a, ZP_P1H
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $10 VSET — volume and pitch of a voice in one command (2026-10-13):
;   p0 = volume 0-127 in bits 0-6, voice bit 2 in bit 7
;   p1 = pitch 0-$3FFF in bits 0-13, voice bits 0-1 in bits 14-15
; A voice in tremolo and vibrato used to send VVOL then VPITCH at every
; tick, two handshakes; this is one. The volume is split by the voice's
; pan weight w (ZP_PAN, set by VPAN): right = volume * w / 16, left =
; volume - right — the formula of pan_to_lr() in audio.c, so a voice keeps
; the side audioPlaySampleOn() gave it. Four DSP writes.
;------------------------------------------------------------------------------
cmd_vset:
    mov a, ZP_P1H
    and a, #$C0         ; voice bits 0-1, in bits 6-7
    xcn a               ; -> bits 2-3
    lsr a
    lsr a               ; -> bits 0-1
    mov ZP_VBASE, a
    mov a, ZP_P0
    and a, #$80         ; voice bit 2, in bit 7
    beq +
    mov a, #$04
+   or a, ZP_VBASE      ; A = voice 0-7 (the SPC700 has no OR A,X)
    mov x, a            ; X = voice: the index of its pan weight
    xcn a               ; -> the DSP register base: voice << 4
    mov ZP_VBASE, a
    mov a, ZP_P0
    and a, #$7F
    mov ZP_VOL, a
    mov y, a            ; Y = volume
    mov a, ZP_PAN+x     ; A = weight 0-16
    mul ya              ; YA = volume * weight (at most 127 * 16)
    xcn a
    and a, #$0F         ; the product's bits 4-7
    mov ZP_VR, a
    mov a, y
    xcn a
    and a, #$F0         ; its bits 8-11
    or a, ZP_VR
    mov ZP_VR, a        ; right = product >> 4
    mov a, ZP_VBASE
    mov $F2, a          ; VxVOLL
    mov a, ZP_VOL
    setc
    sbc a, ZP_VR        ; left = volume - right
    mov $F3, a
    mov a, ZP_VBASE
    or a, #$01          ; VxVOLR
    mov $F2, a
    mov a, ZP_VR
    mov $F3, a
    mov a, ZP_VBASE
    or a, #$02          ; VxPITCHL
    mov $F2, a
    mov a, ZP_P1L
    mov $F3, a
    mov a, ZP_VBASE
    or a, #$03          ; VxPITCHH
    mov $F2, a
    mov a, ZP_P1H
    and a, #$3F
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $11 VPAN — p0 = voice, p1lo = pan weight 0 (left) .. 16 (right), 8 = centre.
; Changes no DSP register: the weight is what the next VSET splits with.
;------------------------------------------------------------------------------
cmd_vpan:
    mov a, ZP_P0
    and a, #$07
    mov x, a
    mov a, ZP_P1L
    mov ZP_PAN+x, a
    jmp !ack

;------------------------------------------------------------------------------
; $06 VADSR — p0 = voice, p1lo = ADSR1, p1hi = ADSR2 (packed by C)
;------------------------------------------------------------------------------
cmd_vadsr:
    mov a, ZP_P0
    xcn a
    or a, #$05          ; VxADSR1
    mov $F2, a
    mov a, ZP_P1L
    mov $F3, a
    mov a, ZP_P0
    xcn a
    or a, #$06          ; VxADSR2
    mov $F2, a
    mov a, ZP_P1H
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $07 VGAIN — p0 = voice, p1lo = raw GAIN byte (also clears ADSR1 so
; GAIN mode actually takes effect — hot-swap hygiene lesson from
; apu_switch: ADSR1 bit 7 silently overrides GAIN)
;------------------------------------------------------------------------------
cmd_vgain:
    mov a, ZP_P0
    xcn a
    or a, #$05          ; VxADSR1 = 0 -> GAIN mode
    mov $F2, a
    mov a, #$00
    mov $F3, a
    mov a, ZP_P0
    xcn a
    or a, #$07          ; VxGAIN
    mov $F2, a
    mov a, ZP_P1L
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $0E PING — result0 = driver version (audioIsReady / init handshake)
;------------------------------------------------------------------------------
cmd_ping:
    mov a, #DRIVER_VERSION
    mov $F5, a
    jmp !ack

;------------------------------------------------------------------------------
; $08 ECHO_CFG — p0 = EDL (1-7, validated CPU-side), p1 = EVOLL:EVOLR.
; Full (re)configuration: disable echo writes, place the ring so that it
; ends at $FF00 (ESA = $FF - 8 * EDL: it takes EDL x 2 KB and no more), set
; EDL, wait for the ring it replaces to run out, CLEAR the new ring (an
; uncleared ring plays garbage), set the volumes, re-enable echo writes.
;
; The wait: the S-DSP takes a new EDL when it reaches the end of the echo
; buffer, not when the register is written (snesdev wiki, Errata, S-DSP:
; up to 240 ms for EDL 15). Until then the ring has its old length from the
; NEW start; one that is longer than the new ring would pass $FFFF and go
; on at $0000, over this driver's zero page. Writes stay off for the old
; delay's time: 16 ms per unit, 240 ms the first time (EDL at power-on is
; not ours). Then ~EDL x 9 ms of clear. Config-time cost, not runtime.
;------------------------------------------------------------------------------
cmd_echo_cfg:
    WDSP $6C, $20       ; FLG: echo writes OFF during reconfig
    mov a, ZP_P0
    asl a
    asl a
    asl a               ; pages = EDL * 8
    mov ZP_PAGES, a
    eor a, #$FF         ; $FF - pages: the ring's first page
    mov ZP_DST_H, a
    mov $F2, #$6D       ; ESA
    mov $F3, a
    mov $F2, #$7D       ; EDL
    mov a, ZP_P0
    mov $F3, a

    ; wait 16 ms per unit of the delay being replaced: 11 turns of a
    ; 1536-cycle loop (16.5 ms at 1.024 MHz) per unit
    mov a, ZP_EDL
    mov y, #11
    mul ya              ; A = units * 11 (165 at most)
    mov x, a
    mov y, #$00
echo_wait:
    dec y
    bne echo_wait
    dec x
    bne echo_wait
    mov a, ZP_P0
    mov ZP_EDL, a       ; the ring running from now on

    mov a, ZP_P1L
    mov ZP_EVL, a
    mov a, ZP_P1H
    mov ZP_EVR, a
    call !echo_start
    jmp !ack

; Clear the ring (ZP_PAGES pages under $FF00), set the echo volumes kept in
; ZP_EVL / ZP_EVR and let the DSP write the ring. Echo writes are off on entry.
echo_start:
    mov a, ZP_PAGES
    mov x, a
    eor a, #$FF
    mov ZP_DST_H, a     ; the ring's first page
    mov a, #$00
    mov ZP_DST_L, a
    mov y, #$00
echo_clear:
    mov [ZP_DST_L]+y, a
    inc y
    bne echo_clear
    inc ZP_DST_H        ; next 256-byte page
    dec x
    bne echo_clear

    mov $F2, #$2C       ; EVOLL
    mov a, ZP_EVL
    mov $F3, a
    mov $F2, #$3C       ; EVOLR
    mov a, ZP_EVR
    mov $F3, a
    WDSP $6C, $00       ; FLG: echo writes back ON
    mov a, #$01
    mov ZP_ECHO_LIVE, a
    ret

;------------------------------------------------------------------------------
; $09 ECHO_FIR — p0 = tap index 0-7 ($0F..$7F step $10), p1lo = coef.
; Index 8 is an extension: EFB (echo feedback) — same signed-byte shape.
;------------------------------------------------------------------------------
cmd_echo_fir:
    mov a, ZP_P0
    cmp a, #$08
    beq @fir_efb
    xcn a               ; index << 4
    or a, #$0F          ; FIRx register
    mov $F2, a
    mov a, ZP_P1L
    mov $F3, a
    jmp !ack
@fir_efb:
    mov $F2, #$0D       ; EFB
    mov a, ZP_P1L
    mov $F3, a
    jmp !ack

;------------------------------------------------------------------------------
; $0A ECHO_ON — p0 = EON voice mask. Mask 0 = full echo off: mute the
; echo output and stop ring writes too (audioDisableEcho); with p1 != 0,
; also forget the ring (audioReleaseEcho).
;------------------------------------------------------------------------------
cmd_echo_on:
    mov $F2, #$4D       ; EON
    mov a, ZP_P0
    mov $F3, a
    bne @eon_on
    WDSP $2C, $00       ; EVOLL = 0
    WDSP $3C, $00       ; EVOLR = 0
    WDSP $6C, $20       ; FLG: echo writes off
    mov a, #$00
    mov ZP_ECHO_LIVE, a
    mov a, ZP_P1L
    beq @eon_done       ; audioDisableEcho: the ring keeps its memory
    ; p1 != 0 (audioReleaseEcho): forget the ring, its memory goes back to
    ; the samples. The DSP reads FLG's echo-write bit at cycles 28-29 of
    ; each 32-cycle sample (anomie's S-DSP doc), so the last ring write
    ; lands within one sample of the FLG write: wait four (128 cycles)
    ; before the ack lets the CPU load into the old ring. The ring is
    ; still read (EVOL is 0, nothing is heard); ZP_EDL keeps the delay the
    ; DSP is still running, which the next ECHO_CFG waits out.
    mov a, #$00
    mov ZP_PAGES, a     ; ECHO_ON no longer restarts a ring
    mov y, #32
-   dbnz y, -           ; 32 x 4 cycles
    jmp !ack
@eon_on:
    ; A mask after a mask of 0: the ring was stopped and muted, and routing
    ; voices into it gave no echo (until 2026-10-13 only a new ECHO_CFG
    ; brought it back). Start it again as ECHO_CFG left it — cleared, since
    ; it still holds what was playing when it was stopped.
    mov a, ZP_PAGES
    beq @eon_done       ; no ring configured: EON alone, as before
    mov a, ZP_ECHO_LIVE
    bne @eon_done
    call !echo_start
@eon_done:
    jmp !ack

;------------------------------------------------------------------------------
; $0D ENVX — p0 = voice; result0 = the voice's current envelope (7-bit).
; The only DSP -> CPU read in the protocol (audioGetVoiceState.active).
;------------------------------------------------------------------------------
cmd_envx:
    mov a, ZP_P0
    xcn a
    or a, #$08          ; VxENVX
    mov $F2, a
    mov a, $F3          ; DSP register read
    mov $F5, a          ; result0
    jmp !ack

;------------------------------------------------------------------------------
; $0F LOAD_SIZE — for the NEXT LOAD transfer: p1 = number of full
; three-byte exchanges, p0 = bytes in the last partial one (0, 1 or 2)
;------------------------------------------------------------------------------
cmd_load_size:
    mov a, ZP_P1L
    mov ZP_SIZE_L, a
    mov a, ZP_P1H
    mov ZP_SIZE_H, a
    mov a, ZP_P0
    mov ZP_TAIL, a
    jmp !ack

;------------------------------------------------------------------------------
; $0C LOAD — p1 = ARAM destination. Acks the command FIRST, then receives
; the stream THREE BYTES AN EXCHANGE: the CPU puts three data bytes on
; $F5, $F6, $F7 and then the exchange's index (low byte) on $F4; the driver
; stores the three and echoes the index. LOAD_SIZE gave the number of full
; exchanges (p1) and how many bytes a last, partial one carries (p0: 0, 1
; or 2); both sides count, so the end needs no in-band marker.
; Epilogue: a held three-step handshake, see load_done.
; APU_CHECK_RESET is NOT polled inside the transfer (documented: no
; hot-swap mid-load).
;
; Until 2026-10-10 this was one byte an exchange, about 60 cycles of this
; loop a byte (1,766 master cycles a byte on examples/audio/soundboard).
; Now about 22 cycles a byte here.
;------------------------------------------------------------------------------
cmd_load:
    mov a, ZP_P1L
    mov ZP_DST_L, a
    mov ZP_LOAD_L, a
    mov a, ZP_P1H
    mov ZP_DST_H, a
    mov ZP_LOAD_H, a

    ; ack the LOAD command itself so the CPU starts streaming
    mov a, ZP_CMD
    mov $F4, a

    mov a, #$00
    mov ZP_IDX, a       ; expected exchange index (low byte)
    mov y, #$00         ; store offset relative to the ZP pointer
    ; The count of full exchanges, N, as two nested counters: X runs the
    ; low byte down, ZP_SIZE_H the passes. A low byte of 0 is 256 turns of
    ; X, so the passes are N >> 8, plus one when the low byte is not 0.
    mov a, ZP_SIZE_L
    or a, ZP_SIZE_H
    beq load_tail       ; no full exchange at all
    mov x, ZP_SIZE_L
    beq load_next
    inc ZP_SIZE_H
load_next:
-   mov a, $F4          ; wait for the CPU to present the expected index
    cmp a, ZP_IDX
    bne -
    mov a, $F5
    mov [ZP_DST_L]+y, a
    inc y
    bne +
    inc ZP_DST_H        ; next 256-byte page
+   mov a, $F6
    mov [ZP_DST_L]+y, a
    inc y
    bne +
    inc ZP_DST_H
+   mov a, $F7
    mov [ZP_DST_L]+y, a
    inc y
    bne +
    inc ZP_DST_H
+   mov a, ZP_IDX
    mov $F4, a          ; echo the index = exchange acknowledged
    inc ZP_IDX
    dec x
    bne load_next
    dec ZP_SIZE_H
    bne load_next

load_tail:
    ; the last, partial exchange: ZP_TAIL bytes (0: there is none)
    mov a, ZP_TAIL
    beq load_done
-   mov a, $F4
    cmp a, ZP_IDX
    bne -
    mov a, $F5
    mov [ZP_DST_L]+y, a
    mov a, ZP_TAIL
    cmp a, #$02
    bne +
    inc y
    bne ++
    inc ZP_DST_H
++  mov a, $F6
    mov [ZP_DST_L]+y, a
+   mov a, ZP_IDX
    mov $F4, a
    inc ZP_IDX

load_done:
    ; Epilogue: three steps in which EVERY value one side waits for is HELD
    ; by the other until it is answered. Nothing here may be shown for a
    ; few cycles only: the CPU can be away in its NMI handler for thousands
    ; of them.
    ;   a. our port still shows the echo of the last exchange, and stays so
    ;      until the CPU, having seen it, shows an end mark on its side —
    ;      $FF, or $7F when the last index was itself $FF: never an index
    ;      it could still be showing;
    ;   b. we answer with the same mark and hold it until the CPU parks its
    ;      latch at 0;
    ;   c. we show 0 ("back in command mode"), which the CPU waits for
    ;      before it sends anything else. The mark stays in ZP_LAST_CMD (no
    ;      command byte has opcode $3F).
    ; History. Until 2026-10-03 there was no mark, and a 513-byte sample
    ; (last index 0) hung both sides. Then we showed $FF and waited for the
    ; CPU's 0, which for the same sizes was ALREADY on its latch: the $FF
    ; lasted a few cycles. On the morning of 2026-10-10 the mark was held —
    ; but shown right after the last echo, which then lasted about 40 of
    ; our cycles: a CPU taken by its NMI in that window never saw the echo
    ; and timed out.
    mov a, #$FF
    mov y, ZP_IDX       ; last index + 1: zero when the last index was $FF
    bne +
    mov a, #$7F
+   mov ZP_LAST_CMD, a
-   mov a, $F4          ; a. the CPU saw the last echo
    cmp a, ZP_LAST_CMD
    bne -
    mov a, ZP_LAST_CMD
    mov $F4, a          ; b. the mark, held
-   mov a, $F4
    bne -               ;    until the CPU's latch is 0
    mov a, #$00
    mov $F4, a          ; c. "back in command mode"
    jmp !main_loop

;------------------------------------------------------------------------------
; $0B DIR_SET — p0 = sample id (0-63), p1 = loop OFFSET in bytes.
; Writes the directory entry at $0A00 + id*4 as {start, start+offset},
; start being the address of the last completed LOAD.
;------------------------------------------------------------------------------
cmd_dir_set:
    mov a, ZP_P0
    asl a
    asl a               ; id * 4
    mov y, a
    mov a, #$00
    mov ZP_DST_L, a     ; reuse the ZP pair as a $0A00 base pointer
    mov a, #$0A
    mov ZP_DST_H, a
    mov a, ZP_LOAD_L
    mov [ZP_DST_L]+y, a ; entry+0: start low
    inc y
    mov a, ZP_LOAD_H
    mov [ZP_DST_L]+y, a ; entry+1: start high
    inc y
    mov a, ZP_LOAD_L
    clrc
    adc a, ZP_P1L
    mov [ZP_DST_L]+y, a ; entry+2: loop low
    inc y               ; INC leaves the carry untouched on SPC700
    mov a, ZP_LOAD_H
    adc a, ZP_P1H
    mov [ZP_DST_L]+y, a ; entry+3: loop high
    jmp !ack

;------------------------------------------------------------------------------
; $12 LOAD_BASE — p1 = the address the next DIR_SET builds its entry from.
; A sample sent in several LOADs (audioLoadSampleStep) starts where its
; first one did, not its last.
;------------------------------------------------------------------------------
cmd_load_base:
    mov a, ZP_P1L
    mov ZP_LOAD_L, a
    mov a, ZP_P1H
    mov ZP_LOAD_H, a
    jmp !ack

;------------------------------------------------------------------------------
; Common ack: echo the full command byte on $F4 out
;------------------------------------------------------------------------------
ack:
    mov a, ZP_CMD
    mov $F4, a
    jmp !main_loop

bit_mask:
    .db $01, $02, $04, $08, $10, $20, $40, $80
