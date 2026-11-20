;==============================================================================
; OpenSNES - the tilering module's cell loops (snes/tilering.h)
;==============================================================================
; A row or a column entering the window is 29 to 33 cells, each a map read,
; a slot lookup, a count and a tilemap entry; another leaves. Compiled from
; C the cell cost about 4,500 master cycles, a diagonal step 0.8 of an NTSC
; frame. These two loops do the common case: the character already has a
; slot. tileRingCellsIn() leaves the map word of a cell whose character
; has none in its place, notes the cell's index in tr_hot_miss[] and goes
; on. tileRingLoad() then gives each of their characters a slot (once: the
; first cell that needs it), tilering.c queues the characters' bytes
; (stage), and tileRingCellsFix() finishes the cells left, the many cells
; of a sea character among them.
;
; They read their context from five globals tilering.c sets before the
; calls (one ring at a time, the duration of one tileRingUpdate()):
;   tr_hot_slot_of  u16 FAR *   the slot of each character ($FFFF: none)
;   tr_hot_refs     u8 FAR *    the count of each slot
;   tr_hot_charpal  const u8 *  the palette byte of each character
;   tr_hot_first    u16         the character number of slot 0
;   tr_hot_resident u16         slots with a count above 0, kept up to date
;   tr_hot_miss     u16[64]     the cells left over (indices, $FFFF: done)
;   tr_hot_char_of  u16 FAR *   the character of each slot ($FFFF: none)
;   tr_hot_slots, tr_hot_cursor, tr_hot_starved   u16
;   tr_hot_load_c, tr_hot_load_s  u16[64]  the characters loaded, their slots
; A pointer is 4 bytes: address, bank, an unused byte.
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

;------------------------------------------------------------------------------
; u16 tileRingCellsIn(const u16 *src, u16 stride, u16 n, u16 *dst)
;   6,s dst  10,s n  12,s stride  14,s src   (after php, phb; the last
;   argument is pushed last, nearest the stack pointer)
; For each of n map words, `stride` bytes apart from src: count the cell on
; its character's slot and write its tilemap entry to dst (consecutive
; words): flips and priority of the map word, the character's palette, the
; slot's character number. A cell whose character has no slot gets its map
; word instead, and its index in tr_hot_miss[]. Returns the number of those
; (n is 64 at most).
;------------------------------------------------------------------------------
.SECTION ".text.tileRingCellsIn" SUPERFREE

tileRingCellsIn:
    php
    phb
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 14,s                ; src
    sta.b tcc__r0
    lda 16,s                ; src (bank)
    sta.b tcc__r0h
    lda 12,s                ; stride
    sta.b tcc__r9h
    lda 10,s                ; n
    sta.b tcc__r9
    lda 6,s                 ; dst
    sta.b tcc__r4
    lda 8,s                 ; dst (bank)
    sta.b tcc__r4h
    lda.l tr_hot_slot_of
    sta.b tcc__r1
    lda.l tr_hot_slot_of+2
    sta.b tcc__r1h
    lda.l tr_hot_refs
    sta.b tcc__r2
    lda.l tr_hot_refs+2
    sta.b tcc__r2h
    lda.l tr_hot_charpal
    sta.b tcc__r3
    lda.l tr_hot_charpal+2
    sta.b tcc__r3h
    lda.l tr_hot_first
    sta.b tcc__r5h
    stz.b tcc__r5           ; slots whose count left 0
    stz.b tcc__lf+2         ; misses noted, in bytes
    ldx.w #0                ; cells done
    lda.b tcc__r9
    beq @done
@cell:
    lda [tcc__r0]           ; the map word
    sta.b tcc__r10
    and.w #$0FFF
    sta.b tcc__r10h         ; its character
    asl a
    tay
    lda [tcc__r1],y         ; its slot
    bmi @miss               ; none: tilering.c loads it
    tay
    clc
    adc.b tcc__r5h
    sta.b tcc__lf           ; the slot's character number
    sep #$20
    .ACCU 8
    lda [tcc__r2],y
    inc a
    sta [tcc__r2],y
    cmp #1
    rep #$20
    .ACCU 16
    bne +
    inc.b tcc__r5           ; a slot shown again
+   ldy.b tcc__r10h
    lda [tcc__r3],y         ; the palette byte (and the next one, dropped)
    xba
    and.w #$FF00
    ora.b tcc__lf
    sta.b tcc__lf
    lda.b tcc__r10
    and.w #$E000            ; flips and priority
    ora.b tcc__lf
@put:
    sta [tcc__r4]
    lda.b tcc__r4
    clc
    adc.w #2
    sta.b tcc__r4
    lda.b tcc__r0
    clc
    adc.b tcc__r9h
    sta.b tcc__r0
    inx
    cpx.b tcc__r9
    bcc @cell
@done:
    lda.l tr_hot_resident
    clc
    adc.b tcc__r5
    sta.l tr_hot_resident
    lda.b tcc__lf+2
    lsr a                   ; misses
    plb
    plp
    rtl
