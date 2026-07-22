;==============================================================================
; APU (SPC700) raw access — the IPL boot-ROM protocol
;==============================================================================
; The S-SMP boots into a 64-byte IPL ROM that speaks a handshake protocol
; over the four APU I/O ports ($2140-$2143). These routines drive it from
; the 65816 side: wait for boot, upload a binary image into APU RAM, and
; start execution. The APU-side program is assembled by wla-spc700 (see
; make/common.mk's SPCSRC stage and templates/memmap_spc700.inc).
;
; This is the modern, ABI-lint-verified path — unlike audio.asm (the
; legacy snesmod ABI), every function here follows the cc65816 calling
; convention and is checked by check_asm_abi.py.
;
; Protocol reference: krom (Peter Lemon)'s SNES_SPC700.INC, reimplemented
; for the cc65816 ABI.
;==============================================================================

.include "memmap.inc"

.SECTION ".apu_asm" SUPERFREE

;------------------------------------------------------------------------------
; void apuWaitBoot(void)
;------------------------------------------------------------------------------
; Blocks until the IPL ROM signals readiness ($AA on IO0, $BB on IO1).
; Call once after reset (consoleInit does NOT do this), and again after
; re-running the boot ROM.
;------------------------------------------------------------------------------
apuWaitBoot:
    php
    phb
    pea $0000
    plb
    plb                     ; DBR = $00 for $21xx access
    sep #$20
    .ACCU 8

    lda #$AA
@wait_aa:
    cmp.w $2140             ; IPL ready signal on IO0
    bne @wait_aa
    sta.w $2140             ; clear a possible stale $CC
    lda #$BB
@wait_bb:
    cmp.w $2141             ; second ready byte on IO1
    bne @wait_bb

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void apuUpload(const u8 *src, u16 spcAddr, u16 size)
;------------------------------------------------------------------------------
; Uploads `size` bytes from ROM/WRAM `src` (far pointer — bank honoured)
; into APU RAM at `spcAddr` via the IPL per-byte handshake: each data
; byte goes to IO1, its index low byte to IO0, and the IPL echoes the
; index back as the acknowledge.
;
; Stack after PHP + PHB (cc65816 left-to-right, post-A6 4-byte pointers):
;   1,s     = B (PHB)
;   2,s     = P (PHP)
;   3-5,s   = return address
;   6-7,s   = size      (rightmost arg, pushed last)
;   8-9,s   = spcAddr
;   10-11,s = src low 16
;   12,s    = src bank
;   13,s    = pad
;
; Clobbers tcc__r0/tcc__r0h (far-pointer staging) and tcc__r1 (size) —
; caller-saved compiler scratch, standard for lib ASM.
;------------------------------------------------------------------------------
apuUpload:
    php
    phb
    pea $0000
    plb
    plb                     ; DBR = $00
    rep #$30
    .ACCU 16
    .INDEX 16

    lda 10,s                ; src low 16
    sta.b tcc__r0           ; DP far-pointer staging (low 16)
    sep #$20
    .ACCU 8
    lda 12,s                ; src bank
    sta.b tcc__r0h          ; DP far-pointer staging (bank)
    rep #$20
    .ACCU 16
    lda 6,s                 ; size
    sta.b tcc__r1           ; loop bound (cpy has no stack-relative mode)
    lda 8,s                 ; spcAddr
    sta.w $2142             ; IO2/IO3 = target APU address (16-bit)

    sep #$20
    .ACCU 8
    lda.w $2140             ; begin-upload command: IO0 + $22
    clc
    adc #$22
    bne @cmd_ok             ; the IPL treats 0 as "no new command" —
    inc a                   ; krom: "special case fully verified"
@cmd_ok:
    sta.w $2141             ; command mirror on IO1
    sta.w $2140             ; command on IO0 starts the transfer
@wait_begin:
    cmp.w $2140             ; IPL echoes the command when ready
    bne @wait_begin

    ldy #0
@byte_loop:
    lda [tcc__r0],y         ; data byte (24-bit pointer, bank honoured)
    sta.w $2141             ; data on IO1
    tya
    sta.w $2140             ; index low byte on IO0 = "byte ready"
@wait_ack:
    cmp.w $2140             ; IPL echoes the index = acknowledged
    bne @wait_ack
    iny
    cpy.b tcc__r1           ; all bytes sent?
    bne @byte_loop

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void apuExecute(u16 spcAddr)
;------------------------------------------------------------------------------
; Ends the upload session and starts APU execution at `spcAddr`.
;
; Stack after PHP+PHB: 1,s = B, 2,s = P, 3-5,s = return address,
;   6-7,s = spcAddr
;------------------------------------------------------------------------------
apuExecute:
    php
    phb
    pea $0000
    plb
    plb                     ; DBR = $00
    rep #$20
    .ACCU 16

    lda 6,s                 ; spcAddr
    sta.w $2142             ; IO2/IO3 = entry point

    sep #$20
    .ACCU 8
    stz.w $2141             ; IO1 = 0 selects "execute" (vs upload)
    lda.w $2140
    clc
    adc #$22
    sta.w $2140             ; command
@wait_exec:
    cmp.w $2140             ; IPL acknowledges, then jumps
    bne @wait_exec
    stz.w $2140             ; park IO0 at $00 so a residual handshake byte
                            ; can never read as APU_RESET_MAGIC ($FE)

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void apuReset(void)
;------------------------------------------------------------------------------
; Cooperative APU hot-swap: ask the running APU program to return to the
; IPL boot ROM, then wait until the IPL is ready again. The APU program
; must poll CPUIO0 ($F4) for APU_RESET_MAGIC ($FE) and run the
; APU_CHECK_RESET idiom (templates/memmap_spc700.inc): silence the DSP,
; zero its output ports 0/1 (the ack we wait on below), re-enable the
; IPL ROM and jump to $FFC0. On return, upload the next program with
; apuUpload()/apuExecute() — do NOT call apuWaitBoot() again.
;
; No stack args (void). Handshake, in order:
;   1. write $FE to IO0            (the request)
;   2. wait for IO0 output == $00  (program acked and is jumping to IPL;
;                                   pre-reset IO0 holds a non-zero
;                                   command echo, so $00 is unambiguous)
;   3. wait for $AA/$BB            (IPL up, same signal as cold boot)
;------------------------------------------------------------------------------
apuReset:
    php
    phb
    pea $0000
    plb
    plb                     ; DBR = $00 for $21xx access
    sep #$20
    .ACCU 8

    lda #$FE
    sta.w $2140             ; APU_RESET_MAGIC on IO0
@wait_ack:
    lda.w $2140             ; program zeroes its output port 0 = ack
    bne @wait_ack

    lda #$AA
@wait_aa:
    cmp.w $2140             ; IPL ready signal on IO0
    bne @wait_aa
    sta.w $2140             ; clear a possible stale $CC (as apuWaitBoot)
    lda #$BB
@wait_bb:
    cmp.w $2141             ; second ready byte on IO1
    bne @wait_bb

    plb
    plp
    rtl

.ENDS
