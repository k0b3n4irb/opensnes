;==============================================================================
; string — memory and string functions (see snes/string.h)
;==============================================================================
; void FAR *memcpy (void FAR *dst, const void *src, unsigned int n)
; void FAR *memmove(void FAR *dst, const void *src, unsigned int n)
; void FAR *memset (void FAR *dst, int c, unsigned int n)
; unsigned int strlen(const char *s)
; int   strcmp (const char *a, const char *b)
; char FAR *strcpy (char FAR *dst, const char *src)
; char FAR *strncpy(char FAR *dst, const char *src, unsigned int n)
; (an int is 16 bits here: every one of these is a 2-byte argument)
;
; Every access follows the 24-bit pointer the caller passed: through it
; (`lda [tcc__r9],y`, `sta [tcc__r10],y`), or with its bank byte as the banks
; of a block move (memcpy, memmove) or as the data bank (memset). So the
; functions work on ROM, on plain RAM and on FAR RAM alike, unlike a C loop
; over a plain pointer, which reads bank $00. A count or a string never
; crosses a bank: "never" means "the object does not straddle two banks",
; which no object the linker places does.
;
; Scratch: tcc__r9 / tcc__r10 (pointers), tcc__r0 to tcc__r2, all direct
; page, so a call from the NMI callback uses the NMI page's copy; and the
; stack, which holds the block move's instruction while it runs.
; Each function is its own section: a ROM links only the ones it calls.
;
; Calling convention (compiler/ABI.md): arguments pushed left to right,
; a pointer is 4 bytes; `php` first, so the last argument is at 5,s. A
; pointer is returned low word in A, high word in tcc__retval_hi.
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
; The block move (memcpy, memmove)
;------------------------------------------------------------------------------
; From STRING_MVN_MIN bytes up, memcpy copies with MVN, and memmove's
; backward copy from STRING_MVP_MIN bytes up with MVP: one instruction, 7 CPU
; cycles a byte, against 11.5 for the word loop through [dp],y that copies
; the shorter counts.
;
; MVN takes its two banks as immediate operands, and this code is in ROM.
; So each call writes the four bytes
;     $54 (MVN) or $44 (MVP), destination bank, source bank, $6B (RTL)
; on the stack, under them a return address into the routine, under that
; the address of the four bytes minus one (bank $00), and enters them with
; RTL. MVN runs from the stack, its RTL comes back, the four bytes are
; dropped. On the stack and not at a fixed address because the CPU fetches
; the MVN instruction again for every byte it moves: an NMI callback that
; calls memcpy while the main thread is inside one would rewrite a fixed
; copy of it under the running move. An interrupt pushes below the four
; bytes, which stay as they are; the interrupted MVN resumes where it
; stopped (its progress is in A, X and Y, which the handler restores).
;
; MVN leaves the data bank register at the destination bank: the block path
; saves it (phb) and restores it.
;
; The thresholds are the measured break-even points (luna, master cycles of
; a call, the word loop against the block move, on lengths 8, 12, 14, 16, 18,
; 20): the block move wins from 12 bytes for memcpy, from 14 for memmove's
; backward copy, and memset's unrolled stores from 20.
;------------------------------------------------------------------------------
.DEFINE STRING_MVN_MIN 12
.DEFINE STRING_MVP_MIN 14
.DEFINE STRING_SET_MIN 20

;------------------------------------------------------------------------------
; void *memcpy(void *dst, const void *src, unsigned int n)
;------------------------------------------------------------------------------
; Copies n bytes forward. The areas must not overlap in a way that a forward
; copy would spoil (dst inside src..src+n): use memmove for that.
;------------------------------------------------------------------------------
.SECTION ".string_memcpy" SUPERFREE
.ACCU 16
.INDEX 16
memcpy:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 5,s                 ; n
    cmp #STRING_MVN_MIN
    bcs @block
    lsr a                   ; words, C = one byte left over
    tax                     ; (the loads and stores below leave C alone)
    lda 11,s                ; dst (low word)
    sta.b tcc__r10
    lda 13,s                ; dst (bank)
    sta.b tcc__r10+2
    lda 7,s                 ; src (low word)
    sta.b tcc__r9
    lda 9,s                 ; src (bank)
    sta.b tcc__r9+2
    ldy #0
    txa
    beq @tail
-   lda [tcc__r9],y
    sta [tcc__r10],y
    iny
    iny
    dex
    bne -
