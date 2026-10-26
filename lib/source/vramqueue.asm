;==============================================================================
; OpenSNES - a queue of VRAM uploads, drained by one routine
;==============================================================================
; A game that streams graphics during play (sprite frames, map rows and
; columns) makes several small VRAM transfers in each VBlank. Called from C
; one by one, each costs about as much to set up as it does to run (issue
; #165). Here the transfers are noted during the
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

.EQU VRAM_QUEUE_ENTRY_COST 80   ; vramqueue.h says the same

; The arrays, vram_queue_count, vram_queue_cost and the capacity word
; vram_queue_max are the project's (templates/vramqueue_ram.asm, at the
; size VRAM_QUEUE_MAX of its Makefile, 32 by default): the library is
; built once for every project and reads the capacity at run time.

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
    cmp.l vram_queue_max
    beq +
    bcc +
    lda.l vram_queue_max    ; a count nobody could have pushed: do not run past the arrays
+   asl a
    sta.b tcc__r9           ; end of the queue, in bytes
    sep #$20
    .ACCU 8
    lda #$01
    sta.w $4300             ; channel 0: two registers, a word at a time
    lda #$18
    sta.w $4301             ; to VMDATAL
    ldx.w #0
    ; A 8-bit, X/Y 16-bit for the whole loop: the three word registers go
    ; through Y, so no entry pays a rep/sep (619 master cycles an entry
    ; beyond its bytes, 670 with the switches; 2026-10-26)
@entry:
    ldy.w vram_queue_size,x
    beq @next               ; a transfer of 0 bytes is 65,536 to the DMA: skip it
    sty.w $4305
    ldy.w vram_queue_addr,x
    sty.w $2116
    ldy.w vram_queue_src,x
    sty.w $4302
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
    lda #$80
    sta.w $2115             ; VMAIN as the rest of the library leaves it
    rep #$20
    .ACCU 16
    stz.w vram_queue_count
@out:
    stz.w vram_queue_cost   ; also when the count was emptied by hand
    plb
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; u16 vramQueueFlushUntil(u16 line)
;   6,s line (after php, phb)
; Returns the entries left in the queue.
;
; Sends, in order, the entries that end before scanline `line` begins, and
; moves the others to the head of the queue, in order, for the next flush.
; The vertical counter is read once: the lines left, at 1324 master cycles
; each (DRAM refresh taken out), make a budget in the units of
; vram_queue_cost (bytes; one byte is 8 master cycles of DMA), from which
; VRAM_QUEUE_UNTIL_COST pays for this routine up to its first DMA. The cut
; is found by halving over vram_queue_cum, so its cost does not grow with
; the queue. Outside the vertical blank (HVBJOY bit 7 clear) nothing is sent.
; Reading the counter clears the H/V latch flag (STAT78) and latches anew
; (SLHV, with WRIO bit 7 set, as crt0 leaves it): snesdev-wiki "PPU
; registers", OPVCT and STAT78 (corpus chunk 7a230e740aeb9460).
;------------------------------------------------------------------------------
.EQU VRAM_QUEUE_UNTIL_COST 64   ; bytes: the counter, the budget, the search

.SECTION ".text.vramQueueFlushUntil" SUPERFREE

vramQueueFlushUntil:
    php
    phb
    pea.w $0000
    plb
    plb                     ; bank $00: the registers and the queue, both by `.w`
    rep #$30
    .ACCU 16
    .INDEX 16
    lda.w vram_queue_count
    bne +
    jmp @done               ; nothing queued: 0 left
+   cmp.l vram_queue_max
    beq +
    bcc +
    lda.l vram_queue_max    ; a count nobody could have pushed: do not run past the arrays
+   sta.b tcc__r10+2        ; entries queued
    sep #$20
    .ACCU 8
    lda.w $4212
    bpl @none               ; HVBJOY: not in the vertical blank, send nothing
    lda.w $213F             ; STAT78: latch flag clear, OPVCT reads its low byte next
    lda.w $2137             ; SLHV: latch the counters
    lda.w $213D             ; OPVCT low
    xba
    lda.w $213D             ; OPVCT high: bit 8, the rest open bus
    and #$01
    xba
    rep #$20
    .ACCU 16
    inc a                   ; the line under way is not counted
    sta.b tcc__r9+2
    lda 6,s                 ; line
    sec
    sbc.b tcc__r9+2
    bcc @none               ; `line` already begun
    beq @none
    sta.b tcc__r9+2         ; n lines; 1324 / 8 = 165 bytes each: 165 n = 33 (5 n)
    asl a
    asl a
    clc
    adc.b tcc__r9+2         ; 5 n
    sta.b tcc__r9+2
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc.b tcc__r9+2         ; 165 n (n <= 312: no carry)
    sec
    sbc.w #VRAM_QUEUE_UNTIL_COST
    bcc @none
    sta.b tcc__r9+2         ; the budget, in bytes
    ; the cut: the largest k with cum[k - 1] <= budget
    lda.b tcc__r10+2
    asl a
    tax
    lda.w vram_queue_cum-2,x
    cmp.b tcc__r9+2
    bcc @all
    beq @all                ; everything fits: the usual case
    stz.b tcc__r10          ; lo = 0 fits; hi = count - 1 is the most that may
    lda.b tcc__r10+2
    dec a
    sta.b tcc__r5
