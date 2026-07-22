; Memory map for SPC700 (APU) code assembly
; Used by wla-spc700 to assemble APU-side programs into flat binaries
; (two-stage build like the GSU: wla-spc700 -> wlalink -b -> .incbin).
; The image is uploaded to APU RAM by the lib's apuUpload(); addresses
; inside the program are absolute APU RAM addresses — the .ORG below
; positions code relative to the upload base ($0200, mirroring krom's
; SPCRAM convention). One 64KB bank covers the whole APU address space.

.MEMORYMAP
    SLOTSIZE $10000
    DEFAULTSLOT 0
    SLOT 0 START $0000 SIZE $10000
.ENDME

.ROMBANKSIZE $10000
.ROMBANKS 1

; Cooperative hot-swap protocol — the SPC700 half of the lib's
; apuReset(). The 65816 writes APU_RESET_MAGIC to CPUIO0; a program
; that wants to be swappable polls for it inside its wait loops (the
; check is 3 instructions on the not-taken path — cheap enough for a
; per-millisecond timer loop). On match: silence the DSP, zero output
; ports 0/1 (the ack apuReset() waits on), re-enable the IPL ROM with
; input latches cleared, and jump back to the boot ROM at $FFC0 —
; ready for the next apuUpload()/apuExecute().
.DEFINE APU_RESET_MAGIC $FE

.MACRO APU_CHECK_RESET
    mov a, $F4              ; CPUIO0: reset requested?
    cmp a, #APU_RESET_MAGIC
    bne _no_reset\@
    mov $F2, #$5C           ; DSP KOFF: release all voices
    mov $F3, #$FF
    mov $F2, #$6C           ; DSP FLG: mute + disable echo writes
    mov $F3, #$60
    mov a, #$00
    mov $F4, a              ; output ports 0/1 = 0: ack to apuReset()
    mov $F5, a
    mov $F1, #$B0           ; CONTROL: IPL ROM on, input latches cleared,
                            ; timers off
    jmp !$FFC0              ; back to the IPL boot ROM
_no_reset\@:
.ENDM
