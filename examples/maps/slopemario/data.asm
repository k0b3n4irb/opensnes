;==============================================================================
; Slope Mario - Graphics and Map Data
; Ported from PVSnesLib slopemario by Nub1604
;
; Single object type (Mario) with slope collision.
;==============================================================================

.section ".rodata1" superfree

tileset:
.incbin "res/tiles.pic"
tilesetend:

tilepal:
.incbin "res/tiles.pal"

mariogfx:
.incbin "res/mario_sprite.pic"
mariogfx_end:

mariopal:
.incbin "res/mario_sprite.pal"

.ends

.section ".rodata2" superfree

mapmario:
.incbin "res/BG1.m16"

objmario:
.incbin "res/map_1_1.o16"

tilesetatt:
.incbin "res/map_1_1.b16"

tilesetdef:
.incbin "res/map_1_1.t16"

.ends

;------------------------------------------------------------------------------
; objRegisterTypes — Register Mario object callback with correct bank byte
;
; cc65816 only passes 16-bit function pointers (no bank byte), but C functions
; may be placed in bank $01+ by the linker. This ASM function uses WLA-DX's
; :label syntax to resolve bank bytes at link time.
;------------------------------------------------------------------------------
.section ".objreg_text" superfree

.accu 16
.index 16
.16bit

objRegisterTypes:
    php
    phb
    phx

    sep #$20
    lda #$7e
    pha
    plb

    ; Type 0: Mario
    rep #$20
    ldx #0*4
    lda #marioinit
    sta objfctinit,x
    lda #marioupdate
    sta objfctupd,x
    lda #0
    sta objfctref,x
    sep #$20
    lda #:marioinit
    sta objfctinit+2,x
    lda #:marioupdate
    sta objfctupd+2,x
    lda #0
    sta objfctref+2,x

    plx
    plb
    plp
    rtl

.ends
