;==============================================================================
; OpenSNES - a queue of VRAM uploads, drained by one routine
;==============================================================================
; A game that streams graphics during play (sprite frames, map rows and
; columns) makes several small VRAM transfers in each VBlank. Called from C
; one by one, each costs as much to set up as it does to run: a real project
; measured 2,280 master cycles for a 128-byte dmaCopyVram(), 1,024 of which
; are the transfer (issue #165). Here the transfers are noted during the
; frame — five words each, vramQueuePush() below — and sent in VBlank by
; vramQueueFlush().
;
; void vramQueueFlush(void)
;
; Call it in VBlank (right after WaitForVBlank()) or in force blank: it
; writes VRAM. No argument; A is 16-bit on return.
;
; The entries are five arrays of words in plain RAM, indexed by entry * 2
; (structure of arrays: one index register serves them all).
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

.EQU VRAM_QUEUE_MAX 32          ; vramqueue.h says the same

.RAMSECTION ".vram_queue" BANK 0 SLOT 1
    vram_queue_src      dsb VRAM_QUEUE_MAX*2    ; source address
    vram_queue_bank     dsb VRAM_QUEUE_MAX*2    ; its bank (low byte)
    vram_queue_addr     dsb VRAM_QUEUE_MAX*2    ; VRAM word address
    vram_queue_size     dsb VRAM_QUEUE_MAX*2    ; bytes
    vram_queue_step     dsb VRAM_QUEUE_MAX*2    ; VMAIN (low byte)
    vram_queue_count    dsb 2                   ; entries waiting
.ENDS

.SECTION ".text.vramQueueFlush" SUPERFREE

vramQueueFlush:
    php
    phb
    pea.w $0000
    plb
    plb                     ; bank $00: the registers and the queue, both by `.w`
    rep #$30
    .ACCU 16
    .INDEX 16
    lda.w vram_queue_count
    beq @out
    cmp.w #VRAM_QUEUE_MAX+1
    bcc +
    lda.w #VRAM_QUEUE_MAX   ; a count nobody could have pushed: do not run past the arrays
+   asl a
    sta.b tcc__r9           ; end of the queue, in bytes
    sep #$20
    .ACCU 8
    lda #$01
    sta.w $4300             ; channel 0: two registers, a word at a time
    lda #$18
    sta.w $4301             ; to VMDATAL
    ldx.w #0
@entry:
    rep #$20
    .ACCU 16
    lda.w vram_queue_size,x
    beq @next               ; a transfer of 0 bytes is 65,536 to the DMA: skip it
    sta.w $4305
    lda.w vram_queue_addr,x
    sta.w $2116
    lda.w vram_queue_src,x
    sta.w $4302
    sep #$20
    .ACCU 8
    lda.w vram_queue_step,x
    sta.w $2115
    lda.w vram_queue_bank,x
    sta.w $4304
    lda #$01
    sta.w $420B             ; go (this byte only: HDMAEN is not touched)
@next:
    inx
    inx
    cpx.b tcc__r9
    bcc @entry
    sep #$20
    .ACCU 8
    lda #$80
    sta.w $2115             ; VMAIN as the rest of the library leaves it
    rep #$20
    .ACCU 16
    stz.w vram_queue_count
@out:
    plb
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; u16 vramQueuePush(const u8 *src, u16 addr, u16 size, u16 step)
;   12,s src (bank)   10,s src   8,s addr   6,s size   4,s step
; Returns 1, or 0 when the queue is full (nothing is noted).
;
; A function and not a macro: compiled, the five indexed stores of a macro
; reload the count and rebuild the index five times, and cost three times
; this call (measured, devtools/libbench).
;------------------------------------------------------------------------------
.SECTION ".text.vramQueuePush" SUPERFREE

vramQueuePush:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda.w vram_queue_count
    cmp.w #VRAM_QUEUE_MAX
    bcc +
    lda.w #0
    rtl
+   asl a
    tax
    lda 10,s            ; src
    sta.w vram_queue_src,x
    lda 12,s            ; src (bank)
    sta.w vram_queue_bank,x
    lda 8,s             ; addr
    sta.w vram_queue_addr,x
    lda 6,s             ; size
    sta.w vram_queue_size,x
    lda 4,s             ; step
    sta.w vram_queue_step,x
    inc.w vram_queue_count
    lda.w #1
    rtl

.ENDS

;------------------------------------------------------------------------------
; u16 vramQueuePushSprite(const u8 *src, u16 addr, u16 size_px)
;   10,s src (bank)   8,s src   6,s addr   4,s size_px
; Returns 1, or 0 when the strips do not all fit (nothing is noted) or
; size_px is under 8.
;
; One frame of a streamed sprite in one call: size_px / 8 strips of
; size_px * 4 bytes, 512 bytes apart in the sheet and 256 words apart in
; VRAM — the layout `opensnes-sprite sheet` writes (a 128-pixel-wide raster:
; 16 tiles a row, 32 bytes a tile at 4 bpp) and OBJ VRAM expects. Four calls
; with a far pointer each cost a real project 21,000 master cycles a frame
; for 4.5 frames of 32x32 (issue #165).
;------------------------------------------------------------------------------
.SECTION ".text.vramQueuePushSprite" SUPERFREE

vramQueuePushSprite:
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 4,s             ; size_px
    lsr a
    lsr a
    lsr a
    beq @no             ; under 8 pixels: no strip
    sta.b tcc__r9       ; strips left
    clc
    adc.w vram_queue_count
    cmp.w #VRAM_QUEUE_MAX+1
    bcc +
@no:
    lda.w #0
    rtl
+   lda 4,s             ; size_px
    asl a
    asl a
    sta.b tcc__r9+2     ; bytes in a strip
    lda 8,s             ; src
    sta.b tcc__r10
    lda 6,s             ; addr
    sta.b tcc__r10+2
    lda.w vram_queue_count
    asl a
    tax
@strip:
    lda.b tcc__r10
    sta.w vram_queue_src,x
    clc
    adc.w #512          ; the next row of tiles in the sheet
    sta.b tcc__r10
    lda 10,s            ; src (bank)
    sta.w vram_queue_bank,x
    lda.b tcc__r10+2
    sta.w vram_queue_addr,x
    clc
    adc.w #256          ; the next row of tiles in VRAM
    sta.b tcc__r10+2
    lda.b tcc__r9+2
    sta.w vram_queue_size,x
    lda.w #$0080        ; VRAM_QUEUE_ROW
    sta.w vram_queue_step,x
    inx
    inx
    inc.w vram_queue_count
    dec.b tcc__r9
    bne @strip
    lda.w #1
    rtl

.ENDS

