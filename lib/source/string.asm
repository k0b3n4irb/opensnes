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
; Every access goes through the 24-bit pointer the caller passed
; (`lda [tcc__r9],y`, `sta [tcc__r10],y`), so the functions work on ROM,
; on plain RAM and on FAR RAM alike — unlike a C loop over a plain pointer,
; which reads bank $00. A count or a string never crosses a bank: Y is the
; 16-bit offset from the pointer, and `[dp],y` does carry into the bank
; byte, so "never" means "the object does not straddle two banks", which no
; object the linker places does.
;
; Scratch: tcc__r9 / tcc__r10 (pointers), tcc__r0 / tcc__r1, all direct
; page — so a call from the NMI callback uses the NMI page's copy.
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
; void *memcpy(void *dst, const void *src, unsigned int n)
;------------------------------------------------------------------------------
; Copies n bytes forward, a word at a time. The areas must not overlap in
; a way that a forward copy would spoil (dst inside src..src+n): use
; memmove for that.
;------------------------------------------------------------------------------
.SECTION ".string_memcpy" SUPERFREE
.ACCU 16
.INDEX 16
memcpy:
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
    lsr a                   ; words in A, C = one byte left over
    tax                     ; (tax, the loads and stores below leave C alone)
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
.ENDS

;------------------------------------------------------------------------------
; void *memmove(void *dst, const void *src, unsigned int n)
;------------------------------------------------------------------------------
; As memcpy when a forward copy is safe: different banks, dst below src,
; or dst at or past src + n. Otherwise copies backward, from the last byte.
;------------------------------------------------------------------------------
.SECTION ".string_memmove" SUPERFREE
.ACCU 16
.INDEX 16
memmove:
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
    ; forward unless same bank and src < dst < src + n
    lda.b tcc__r10+2
    eor.b tcc__r9+2
    and #$00FF
    bne @forward            ; different banks: no overlap
    lda.b tcc__r10
    sec
    sbc.b tcc__r9           ; dst - src
    bcc @forward            ; dst below src
    beq @done               ; same place: nothing to do
    cmp 5,s                 ; n
    bcs @forward            ; dst - src >= n: no overlap
    ; backward: Y runs from n down to 0
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
    bra @done
@forward:
    ldy #0
    lda 5,s                 ; n
    lsr a
    tax
    beq @ftail
-   lda [tcc__r9],y
    sta [tcc__r10],y
    iny
    iny
    dex
    bne -
@ftail:
    bcc @done
    sep #$20
    .ACCU 8
    lda [tcc__r9],y
    sta [tcc__r10],y
    rep #$20
    .ACCU 16
@done:
    lda 13,s                ; dst (bank)
    sta.b tcc__retval_hi
    lda 11,s                ; dst (low word)
    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; void *memset(void *dst, int c, unsigned int n)
;------------------------------------------------------------------------------
.SECTION ".string_memset" SUPERFREE
.ACCU 16
.INDEX 16
memset:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 9,s                 ; dst (low word)
    sta.b tcc__r10
    lda 11,s                ; dst (bank)
    sta.b tcc__r10+2
    lda 7,s                 ; c
    and #$00FF
    sta.b tcc__r0
    xba
    ora.b tcc__r0
    sta.b tcc__r0           ; c in both bytes
    ldy #0
    lda 5,s                 ; n
    lsr a
    tax
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
