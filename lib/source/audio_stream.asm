;==============================================================================
; OpenSNES - audio_stream_blocks(): the CPU side of a sample upload
;==============================================================================
; audioLoadSample() (audio.c) sends LOAD_SIZE and LOAD, then calls this to
; stream the bytes; the epilogue handshake is back in C.
;
; u16 audio_stream_blocks(const u8 *data, u16 full, u16 tail)
;   data  the BRR bytes (far pointer)
;   full  number of three-byte exchanges
;   tail  bytes in a last, partial exchange: 0, 1 or 2
;   returns 0, or 1 when the driver stopped answering
;
; One exchange: three data bytes on $2141-$2143, then the exchange's index
; (low byte) on $2140; the driver stores the three and echoes the index
; (audio_driver.spc700.asm, cmd_load). The index goes last: it is what the
; driver waits for, and the data must be there when it looks.
;
; Not declared in a public header: it is half of a protocol, not an API.
;
; Calling convention (cc65816): arguments pushed left to right; with the php
; below, tail is at 5,s, full at 7,s, data at 9,s (bank at 11,s).
;
; tcc__r0 the data pointer, tcc__r1 the index, tcc__r2 the exchanges left.
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

.DEFINE AS_SPIN 20000           ; polls of one echo before giving up (as audio.c)

.SECTION ".text.audio_stream_blocks" SUPERFREE

audio_stream_blocks:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    lda 9,s
    sta.b tcc__r0
    lda 11,s
    sta.b tcc__r0+2
    stz.b tcc__r1
    ldy.w #0                    ; 16 bits: a sample is under 64 KB
    lda 7,s
    sta.b tcc__r2
    beq @tail

@exchange:
    sep #$20
    .ACCU 8
    lda [tcc__r0],y
    sta.l $002141
    iny
    lda [tcc__r0],y
    sta.l $002142
    iny
    lda [tcc__r0],y
    sta.l $002143
    iny
    lda.b tcc__r1
    sta.l $002140
    ldx.w #AS_SPIN
-   cmp.l $002140               ; the echo of the index
    beq +
    dex
    bne -
    jmp @timeout
+   inc.b tcc__r1
    rep #$20
    .ACCU 16
    dec.b tcc__r2
    bne @exchange

@tail:
    .ACCU 16
    lda 5,s
    beq @ok
    cmp.w #2
    sep #$20
    .ACCU 8
    bne +
    iny
    lda [tcc__r0],y             ; the second byte of a two-byte tail
    sta.l $002142
    dey
+   lda [tcc__r0],y
    sta.l $002141
    lda.b tcc__r1
    sta.l $002140
    ldx.w #AS_SPIN
-   cmp.l $002140
    beq @ok8
    dex
    bne -
@timeout:
    rep #$20
    .ACCU 16
    lda.w #1
    plp
    rtl
@ok8:
    rep #$20
    .ACCU 16
@ok:
    lda.w #0
    plp
    rtl

.ENDS
