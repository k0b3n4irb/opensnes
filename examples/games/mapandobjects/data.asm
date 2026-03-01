.section ".rodata1" superfree

tileset:
.incbin "tilesMario.pic"
tilesetend:

tilesetpal:
.incbin "tilesMario.pal"

mapmario:
.incbin "BG1.m16"

objmario:
.incbin "tiledMario.o16"

tilesetatt:
.incbin "tiledMario.b16"

tilesetdef:
.incbin "tiledMario.t16"

.ends

.section ".rodata2" superfree

sprmario:
.incbin "mario.pic"

sprgoomba:
.incbin "goomba.pic"

sprkoopatroopa:
.incbin "koopatroopa.pic"

palsprite:
.incbin "mario.pal"

.ends

;------------------------------------------------------------------------------
; objRegisterTypes — Register object type callbacks with correct bank bytes
;
; cproc only passes 16-bit function pointers (no bank byte), but C functions
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

    ; Type 1: Goomba
    rep #$20
    ldx #1*4
    lda #goombainit
    sta objfctinit,x
    lda #goombaupdate
    sta objfctupd,x
    lda #0
    sta objfctref,x
    sep #$20
    lda #:goombainit
    sta objfctinit+2,x
    lda #:goombaupdate
    sta objfctupd+2,x
    lda #0
    sta objfctref+2,x

    ; Type 2: Koopa Troopa
    rep #$20
    ldx #2*4
    lda #koopatroopainit
    sta objfctinit,x
    lda #koopatroopaupdate
    sta objfctupd,x
    lda #0
    sta objfctref,x
    sep #$20
    lda #:koopatroopainit
    sta objfctinit+2,x
    lda #:koopatroopaupdate
    sta objfctupd+2,x
    lda #0
    sta objfctref+2,x

    plx
    plb
    plp
    rtl

.ends