@tail:
    bcc @done
    sep #$20
    .ACCU 8
    lda [tcc__r9],y
    sta [tcc__r10],y
    rep #$20
    .ACCU 16
@done:
    lda 13,s                ; dst (bank): the return value's high word
    sta.b tcc__retval_hi
    lda 11,s                ; dst (low word)
    plp
    rtl
@block:
    phb                     ; MVN changes it
    sep #$20
    .ACCU 8
    lda #$6B                ; RTL, the last of the four bytes
    pha
    lda 11,s                ; source bank (2 bytes pushed since)
    pha
    lda 16,s                ; destination bank (3 bytes pushed since)
    pha
    lda #$54                ; MVN
    pha
    phk                     ; where the four bytes return
    pea.w @back-1
    lda #$00                ; the four bytes are in bank $00
    pha
    rep #$20
    .ACCU 16
    lda 16,s                ; source address (9 bytes pushed since)
    tax
    lda 20,s                ; destination address (9 bytes pushed since)
    tay
    tsc
    clc
    adc #4                  ; the address of the MVN byte, minus one
    pha
    lda 16,s                ; byte count (11 bytes pushed since)
    dec a                   ; MVN moves A + 1 bytes
    rtl                     ; into the four bytes: MVN, then RTL to @back
@back:
    tsc                     ; drop the four bytes
    clc
    adc #4
    tcs
    plb
    bra @done
.ENDS

;------------------------------------------------------------------------------
; void *memmove(void *dst, const void *src, unsigned int n)
;------------------------------------------------------------------------------
; memcpy when a forward copy is safe: different banks, dst below src, or dst
; at or past src + n. Otherwise copies backward, from the last byte.
;------------------------------------------------------------------------------
.SECTION ".string_memmove" SUPERFREE
.ACCU 16
.INDEX 16
memmove:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    ; forward unless same bank and src < dst < src + n
    lda 13,s                ; dst (bank)
    eor 9,s                 ; src (bank)
    and #$00FF
    bne @forward            ; different banks: no overlap
    lda 11,s                ; dst (low word)
    sec
    sbc 7,s                 ; src (low word): the distance
    bcc @forward            ; dst below src
    beq @done               ; same place: nothing to do
    cmp 5,s                 ; n
    bcs @forward            ; dst - src >= n: no overlap
    lda 5,s                 ; n
    cmp #STRING_MVP_MIN
    bcs @block
    ; backward, a word at a time: Y runs from n down to 0
    lda 11,s                ; dst (low word)
    sta.b tcc__r10
    lda 13,s                ; dst (bank)
    sta.b tcc__r10+2
    lda 7,s                 ; src (low word)
    sta.b tcc__r9
    lda 9,s                 ; src (bank)
    sta.b tcc__r9+2
    lda 5,s                 ; n
    tay
    lsr a
    tax                     ; words; C = one byte left over, copied first
    bcc @bwords
    dey
    sep #$20
    .ACCU 8
    lda [tcc__r9],y
    sta [tcc__r10],y
    rep #$20
    .ACCU 16
@bwords:
    cpx #0
    beq @done
-   dey
    dey
    lda [tcc__r9],y
    sta [tcc__r10],y
    dex
    bne -
@done:
    lda 13,s                ; dst (bank)
    sta.b tcc__retval_hi
    lda 11,s                ; dst (low word)
    plp
    rtl
@forward:
    plp
    jml memcpy              ; same arguments, same stack
@block:
    ; MVP: X and Y on the last byte of each area
    phb                     ; MVP changes it
    sep #$20
    .ACCU 8
    lda #$6B                ; RTL, the last of the four bytes
    pha
    lda 11,s                ; source bank (2 bytes pushed since)
    pha
    lda 16,s                ; destination bank (3 bytes pushed since)
    pha
    lda #$44                ; MVP
    pha
    phk                     ; where the four bytes return
    pea.w @back-1
    lda #$00                ; the four bytes are in bank $00
    pha
    rep #$20
    .ACCU 16
    lda 14,s                ; byte count (9 bytes pushed since)
    dec a
    sta.b tcc__r0           ; count - 1: MVP moves A + 1 bytes
    clc
    adc 16,s                ; source address (9 bytes pushed since)
    tax                     ; the source's last byte
    lda.b tcc__r0
    clc
    adc 20,s                ; destination address (9 bytes pushed since)
    tay                     ; the destination's last byte
    tsc
    clc
    adc #4                  ; the address of the MVP byte, minus one
    pha
    lda.b tcc__r0
    rtl                     ; into the four bytes: MVP, then RTL to @back