@miss:
    txa
    phx
    ldx.b tcc__lf+2
    sta.l tr_hot_miss,x
    inx
    inx
    stx.b tcc__lf+2
    plx
    lda.b tcc__r10          ; the map word, for tilering.c
    bra @put

.ENDS

;------------------------------------------------------------------------------
; void tileRingCellsOut(const u16 *src, u16 stride, u16 n)
;   6,s n  8,s stride  10,s src   (after php, phb)
; For each of n map words, `stride` bytes apart from src: take the cell off
; its character's slot. A slot whose count falls to 0 keeps its character.
;------------------------------------------------------------------------------
.SECTION ".text.tileRingCellsOut" SUPERFREE

tileRingCellsOut:
    php
    phb
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 10,s                ; src
    sta.b tcc__r0
    lda 12,s                ; src (bank)
    sta.b tcc__r0h
    lda 8,s                 ; stride
    sta.b tcc__r9h
    lda 6,s                 ; n
    sta.b tcc__r9
    lda.l tr_hot_slot_of
    sta.b tcc__r1
    lda.l tr_hot_slot_of+2
    sta.b tcc__r1h
    lda.l tr_hot_refs
    sta.b tcc__r2
    lda.l tr_hot_refs+2
    sta.b tcc__r2h
    stz.b tcc__r5           ; slots whose count fell to 0
    lda.b tcc__r9
    beq @done
@cell:
    lda [tcc__r0]
    and.w #$0FFF
    asl a
    tay
    lda [tcc__r1],y         ; the slot
    bmi @next               ; none (a character that never got one)
    tay
    sep #$20
    .ACCU 8
    lda [tcc__r2],y
    beq @next8              ; already 0
    dec a
    sta [tcc__r2],y
    bne @next8
    rep #$20
    .ACCU 16
    inc.b tcc__r5
@next8:
    rep #$20
    .ACCU 16
@next:
    lda.b tcc__r0
    clc
    adc.b tcc__r9h
    sta.b tcc__r0
    dec.b tcc__r9
    bne @cell
@done:
    lda.l tr_hot_resident
    sec
    sbc.b tcc__r5
    sta.l tr_hot_resident
    plb
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; void tileRingCellsFix(u16 *dst, u16 m)
;   6,s m  8,s dst   (after php, phb)
; The m cells tileRingCellsIn() noted in tr_hot_miss[] (an index of $FFFF
; is a cell tilering.c finished itself): dst[index] holds the map word;
; count it on its character's slot, which tilering.c has loaded since, and
; replace it with its tilemap entry. A character still without a slot (the
; ring was full) shows slot 0.
;------------------------------------------------------------------------------
.SECTION ".text.tileRingCellsFix" SUPERFREE

tileRingCellsFix:
    php
    phb
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s                 ; m
    asl a
    sta.b tcc__r9           ; end of the list, in bytes
    lda 8,s                 ; dst
    sta.b tcc__r4
    lda 10,s                ; dst (bank)
    sta.b tcc__r4h
    lda.l tr_hot_slot_of
    sta.b tcc__r1
    lda.l tr_hot_slot_of+2
    sta.b tcc__r1h
    lda.l tr_hot_refs
    sta.b tcc__r2
    lda.l tr_hot_refs+2
    sta.b tcc__r2h
    lda.l tr_hot_charpal
    sta.b tcc__r3
    lda.l tr_hot_charpal+2
    sta.b tcc__r3h
    stz.b tcc__r5           ; slots whose count left 0
    ldx.w #0
    cpx.b tcc__r9
    bcs @done
@cell:
    lda.l tr_hot_miss,x
    bmi @next               ; finished by tilering.c
    asl a
    sta.b tcc__r9h          ; the cell's offset in dst
    tay
    lda [tcc__r4],y         ; the map word
    sta.b tcc__r10
    and.w #$0FFF
    sta.b tcc__r10h
    asl a
    tay
    lda [tcc__r1],y         ; its slot
    bpl +
    lda.w #0                ; none: slot 0
    bra @entry
+   tay
    sep #$20
    .ACCU 8
    lda [tcc__r2],y
    inc a
    sta [tcc__r2],y
    cmp #1
    rep #$20
    .ACCU 16
    bne +
    inc.b tcc__r5
+   tya
@entry:
    clc
    adc.l tr_hot_first
    sta.b tcc__lf
    ldy.b tcc__r10h
    lda [tcc__r3],y         ; the palette byte
    xba
    and.w #$FF00
    ora.b tcc__lf
    sta.b tcc__lf
    lda.b tcc__r10
    and.w #$E000
    ora.b tcc__lf
    ldy.b tcc__r9h
    sta [tcc__r4],y
@next:
    inx
    inx
    cpx.b tcc__r9
    bcc @cell
