;==============================================================================
; OpenSNES - oamPlaceWorld(): a batch of world-space sprites in one call
;==============================================================================
; A scrolling game keeps its sprites in world coordinates and, every frame,
; turns each into an OAM entry: subtract the camera, decide whether it is on
; screen, write x / y / tile / attributes and the ninth x bit, or hide it.
; It is the same loop for every scrolling game (issue #165), so it is here,
; once, in assembly.
;
; void oamPlaceWorld(const OamWorldBatch *batch, u16 cam_x, u16 cam_y)
;
; Calling convention (cc65816): arguments pushed left to right; with the php
; and the phb below, cam_y is at 6,s, cam_x at 8,s, batch at 10,s (bank at
; 12,s). The phb is pulled back once the batch is read: from there on the
; camera is at 5,s (y) and 7,s (x).
;
; OamWorldBatch (sprite.h; the offsets are asserted there):
;    0 x        const s16 *   world x of each sprite's top-left corner
;    4 y        const s16 *   world y
;    8 tile     const u8 *    tile number, low byte
;   12 attr     const u8 *    attribute byte (vhoopppc)
;   16 visible  u8 *          out: 1 placed, 0 hidden; may be 0
;   20 first_id u8            OAM id of the first sprite
;   21 count    u8
;   22 size     u8            width and height in pixels
;   24 order    const u8 *    optional: slot i shows sprite order[i]; 0 = slot i
;                             shows sprite i
;
; How (devtools/placebench times it against the body it replaced, and
; .claude/notes/chantiers/gap_lots/LIB_place_prod.md has the figures):
;   - the batch is read with the data bank set to its bank (abs,x), the
;     caller's data bank back before any write: the OAM shadow and its flags
;     are plain RAM below $2000, reached through the data bank (see
;     sprite_oamset.asm);
;   - a batch of one sprite takes a straight path: the screen position
;     tested by its sign (left / top edge) and against 256 / 224 (size 0
;     first hides a sprite at screen x = 0 or y = 0, as the general rule
;     does); a batch of 2 to 8 sprites runs one sprite at a time, with the
;     same tests;
;   - any other batch is cut into groups of eight OAM entries (the eight
;     share one word of the high table). A whole group runs unrolled at fixed
;     OAM offsets; its ninth x bits are gathered in ACC and written once, as
;     a word, keeping the size bits. The first and the last group, when the
;     batch covers only part of them, run a copy that tests the end before
;     each sprite, entered at the batch's first sprite through a table, and
;     write the word under a mask that also keeps the bits of the sprites
;     outside the batch.
;   - the camera goes into the direct page offset by size - 1: one unsigned
;     compare a side (t = screen + size - 1 is under 255 + size, or 223 +
;     size); adc 1 - size then gives the screen position, its sign the ninth
;     bit. Y = 2 * sprite for the word arrays, halved for the byte arrays.
;   - x is stored first, as a word (its high byte lands on y, rewritten
;     next); y as a word (its high byte lands on tile, rewritten next).
;
; What differs from the body before 2026-11-15 (sprite.h says it too):
;   (a) the two camera arguments on the stack are left as they were (the old
;       body rewrote them as camera - size; the caller owns them either way);
;   (b) the tile and attribute bytes of a HIDDEN sprite are left untouched
;       (the old body could leave its y byte's high half in the tile byte);
;   (c) up to 30 bytes of tcc__lf are used as scratch (the old body 14; the
;       ABI gives a callee 40, compiler/ABI.md).
; Within a call, the ninth x bits of a group of eight land together after
; the group's entries (the old body wrote each right after its entry);
; oam_max_id and oam_update_flag are still set last. Nobody sees the table
; between the two: the vblank handler sends the OAM only when the main loop
; waits in WaitForVBlank(), never in the middle of a call.
;
; Measured (devtools/placebench, master cycles, SlowROM / FastROM): a call
; of one sprite 3 181 / 2 730 placed (the old body 4 372 / 3 780); a sprite
; of a large batch 939 / 814 placed, 588 / 509 hidden (1 301 / 1 121 and
; 936 / 803). The section is about 7.7 KB (the old one 530 bytes),
; SUPERFREE: the linker may put it in bank $01 or above.
;
; Every tcc__r* is the callee's to use (compiler/ABI.md): r0 x, r1 y, r2
; tile, r3 attr, r4 visible (24-bit pointers, never stepped); r5 the high-
; table word of the current group; r5h the OAM offset of the first group;
; r9 the jump into the first group; r9h the last OAM id of the batch; r10
; size; r10h first_id. tcc__lf: the defines below.
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

.DEFINE NCX   tcc__lf+0     ; size - 1 - cam_x
.DEFINE PCY   tcc__lf+2     ; cam_y + 1 - size (sbc, carry set)
.DEFINE RLIM  tcc__lf+4     ; 255 + size
.DEFINE BLIM  tcc__lf+6     ; 223 + size
.DEFINE NSZ   tcc__lf+8     ; 1 - size
.DEFINE ACC   tcc__lf+10    ; ninth bits of the group being placed
.DEFINE EG    tcc__lf+12    ; end of the whole groups (Y units: 2 a sprite; slots with order)
.DEFINE EE    tcc__lf+14    ; end of the batch (same units)
.DEFINE ORD   tcc__lf+16    ; order pointer, 4 bytes
.DEFINE GIVEN tcc__lf+20    ; count as given (an order byte must be under it)
.DEFINE SPR   tcc__lf+22    ; the sprite the slot shows
.DEFINE SLOT  tcc__lf+24
.DEFINE KEEP  tcc__lf+26    ; high-table bits the current group keeps
.DEFINE TAILM tcc__lf+28    ; ...and those past the batch's last sprite
.DEFINE XB    tcc__r5+2     ; OAM offset of the first group
.DEFINE DUFF  tcc__r9       ; 2 * (first_id & 7), then the jump into the group
.DEFINE LAST  tcc__r9+2     ; last OAM id of the batch
.DEFINE FIRST tcc__r10+2    ; first_id
.DEFINE SMALL 8             ; batches of 2 to SMALL sprites: the per-sprite loop

; One sprite of a group. K: its place in the group (0-7); ORDERED, VIS: the
; shape; CHK: test the end of the batch first (EXIT when reached). Entry: A,
; X, Y 16 bits, X = the group's OAM offset; plain: Y = 2i, carry clear
; (CHK sets it itself); ordered: SLOT = the slot. Exit: carry clear (plain)
; and Y = 2(i + 1); ordered: SLOT + 1.
.MACRO GSPR ARGS K, ORDERED, VIS, CHK, EXIT
.IF ORDERED == 1
    ldy.b SLOT
.IF CHK == 1
    cpy.b EE
    bcc +
    jmp EXIT
+
.ENDIF
    lda [ORD],y
    and.w #$00FF
    cmp.b GIVEN
    bcc +
    jmp ph_bad\@
+   sta.b SPR
    asl a                   ; (carry clear: sprite < 128)
    tay
.ELSE
.IF CHK == 1
    cpy.b EE
    bcc +
    jmp EXIT
+
.ENDIF
.ENDIF
    lda [tcc__r0],y         ; world x
    adc.b NCX               ; screen x + size - 1
    cmp.b RLIM
    bcs ph_hid\@
    adc.b NSZ               ; (C clear) screen x
    sta.w oamMemory+K*4,x   ; as a word: the high byte lands on y, rewritten below
    bmi ph_left\@
ph_y\@:
    lda [tcc__r1],y         ; world y
    sec
    sbc.b PCY               ; screen y + size - 1
    cmp.b BLIM
    bcs ph_hid\@
    adc.b NSZ               ; (C clear) screen y
    sta.w oamMemory+K*4+1,x ; as a word: the high byte lands on tile, rewritten below
.IF ORDERED == 1
    ldy.b SPR
.ELSE
    tya
    lsr a
    tay                     ; Y = i for the byte arrays
.ENDIF
    sep #$20
    .ACCU 8
    lda [tcc__r2],y
    sta.w oamMemory+K*4+2,x
    lda [tcc__r3],y
    sta.w oamMemory+K*4+3,x
.IF VIS == 1
    lda #1
    sta [tcc__r4],y
.ENDIF
    rep #$20
    .ACCU 16
.IF ORDERED == 0
    tya
    inc a
    asl a                   ; (clears the carry)
    tay                     ; Y = 2(i + 1)
.ENDIF
    bra ph_nx\@

ph_left\@:                  ; x < 0: the ninth bit
    lda.w #1<<(K*2)
    tsb.b ACC
    bra ph_y\@

ph_hid\@:
    .ACCU 16
.IF VIS == 1
.IF ORDERED == 1
    ldy.b SPR
.ELSE
    tya
    lsr a
    tay
.ENDIF
    sep #$20
    .ACCU 8
    lda #0
    sta [tcc__r4],y
    rep #$20
    .ACCU 16
.IF ORDERED == 0
    tya
    inc a
    asl a
    tay
.ENDIF
.ELSE
.IF ORDERED == 0
    iny
    iny
.ENDIF
.ENDIF
ph_bad\@:
    .ACCU 16
    lda.w #$F001            ; as oamHide(): x = 257, y = 240
    sta.w oamMemory+K*4,x
    lda.w #1<<(K*2)
    tsb.b ACC
.IF ORDERED == 0
    clc                     ; (the ordered prologue sets its own carry)
.ENDIF
ph_nx\@:
.IF ORDERED == 1
    inc.b SLOT
.ENDIF
.ENDM

; The loop test: Y (plain) or SLOT (ordered) against LIMIT; falls through,
; carry clear, while under it.
.MACRO PTEST ARGS ORDERED, LIMIT, OUT
.IF ORDERED == 1
    lda.b SLOT
    cmp.b LIMIT
.ELSE
    cpy.b LIMIT
.ENDIF
    bcc +
    jmp OUT
+
.ENDM

; The camera constants, the groups and the limits of a batch of two or more
; sprites (or of size 0). PLAIN: the units are Y's, two a sprite.
.MACRO GSETUP ARGS PLAIN
    lda.b tcc__r10          ; size
    clc
    adc.w #255
    sta.b RLIM
    adc.w #223-255          ; (C clear: size < 256)
    sta.b BLIM
    lda.w #1
    sec
    sbc.b tcc__r10
    sta.b NSZ               ; 1 - size
    clc
    adc 5,s                 ; the camera's y + 1 - size
    sta.b PCY
    lda.b NSZ
    clc
    adc 7,s                 ; the camera's x + 1 - size
    eor.w #$FFFF
    inc a
    sta.b NCX               ; size - 1 - the camera's x
    lda.b FIRST
    and.w #$0007
    asl a
    sta.b DUFF              ; 2 * the first sprite's place in its group
    tax
    lda.l ow_low,x
    ora.w #$AAAA
    sta.b KEEP              ; the size bits, and the x bits before the batch
    lda.b LAST
    inc a
    and.w #$0007
    asl a
    tax
    lda.l ow_low,x
    eor.w #$5555
    sta.b TAILM             ; the x bits after the batch, in its last group
    lda.b FIRST
    lsr a
    lsr a
    and.w #$00FE
    clc
    adc.w #oamMemory+512
    sta.b tcc__r5           ; the first group's word of the high table
    lda.b FIRST
    and.w #$00F8
    asl a
    asl a
    sta.b XB                ; the first group's OAM offset
    lda.b LAST
    sec
    sbc.b FIRST
    inc a
    tay                     ; count, clamped
    lda.w #16
    sec
    sbc.b DUFF
    lsr a
    and.w #$0007
    sta.b EG                ; the head: sprites up to the next group (0 if none)
    tya
    sec
    sbc.b EG
    and.w #$FFF8
    clc
    adc.b EG
.IF PLAIN == 1
    asl a
.ENDIF
    sta.b EG                ; end of the whole groups (unused when the batch ends in the head)
    tya
.IF PLAIN == 1
    asl a
.ENDIF
    sta.b EE
.IF PLAIN == 1
    ldy.w #0
.ELSE
    stz.b SLOT
.ENDIF
.ENDM

; A whole shape. NAME: entered after GSETUP. The first group, when the batch
; does not start on one, is entered at its first sprite (the checked copy);
; then the whole groups (the unchecked copy); then the last part (the
; checked copy, from its first place).
.MACRO PLOOP ARGS ORDERED, VIS, NAME
\3:
    ldx.b DUFF
    beq pl_a\@
    lda.l pl_tab\@,x
    sta.b DUFF
    ldx.b XB
    stz.b ACC
    jmp (DUFF)
pl_a\@:
    ldx.b XB
pl_t\@:
    PTEST ORDERED, EG, pl_tl\@
    stz.b ACC
    GSPR 0, ORDERED, VIS, 0, ph_done
    GSPR 1, ORDERED, VIS, 0, ph_done
    GSPR 2, ORDERED, VIS, 0, ph_done
    GSPR 3, ORDERED, VIS, 0, ph_done
    GSPR 4, ORDERED, VIS, 0, ph_done
    GSPR 5, ORDERED, VIS, 0, ph_done
    GSPR 6, ORDERED, VIS, 0, ph_done
    GSPR 7, ORDERED, VIS, 0, ph_done
    lda (tcc__r5)
    and.w #$AAAA            ; the size bits stay
    ora.b ACC
    sta (tcc__r5)
    inc.b tcc__r5
    inc.b tcc__r5
    txa
    clc
    adc.w #32
    tax
    jmp pl_t\@
pl_tl\@:
    PTEST ORDERED, EE, ph_done
    stz.b ACC
pl_c0\@:
    GSPR 0, ORDERED, VIS, 1, pl_cx\@
pl_c1\@:
    GSPR 1, ORDERED, VIS, 1, pl_cx\@
pl_c2\@:
    GSPR 2, ORDERED, VIS, 1, pl_cx\@
pl_c3\@:
    GSPR 3, ORDERED, VIS, 1, pl_cx\@
pl_c4\@:
    GSPR 4, ORDERED, VIS, 1, pl_cx\@
pl_c5\@:
    GSPR 5, ORDERED, VIS, 1, pl_cx\@
pl_c6\@:
    GSPR 6, ORDERED, VIS, 1, pl_cx\@
pl_c7\@:
    GSPR 7, ORDERED, VIS, 1, pl_cx\@
    lda (tcc__r5)           ; the group ends with the batch still going
    and.b KEEP
    ora.b ACC
    sta (tcc__r5)
    lda.w #$AAAA
    sta.b KEEP              ; the next groups keep the size bits only
    inc.b tcc__r5
    inc.b tcc__r5
    txa
    clc
    adc.w #32
    tax
    jmp pl_t\@
pl_cx\@:                    ; the batch ends inside this group
    lda.b KEEP
    ora.b TAILM
    and (tcc__r5)
    ora.b ACC
    sta (tcc__r5)
    jmp ph_done
pl_tab\@:
    .dw pl_c0\@, pl_c1\@, pl_c2\@, pl_c3\@, pl_c4\@, pl_c5\@, pl_c6\@, pl_c7\@
.ENDM

; A batch of 2 to SMALL sprites (size not 0): one sprite at a time, the
; camera read from the stack, the screen position tested by its sign (the
; one-sprite path's tests), the high-table byte read-modified-written as the
; old body did. Less to set up than the groups, more a sprite. Entry: X =
; first_id * 4, r5 its byte of the high table, EE the end (Y units: 2 a
; sprite; slots with order); plain: Y = 0; ordered: SLOT = 0.
.MACRO SLOOP ARGS ORDERED, VIS, NAME
\3:
sl_top\@:
.IF ORDERED == 1
    ldy.b SLOT
    lda [ORD],y
    and.w #$00FF
    cmp.b GIVEN
    bcc +
    jmp sl_bad\@
+   sta.b SPR
    asl a
    tay
.ENDIF
    lda [tcc__r0],y         ; world x
    sec
    sbc 7,s                 ; the camera's x: screen x
    bmi sl_xl\@
    cmp.w #256
    bcs sl_hid\@
    sta.w oamMemory,x       ; as a word: the high byte lands on y, rewritten below
    lda [tcc__r1],y         ; world y
    sec
    sbc 5,s                 ; the camera's y: screen y
    bmi sl_t1\@
    cmp.w #224
    bcs sl_hid\@
sl_y1\@:
    sta.w oamMemory+1,x     ; as a word: the high byte lands on tile, rewritten below
.IF ORDERED == 1
    ldy.b SPR
.ELSE
    tya
    lsr a
    tay
.ENDIF
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab+1,x
    and (tcc__r5)           ; x >= 0: the ninth bit cleared
    bra sl_tile\@
sl_t1\@:
    .ACCU 16
    clc
    adc.b tcc__r10          ; y + size
    beq sl_hid\@
    bmi sl_hid\@
    sbc.b tcc__r10          ; (C set) y
    bra sl_y1\@
sl_xl\@:
    clc
    adc.b tcc__r10          ; x + size
    beq sl_hid\@
    bmi sl_hid\@
    sbc.b tcc__r10          ; (C set) x
    sta.w oamMemory,x
    lda [tcc__r1],y
    sec
    sbc 5,s
    bmi sl_t2\@
    cmp.w #224
    bcs sl_hid\@
sl_y2\@:
    sta.w oamMemory+1,x
.IF ORDERED == 1
    ldy.b SPR
.ELSE
    tya
    lsr a
    tay
.ENDIF
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora (tcc__r5)           ; x < 0: the ninth bit set
sl_tile\@:
    sta (tcc__r5)
    lda [tcc__r2],y         ; tile
    sta.w oamMemory+2,x
    lda [tcc__r3],y         ; attr
    sta.w oamMemory+3,x
.IF VIS == 1
    lda #1
    sta [tcc__r4],y
.ENDIF
    rep #$20
    .ACCU 16
.IF ORDERED == 0
    tya
    inc a
    asl a
    tay
.ENDIF
    bra sl_next\@
sl_t2\@:
    clc
    adc.b tcc__r10
    beq sl_hid\@
    bmi sl_hid\@
    sbc.b tcc__r10
    bra sl_y2\@
sl_hid\@:
    .ACCU 16
.IF VIS == 1
.IF ORDERED == 1
    ldy.b SPR
.ELSE
    tya
    lsr a
    tay
.ENDIF
    sep #$20
    .ACCU 8
    lda #0
    sta [tcc__r4],y
    rep #$20
    .ACCU 16
.IF ORDERED == 0
    tya
    inc a
    asl a
    tay
.ENDIF
.ELSE
.IF ORDERED == 0
    iny
    iny
.ENDIF
.ENDIF
sl_bad\@:
    lda.w #$F001            ; as oamHide(): x = 257, y = 240
    sta.w oamMemory,x
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora (tcc__r5)
    sta (tcc__r5)
    rep #$20
    .ACCU 16
sl_next\@:
.IF ORDERED == 1
    inc.b SLOT
.ENDIF
    txa
    clc
    adc.w #4
    tax
    and.w #$000C
    bne +
    inc.b tcc__r5           ; four sprites to a byte of the high table
+
.IF ORDERED == 1
    lda.b SLOT
    cmp.b EE
.ELSE
    cpy.b EE
.ENDIF
    bcs +
    jmp sl_top\@
+   jmp ph_done
.ENDM

.SECTION ".text.oamPlaceWorld" SUPERFREE

    .ACCU 16
    .INDEX 16
ph_out:                     ; (before the entry: in reach of its branches)
    plb                     ; the caller's data bank (pushed by the phb)
    plp
    rtl

oamPlaceWorld:
    php
    phb
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 11,s                ; the bank byte of the batch, and the byte under it
    pha
    plb
    plb                     ; data bank: the batch's
    lda 10,s                ; batch
    tax

    lda.w 20,x              ; first_id, count
    tay
    and.w #$00FF
    cmp.w #128
    bcs ph_out              ; no sprite of that number
    sta.b FIRST
    tya
    xba
    and.w #$00FF
    beq ph_out              ; nothing to place
    sta.b GIVEN             ; count as given: the length of the caller's arrays
    adc.b FIRST             ; (C clear)
    cmp.w #129
    bcc +
    lda.w #128              ; the batch runs past sprite 127: stop there
+   dec a
    sta.b LAST

    ; the five pointers, the size and order (spelled out: a loop costs half
    ; as much again, on every call)
    lda.w 0,x
    sta.b tcc__r0
    lda.w 2,x
    sta.b tcc__r0+2
    lda.w 4,x
    sta.b tcc__r1
    lda.w 6,x
    sta.b tcc__r1+2
    lda.w 8,x
    sta.b tcc__r2
    lda.w 10,x
    sta.b tcc__r2+2
    lda.w 12,x
    sta.b tcc__r3
    lda.w 14,x
    sta.b tcc__r3+2
    lda.w 16,x
    sta.b tcc__r4
    lda.w 18,x
    sta.b tcc__r4+2
    lda.w 22,x
    and.w #$00FF
    sta.b tcc__r10          ; size
    lda.w 26,x
    sta.b ORD+2
    lda.w 24,x
    sta.b ORD
    ora.w 25,x              ; all 24 bits of order (0: none)
    tay
    plb                     ; the caller's data bank: the OAM shadow's
    tyx
    bne ph_ordered
    lda.b LAST              ; no order
    sec
    sbc.b FIRST             ; count - 1
    beq ph_one_sz
    cmp.w #SMALL
    bcs ph_plain_j
    lda.b tcc__r10
    beq ph_plain_j          ; size 0: the general path
    jmp ph_small_p
ph_plain_j:
    jmp ph_plain_n

ph_ordered:
    lda.b LAST
    sec
    sbc.b FIRST
    bne +
    lda [ORD]               ; one sprite: order byte 0, the straight path
    and.w #$00FF
    beq ph_one_sz
    lda.b tcc__r10          ; another (a batch cut at sprite 127, or a bad byte)
    beq ph_ord_j
    bra ph_small_oj
+   cmp.w #SMALL
    bcs ph_ord_j
    lda.b tcc__r10
    beq ph_ord_j            ; size 0: the general path
ph_small_oj:
    jmp ph_small_o
ph_ord_j:
    jmp ph_ord_n

ph_one_sz:
    lda.b tcc__r10
    bne ph_one
    ; size 0: the sign tests below would show a sprite at screen x = 0 or
    ; y = 0, which the general rule (0 < position + size < limit + size)
    ; hides. Those two hidden first; the rest is the straight path's.
    lda.b FIRST
    asl a
    asl a
    tax
    lda [tcc__r0]
    cmp 7,s                 ; the camera's x
    beq +
    lda [tcc__r1]
    cmp 5,s                 ; the camera's y
    bne ph_one_x
+   jmp ph_one_hid

    ;--------------------------------------------------------------------------
    ; A batch of one sprite (size 0 enters at ph_one_x, above). On screen,
    ; even partly, when -size < x < 256 and -size < y < 224.
    ;--------------------------------------------------------------------------
ph_one:
    lda.b FIRST
    asl a
    asl a
    tax                     ; X = id * 4: the entry, and its row of oam_ext_tab
ph_one_x:
    lda [tcc__r0]           ; world x
    sec
    sbc 7,s                 ; the camera's x: screen x
    bmi ph_one_xl
    cmp.w #256
    bcs ph_one_hid
    sta.w oamMemory,x       ; as a word: the high byte lands on y, rewritten below
    lda [tcc__r1]           ; world y
    sec
    sbc 5,s                 ; the camera's y: screen y
    bmi ph_one_t1
    cmp.w #224
    bcs ph_one_hid
ph_one_y1:
    sta.w oamMemory+1,x     ; as a word: the high byte lands on tile, rewritten below
    lda.b FIRST
    lsr a
    lsr a
    tay                     ; the sprite's byte of the high table
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab+1,x
    and.w oamMemory+512,y   ; x >= 0: the ninth bit cleared
    sta.w oamMemory+512,y
    bra ph_one_tile

ph_one_t1:
    .ACCU 16
    clc
    adc.b tcc__r10          ; y + size
    beq ph_one_hid
    bmi ph_one_hid
    sbc.b tcc__r10          ; (C set) y
    bra ph_one_y1

ph_one_xl:
    clc
    adc.b tcc__r10          ; x + size
    beq ph_one_hid
    bmi ph_one_hid
    sbc.b tcc__r10          ; (C set) x
    sta.w oamMemory,x
    lda [tcc__r1]           ; world y
    sec
    sbc 5,s                 ; the camera's y
    bmi ph_one_t2
    cmp.w #224
    bcs ph_one_hid
ph_one_y2:
    sta.w oamMemory+1,x
    lda.b FIRST
    lsr a
    lsr a
    tay
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora.w oamMemory+512,y   ; x < 0: the ninth bit set
    sta.w oamMemory+512,y
ph_one_tile:
    lda [tcc__r2]           ; tile
    sta.w oamMemory+2,x
    lda [tcc__r3]           ; attr
    sta.w oamMemory+3,x
    lda.b tcc__r4
    ora.b tcc__r4+1
    ora.b tcc__r4+2
    beq ph_one_done
    lda #1
    sta [tcc__r4]           ; visible[0] = 1
    bra ph_one_done

ph_one_t2:
    .ACCU 16
    clc
    adc.b tcc__r10
    beq ph_one_hid
    bmi ph_one_hid
    sbc.b tcc__r10
    bra ph_one_y2

ph_one_hid:
    .ACCU 16
    lda.w #$F001            ; as oamHide(): x = 257, y = 240
    sta.w oamMemory,x
    lda.b FIRST
    lsr a
    lsr a
    tay
    sep #$20
    .ACCU 8
    lda.l oam_ext_tab,x
    ora.w oamMemory+512,y
    sta.w oamMemory+512,y
    lda.b tcc__r4
    ora.b tcc__r4+1
    ora.b tcc__r4+2
    beq ph_one_done
    lda #0
    sta [tcc__r4]           ; visible[0] = 0
ph_one_done:
    .ACCU 8
    lda.b FIRST
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id
+   lda #1
    sta.w oam_update_flag
    plp
    rtl

ph_plain_n:
    GSETUP 1
    lda.b tcc__r4
    ora.b tcc__r4+1         ; all 24 bits of visible
    bne +
    jmp ph_pl
+   jmp ph_plvis

ph_ord_n:
    GSETUP 0
    lda.b tcc__r4
    ora.b tcc__r4+1
    bne +
    jmp ph_ord
+   jmp ph_ordvis

ph_small_p:
    jsr ph_small_set
    asl a
    sta.b EE                ; 2 * count
    ldy.w #0
    lda.b tcc__r4
    ora.b tcc__r4+1
    bne +
    jmp ph_spl
+   jmp ph_splvis

ph_small_o:
    jsr ph_small_set
    sta.b EE
    stz.b SLOT
    lda.b tcc__r4
    ora.b tcc__r4+1
    bne +
    jmp ph_sord
+   jmp ph_sordvis

    ; X = first_id * 4, r5 its byte of the high table; A = count, clamped
ph_small_set:
    lda.b FIRST
    asl a
    asl a
    tax
    lda.b FIRST
    lsr a
    lsr a
    clc
    adc.w #oamMemory+512
    sta.b tcc__r5
    lda.b LAST
    sec
    sbc.b FIRST
    inc a
    rts

ph_done:
    .ACCU 16
    ; the table changed, up to the last sprite of the batch
    sep #$20
    .ACCU 8
    lda.b LAST
    cmp.w oam_max_id
    bcc +
    sta.w oam_max_id
+   lda #1
    sta.w oam_update_flag
    plp
    rtl

    .ACCU 16
    SLOOP 0, 0, ph_spl
    SLOOP 0, 1, ph_splvis
    SLOOP 1, 0, ph_sord
    SLOOP 1, 1, ph_sordvis
    PLOOP 0, 0, ph_pl
    PLOOP 0, 1, ph_plvis
    PLOOP 1, 0, ph_ord
    PLOOP 1, 1, ph_ordvis

; the x bits of the places before k in a group's word of the high table
ow_low:
    .dw $0000, $0001, $0005, $0015, $0055, $0155, $0555, $1555

.ENDS