@back:
    tsc                     ; drop the four bytes
    clc
    adc #4
    tcs
    plb
    bra @done
.ENDS

;------------------------------------------------------------------------------
; void *memset(void *dst, int c, unsigned int n)
;------------------------------------------------------------------------------
; Under STRING_SET_MIN bytes, word stores through the long pointer (8 CPU
; cycles a byte). From there, absolute indexed stores with the data bank
; register set to the destination bank: 16 word stores a turn, 6 cycles
; each, and 18 cycles to go round: 3.6 CPU cycles a byte on a long fill. The
; first turn enters the block k stores in, k = (16 - words % 16) % 16, with X
; moved back by 2k so that its first store lands on dst. When that move
; would wrap (dst in the first 2k bytes of its bank: the indexed address
; carries into the next bank), the fill is the word loop through the long
; pointer. An odd count's last byte is stored first, through the long
; pointer.
;------------------------------------------------------------------------------
.SECTION ".string_memset" SUPERFREE
.ACCU 16
.INDEX 16
memset:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 7,s                 ; c
    and #$00FF
    sta.b tcc__r0
    xba
    ora.b tcc__r0
    sta.b tcc__r0           ; c in both bytes
    lda 5,s                 ; n
    cmp #STRING_SET_MIN
    bcs @long
    lsr a                   ; words, C = one byte more
    tax                     ; (the loads and stores below leave C alone)
    lda 9,s                 ; dst (low word)
    sta.b tcc__r10
    lda 11,s                ; dst (bank)
    sta.b tcc__r10+2
    ldy #0
    txa
    beq @tail
    lda.b tcc__r0
-   sta [tcc__r10],y
    iny
    iny
    dex
    bne -
@tail:
    bcc @done
    lda.b tcc__r0
    sep #$20
    .ACCU 8
    sta [tcc__r10],y
    rep #$20
    .ACCU 16
@done:
    lda 11,s                ; dst (bank)
    sta.b tcc__retval_hi
    lda 9,s                 ; dst (low word)
    plp
    rtl
@long:
    phb                     ; changed to the destination's bank below
    lsr a                   ; words (one at least); C = one byte more, the last
    sta.b tcc__r1
    bcc @even
    lda 10,s                ; destination address (1 byte pushed since)
    sta.b tcc__r10
    lda 12,s                ; destination bank (1 byte pushed since)
    sta.b tcc__r10+2
    lda 6,s                 ; byte count (1 byte pushed since)
    dec a
    tay
    lda.b tcc__r0
    sep #$20
    .ACCU 8
    sta [tcc__r10],y        ; the last byte
    rep #$20
    .ACCU 16
@even:
    lda.b tcc__r1
    eor #$FFFF
    inc a
    and #$000F
    sta.b tcc__r2           ; k, the stores the first turn skips
    asl a
    eor #$FFFF
    sec
    adc 10,s                ; destination address - 2k (1 byte pushed since)
    bcc @near               ; it borrows: dst is in the first 30 bytes of its bank
    tax
    sep #$20
    .ACCU 8
    lda 12,s                ; destination bank (1 byte pushed since)
    pha
    plb                     ; DBR = the destination's bank
    rep #$20
    .ACCU 16
    lda.b tcc__r2
    asl a                   ; C = 0: k < 16
    adc.b tcc__r2
    adc #(@fill - 1) & $FFFF ; 3 bytes a store; RTS adds one
    pha
    lda.b tcc__r1
    clc
    adc #15
    lsr a
    lsr a
    lsr a
    lsr a
    tay                     ; turns: words / 16, rounded up
    lda.b tcc__r0
    rts                     ; into the block, k stores in
@near:
    ; X moved back would wrap in its bank, and the index carries into the
    ; next one: the word loop through the long pointer instead
    lda 10,s                ; destination address (1 byte pushed since)
    sta.b tcc__r10
    lda 12,s                ; destination bank (1 byte pushed since)
    sta.b tcc__r10+2
    ldx.b tcc__r1           ; words
    ldy #0
    lda.b tcc__r0
-   sta [tcc__r10],y
    iny
    iny
    dex
    bne -
    bra @lend