@done:
    lda.l tr_hot_resident
    clc
    adc.b tcc__r5
    sta.l tr_hot_resident
    plb
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; u16 tileRingLoad(u16 *dst, u16 m)
;   6,s m  8,s dst   (after php, phb)
; For the m cells of tr_hot_miss[] whose character has no slot: take the
; first slot of count 0 from the cursor on (its old character, if any,
; loses it), give it the character with a count of 1, write the cell's
; entry and mark the cell done ($FFFF). Each character loaded and its slot
; go to tr_hot_load_c[] / tr_hot_load_s[], in order; returns how many. A
; cell whose character got a slot from an earlier cell is left to
; tileRingCellsFix(). No slot of count 0 at all: the cell shows slot 0 and
; tr_hot_starved counts it.
;------------------------------------------------------------------------------
.SECTION ".text.tileRingLoad" SUPERFREE

tileRingLoad:
    php
    phb
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 6,s                 ; m
    asl a
    sta.b tcc__r9           ; end of the list, in bytes
    lda 8,s                 ; dst
    sta.b tcc__r4
    lda 10,s                ; dst (bank)
    sta.b tcc__r4h
    lda.l tr_hot_char_of
    sta.b tcc__r0
    lda.l tr_hot_char_of+2
    sta.b tcc__r0h
    lda.l tr_hot_slot_of
    sta.b tcc__r1
    lda.l tr_hot_slot_of+2
    sta.b tcc__r1h
    lda.l tr_hot_refs
    sta.b tcc__r2
    lda.l tr_hot_refs+2
    sta.b tcc__r2h
    lda.l tr_hot_charpal
    sta.b tcc__r3
    lda.l tr_hot_charpal+2
    sta.b tcc__r3h
    lda.l tr_hot_slots
    sta.b tcc__r5h
    stz.b tcc__lf+2         ; characters loaded, in bytes
    ldx.w #0
@cell:
    cpx.b tcc__r9
    bcc +
    jmp @done
+   lda.l tr_hot_miss,x
    bpl +
    jmp @next               ; done already
+   asl a
    sta.b tcc__r9h          ; the cell's offset in dst
    tay
    lda [tcc__r4],y         ; the map word
    sta.b tcc__r10
    and.w #$0FFF
    sta.b tcc__r10h         ; its character
    asl a
    tay
    lda [tcc__r1],y
    bmi +
    jmp @next               ; loaded by an earlier cell: tileRingCellsFix()
+   phx
    lda.b tcc__r5h
    sta.b tcc__r5           ; slots left to look at
    lda.l tr_hot_cursor
    tay
@scan:
    sep #$20
    .ACCU 8
    lda [tcc__r2],y
    rep #$20                ; (REP leaves Z alone)
    .ACCU 16
    beq @found
    iny
    cpy.b tcc__r5h
    bcc +
    ldy.w #0
+   dec.b tcc__r5
    bne @scan
    ; every slot is shown: the ring is too small for this window
    plx
    lda.l tr_hot_starved
    inc a
    sta.l tr_hot_starved
    lda.w #0
    sta.b tcc__lf+4
    bra @entry
@found:
    sty.b tcc__lf+4         ; the slot
    tya
    asl a
    tay
    lda [tcc__r0],y         ; its old character
    bmi +
    asl a
    tay
    lda.w #$FFFF
    sta [tcc__r1],y         ; loses its slot
    ldy.b tcc__lf+4
    tya
    asl a
    tay
+   lda.b tcc__r10h
    sta [tcc__r0],y         ; char_of[s] = c
    asl a
    tay
    lda.b tcc__lf+4
    sta [tcc__r1],y         ; slot_of[c] = s
    tay
    sep #$20
    .ACCU 8
    lda #1
    sta [tcc__r2],y         ; one cell shows it
    rep #$20
    .ACCU 16
    iny
    cpy.b tcc__r5h
    bcc +
    ldy.w #0
+   tya
    sta.l tr_hot_cursor
    lda.l tr_hot_resident
    inc a
    sta.l tr_hot_resident
    ldx.b tcc__lf+2
    lda.b tcc__r10h
    sta.l tr_hot_load_c,x
    lda.b tcc__lf+4
    sta.l tr_hot_load_s,x
    inx
    inx
    stx.b tcc__lf+2
    plx
@entry:
    lda.b tcc__lf+4
    clc
    adc.l tr_hot_first
    sta.b tcc__lf
    ldy.b tcc__r10h
    lda [tcc__r3],y         ; the palette byte
    xba
    and.w #$FF00
    ora.b tcc__lf
    sta.b tcc__lf
    lda.b tcc__r10
    and.w #$E000
    ora.b tcc__lf
    ldy.b tcc__r9h
    sta [tcc__r4],y
    lda.w #$FFFF
    sta.l tr_hot_miss,x     ; done
@next:
    inx
    inx
    jmp @cell
@done:
    lda.b tcc__lf+2
    lsr a
    plb
    plp
    rtl

.ENDS
