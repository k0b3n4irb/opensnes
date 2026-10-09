;==============================================================================
; OpenSNES - oamMetaDrawDyn() in assembly
;==============================================================================
; Walks a MetaspriteItem array, fills one oambuffer entry per item and calls
; the dynamic draw routine of the item's pixel size. Compiled C until
; 2026-10-09: devtools/twinbench measured the dynamic metasprite example at
; 1.75 times the frame cost of PVSnesLib's twin, whose iterator is assembly.
;
; void oamMetaDrawDyn(u16 id, s16 x, s16 y, const MetaspriteItem *meta,
;                     const u8 *gfxptr, u8 size_class)
;
; Calling convention (cc65816): arguments pushed left to right; with the php
; below, size_class is at 5,s, gfxptr at 7,s (bank at 9,s), meta at 11,s
; (bank at 13,s), y at 15,s, x at 17,s, id at 19,s.
;
; Scratch: tcc__r9 (the item pointer), tcc__r10 (attribute bits and refresh
; flag), tcc__r10+2 (pixel size). oamDynamic{8,16,32}Draw use no tcc__
; register and save X and Y.
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

.EQU OMDD_OAMX       0
.EQU OMDD_OAMY       2
.EQU OMDD_FRAMEID    4
.EQU OMDD_ATTRIBUTE  6           ; attribute, then the refresh flag at 7
.EQU OMDD_GRAPHICS   8
.EQU OMDD_END        $FF80       ; metasprite_end (-128) in dx

; Pixel size of the small and of the large sprite for each OBJ_SIZE* mode
; (0-5), as mode_small_size() / mode_large_size() return them.
.SECTION ".rodata.omdd_size" SEMISUPERFREE BANKS 7-1
omdd_small:
    .db 8, 8, 8, 16, 16, 32
omdd_large:
    .db 16, 32, 64, 32, 64, 64
    .db 0                       ; a 16-bit read of the last entry stays inside
.ENDS

.SECTION ".text.oamMetaDrawDyn" SUPERFREE

oamMetaDrawDyn:
    php
    rep #$30
    .ACCU 16
    .INDEX 16

    lda 19,s            ; id
    cmp.w #128
    bcc +
    plp                 ; no sprite of that number: nothing to draw
    rtl
+   asl a
    asl a
    asl a
    asl a
    tax
    ; every item takes the refresh flag the FIRST entry had on entry
    lda.l oambuffer+OMDD_ATTRIBUTE,x
    and.w #$FF00        ; refresh in the high byte; attribute bits: none yet
    sta.b tcc__r10

    lda.l oam_dynamic_size_mode
    and.w #$00FF
    cmp.w #6
    bcc +
    lda.w #5
+   tax
    lda 5,s             ; size_class
    and.w #$00FF
    bne @large
    inc.b tcc__r10      ; OBJ_SMALL: the small sprites use the second name table
    lda.l omdd_small,x
    bra @sized
@large:
    lda.l omdd_large,x
@sized:
    and.w #$00FF
    sta.b tcc__r10+2    ; pixel size: 8, 16, 32, or 64 (not drawn)

    lda 11,s            ; meta
    sta.b tcc__r9
    lda 13,s            ; meta (bank)
    sta.b tcc__r9+2

@item:
    lda [tcc__r9]       ; dx
    cmp.w #OMDD_END
    beq @done
    lda 19,s            ; id
    cmp.w #128
    bcs @done           ; the metasprite runs past sprite 127: stop there
    asl a
    asl a
    asl a
    asl a
    tax

    lda [tcc__r9]       ; dx
    clc
    adc 17,s            ; x
    sta.l oambuffer+OMDD_OAMX,x
    ldy.w #2
    lda [tcc__r9],y     ; dy
    clc
    adc 15,s            ; y
    sta.l oambuffer+OMDD_OAMY,x
    ldy.w #4
    lda [tcc__r9],y     ; tile
    sta.l oambuffer+OMDD_FRAMEID,x
    ldy.w #6
    lda [tcc__r9],y     ; attr (the byte after it is padding)
    and.w #$00FF
    ora.b tcc__r10
    sta.l oambuffer+OMDD_ATTRIBUTE,x    ; attribute and refresh flag
    lda 7,s             ; gfxptr
    sta.l oambuffer+OMDD_GRAPHICS,x
    sep #$20
    .ACCU 8
    lda 9,s             ; gfxptr (bank)
    sta.l oambuffer+OMDD_GRAPHICS+2,x
    rep #$20
    .ACCU 16

    lda 19,s            ; id
    pha
    lda.b tcc__r10+2
    cmp.w #16
    beq @draw16
    bcc @draw8
    cmp.w #32
    bne @drawn          ; 64x64: no dynamic routine, the entry is set and left
    jsl oamDynamic32Draw
    bra @drawn
@draw16:
    jsl oamDynamic16Draw
    bra @drawn
@draw8:
    jsl oamDynamic8Draw
@drawn:
    pla

    lda.b tcc__r9
    clc
    adc.w #8            ; sizeof(MetaspriteItem)
    sta.b tcc__r9
    lda 19,s            ; id
    inc a
    sta 19,s
    jmp @item

@done:
    plp
    rtl

.ENDS