@fill:
    sta.w $0000,x
    sta.w $0002,x
    sta.w $0004,x
    sta.w $0006,x
    sta.w $0008,x
    sta.w $000A,x
    sta.w $000C,x
    sta.w $000E,x
    sta.w $0010,x
    sta.w $0012,x
    sta.w $0014,x
    sta.w $0016,x
    sta.w $0018,x
    sta.w $001A,x
    sta.w $001C,x
    sta.w $001E,x
    txa
    clc
    adc #32
    tax
    lda.b tcc__r0
    dey
    bne @fill
@lend:
    plb
    brl @done               ; (beyond a short branch)
.ENDS

;------------------------------------------------------------------------------
; unsigned int strlen(const char *s)
;------------------------------------------------------------------------------
.SECTION ".string_strlen" SUPERFREE
.ACCU 16
.INDEX 16
strlen:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 5,s                 ; s (low word)
    sta.b tcc__r9
    lda 7,s                 ; s (bank)
    sta.b tcc__r9+2
    ldy #0
    sep #$20
    .ACCU 8
-   lda [tcc__r9],y
    beq +
    iny
    bne -
+   rep #$20
    .ACCU 16
    tya
    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; int strcmp(const char *a, const char *b)
;------------------------------------------------------------------------------
; 0 when equal; otherwise the difference of the first two bytes that
; differ, each taken as unsigned (so the sign tells which string is first).
;------------------------------------------------------------------------------
.SECTION ".string_strcmp" SUPERFREE
.ACCU 16
.INDEX 16
strcmp:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 9,s                 ; a (low word)
    sta.b tcc__r9
    lda 11,s                ; a (bank)
    sta.b tcc__r9+2
    lda 5,s                 ; b (low word)
    sta.b tcc__r10
    lda 7,s                 ; b (bank)
    sta.b tcc__r10+2
    ldy #0
    sep #$20
    .ACCU 8
-   lda [tcc__r9],y
    cmp [tcc__r10],y
    bne @differ
    cmp #0
    beq @equal
    iny
    bne -
@equal:
    rep #$20
    .ACCU 16
    lda #0
    plp
    rtl
@differ:
    .ACCU 8
    rep #$20
    .ACCU 16
    lda [tcc__r10],y
    and #$00FF
    sta.b tcc__r0
    lda [tcc__r9],y
    and #$00FF
    sec
    sbc.b tcc__r0
    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; char *strcpy(char *dst, const char *src)
;------------------------------------------------------------------------------
.SECTION ".string_strcpy" SUPERFREE
.ACCU 16
.INDEX 16
strcpy:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 9,s                 ; dst (low word)
    sta.b tcc__r10
    lda 11,s                ; dst (bank)
    sta.b tcc__r10+2
    lda 5,s                 ; src (low word)
    sta.b tcc__r9
    lda 7,s                 ; src (bank)
    sta.b tcc__r9+2
    ldy #0
    sep #$20
    .ACCU 8
-   lda [tcc__r9],y
    sta [tcc__r10],y
    beq +
    iny
    bne -
+   rep #$20
    .ACCU 16
    lda 11,s                ; dst (bank)
    sta.b tcc__retval_hi
    lda 9,s                 ; dst (low word)
    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; char *strncpy(char *dst, const char *src, unsigned int n)
;------------------------------------------------------------------------------
; Copies at most n bytes of src; if src is shorter, the rest of the n
; bytes is filled with zeros. As in C, dst is NOT terminated when src has
; n bytes or more before its end.
;------------------------------------------------------------------------------
.SECTION ".string_strncpy" SUPERFREE
.ACCU 16
.INDEX 16
strncpy:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 11,s                ; dst (low word)
    sta.b tcc__r10
    lda 13,s                ; dst (bank)
    sta.b tcc__r10+2
    lda 7,s                 ; src (low word)
    sta.b tcc__r9
    lda 9,s                 ; src (bank)
    sta.b tcc__r9+2
    ldy #0
    lda 5,s                 ; n
    tax
    beq @done
    sep #$20
    .ACCU 8
-   lda [tcc__r9],y
    sta [tcc__r10],y
    beq @pad                ; the terminator is copied: pad what is left
    iny
    dex
    bne -
    bra @out
@pad:
    iny
    dex
    beq @out
    lda #0
-   sta [tcc__r10],y
    iny
    dex
    bne -
@out:
    rep #$20
    .ACCU 16
@done:
    lda 13,s                ; dst (bank)
    sta.b tcc__retval_hi
    lda 11,s                ; dst (low word)
    plp
    rtl
.ENDS