@half:
    lda.b tcc__r10
    cmp.b tcc__r5
    bcs @cut                ; lo = hi
    clc
    adc.b tcc__r5
    inc a
    lsr a                   ; mid = (lo + hi + 1) / 2, at least 1
    tay
    asl a
    tax
    lda.w vram_queue_cum-2,x
    cmp.b tcc__r9+2
    beq +
    bcs @over
+   sty.b tcc__r10          ; cum[mid - 1] fits: lo = mid
    bra @half
@over:
    dey
    sty.b tcc__r5          ; it does not: hi = mid - 1
    bra @half
@none:
    rep #$20
    .ACCU 16
    stz.b tcc__r10          ; k = 0
    bra @cut
@all:
    lda.b tcc__r10+2
    sta.b tcc__r10          ; k = count
@cut:
    lda.b tcc__r10
    beq @keep               ; nothing fits
    asl a
    sta.b tcc__r9           ; end of what is sent, in bytes
    sep #$20
    .ACCU 8
    lda #$01
    sta.w $4300             ; channel 0: two registers, a word at a time
    lda #$18
    sta.w $4301             ; to VMDATAL
    ldx.w #0
@entry:                     ; vramQueueFlush's loop
    ldy.w vram_queue_size,x
    beq @next
    sty.w $4305
    ldy.w vram_queue_addr,x
    sty.w $2116
    ldy.w vram_queue_src,x
    sty.w $4302
    lda.w vram_queue_step,x
    sta.w $2115
    lda.w vram_queue_bank,x
    sta.w $4304
    lda #$01
    sta.w $420B
@next:
    inx
    inx
    cpx.b tcc__r9
    bcc @entry
    lda #$80
    sta.w $2115             ; VMAIN as the rest of the library leaves it
    rep #$20
    .ACCU 16
@keep:
    ; What did not fit moves to the head, in order. This is RAM work and
    ; needs no blank: the time is the game's, after the VBlank's.
    lda.b tcc__r10+2
    sec
    sbc.b tcc__r10
    sta.w vram_queue_count  ; entries left
    bne +
    stz.w vram_queue_cost   ; all sent
    brl @done
+   ldx.b tcc__r10
    bne +
    brl @done               ; nothing sent: the queue is as it was
+
    asl a
    dec a
    sta.b tcc__r5          ; bytes to move - 1, as MVN counts
    lda.b tcc__r10
    asl a
    sta.b tcc__r9           ; 2 k
    tax
    lda.w vram_queue_cum-2,x
    sta.b tcc__r9+2         ; the cost of what was sent
    phb
.MACRO VRAM_QUEUE_MOVE ARGS ARR
    lda.w #ARR
    clc
    adc.b tcc__r9
    tax                     ; from entry k
    ldy.w #ARR              ; to entry 0
    lda.b tcc__r5
    mvn $00,$00
.ENDM
    VRAM_QUEUE_MOVE vram_queue_src
    VRAM_QUEUE_MOVE vram_queue_bank
    VRAM_QUEUE_MOVE vram_queue_addr
    VRAM_QUEUE_MOVE vram_queue_size
    VRAM_QUEUE_MOVE vram_queue_step
    VRAM_QUEUE_MOVE vram_queue_cum
    plb
    ; the costs left are counted from the new head
    ldx.w #0
@cum:
    lda.w vram_queue_cum,x
    cmp.w #$FFFF
    beq +                   ; saturated stays saturated
    sec
    sbc.b tcc__r9+2
    sta.w vram_queue_cum,x
+   inx
    inx
    cpx.b tcc__r5
    bcc @cum
    lda.w vram_queue_cost
    cmp.w #$FFFF
    beq @done
    sec
    sbc.b tcc__r9+2
    sta.w vram_queue_cost
@done:
    rep #$20
    .ACCU 16
    lda.w vram_queue_count
    plb
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; u16 vramQueuePush(const u8 *src, u16 addr, u16 size, u16 step)
;   12,s src (bank)   10,s src   8,s addr   6,s size   4,s step
; Returns 1, or 0 when the queue is full (nothing is noted).
; Adds size + VRAM_QUEUE_ENTRY_COST to vram_queue_cost, and notes the new
; total in vram_queue_cum for the entry.
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
    cmp.l vram_queue_max
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
    clc
    adc.w #VRAM_QUEUE_ENTRY_COST
    bcs @sat
    adc.w vram_queue_cost
    bcc +
@sat:
    lda.w #$FFFF        ; saturated, never back through zero
+   sta.w vram_queue_cost
    sta.w vram_queue_cum,x  ; the cost up to this entry, for vramQueueFlushUntil
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
; 16 tiles a row, 32 bytes a tile at 4 bpp) and OBJ VRAM expects (issue
; #165).
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
    cmp.l vram_queue_max
    beq +
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
    clc
    adc.w #VRAM_QUEUE_ENTRY_COST    ; 352 at most: no carry
    adc.w vram_queue_cost
    bcc +
    lda.w #$FFFF        ; saturated, never back through zero
+   sta.w vram_queue_cost
    sta.w vram_queue_cum,x  ; the cost up to this strip
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

