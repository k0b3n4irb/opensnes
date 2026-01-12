;==============================================================================
; Data section - BRR samples
;==============================================================================

.SECTION ".rodata.tada" SUPERFREE
soundbrr:
    .INCBIN "tada.brr"
soundbrrend:
.ENDS

;==============================================================================
; Font data - minimal 8x8 2bpp font for "TADA"
;==============================================================================

.SECTION ".rodata.font" SUPERFREE

; Tile 0: Blank space (16 bytes)
font_data:
    .db $00,$00, $00,$00, $00,$00, $00,$00
    .db $00,$00, $00,$00, $00,$00, $00,$00

; Tile 1: Letter T
    .db $FF,$FF, $FF,$FF, $18,$18, $18,$18
    .db $18,$18, $18,$18, $18,$18, $18,$18

; Tile 2: Letter A
    .db $18,$18, $3C,$3C, $66,$66, $C3,$C3
    .db $FF,$FF, $C3,$C3, $C3,$C3, $C3,$C3

; Tile 3: Letter D
    .db $FC,$FC, $C6,$C6, $C3,$C3, $C3,$C3
    .db $C3,$C3, $C3,$C3, $C6,$C6, $FC,$FC

; Tile 4: Letter P
    .db $FC,$FC, $C6,$C6, $C6,$C6, $FC,$FC
    .db $C0,$C0, $C0,$C0, $C0,$C0, $C0,$C0

; Tile 5: Letter R
    .db $FC,$FC, $C6,$C6, $C6,$C6, $FC,$FC
    .db $CC,$CC, $C6,$C6, $C6,$C6, $C3,$C3

; Tile 6: Letter E
    .db $FE,$FE, $C0,$C0, $C0,$C0, $F8,$F8
    .db $C0,$C0, $C0,$C0, $C0,$C0, $FE,$FE

; Tile 7: Letter S
    .db $7E,$7E, $C0,$C0, $C0,$C0, $7C,$7C
    .db $06,$06, $06,$06, $06,$06, $FC,$FC

; Tile 8: Letter O
    .db $3C,$3C, $66,$66, $C3,$C3, $C3,$C3
    .db $C3,$C3, $C3,$C3, $66,$66, $3C,$3C

; Tile 9: Letter L
    .db $C0,$C0, $C0,$C0, $C0,$C0, $C0,$C0
    .db $C0,$C0, $C0,$C0, $C0,$C0, $FE,$FE

; Tile 10: Letter Y
    .db $C3,$C3, $C3,$C3, $66,$66, $3C,$3C
    .db $18,$18, $18,$18, $18,$18, $18,$18

font_data_end:

.ENDS

;==============================================================================
; VRAM transfer routines
;==============================================================================

.SECTION ".text.vram" SUPERFREE

;------------------------------------------------------------------------------
; vram_init_text - Set up VRAM with font and "TADA" text
;------------------------------------------------------------------------------
vram_init_text:
    php
    phb
    phd
    sep #$20
    rep #$10

    ; Set data bank and direct page to 0
    lda #$00
    pha
    plb
    rep #$20
    lda #$0000
    tcd
    sep #$20

    ; Wait for VBlank
-   lda $4212
    bpl -

    ; Set VRAM address to $0000 (font tiles)
    lda #$80
    sta $2115          ; VRAM increment mode (inc on high byte write)
    rep #$20
    lda #$0000
    sta $2116          ; VMADD = $0000
    sep #$20

    ; Upload font data (4 tiles * 16 bytes = 64 bytes)
    ldx #$0000
@font_loop:
    lda.l font_data,x
    sta $2118          ; VMDATAL
    lda.l font_data+1,x
    sta $2119          ; VMDATAH
    inx
    inx
    cpx #font_data_end - font_data
    bcc @font_loop

    ; Set VRAM address to $0400 (tilemap)
    rep #$20
    lda #$0400
    sta $2116
    sep #$20

    ; Clear tilemap (32x32 = 2048 bytes)
    ldx #$0000
@clear_loop:
    stz $2118
    stz $2119
    inx
    cpx #$0400         ; 1024 words = 2048 bytes
    bcc @clear_loop

    ; Write "TADA" at row 12, column 14 (centered)
    ; Row 12, col 14 = $0400 + (12*32 + 14) = $0400 + $018E = $058E
    rep #$20
    lda #$058E
    sta $2116
    sep #$20

    ; T=1, A=2, D=3, A=2
    lda #$01           ; T
    sta $2118
    lda #$00
    sta $2119
    lda #$02           ; A
    sta $2118
    lda #$00
    sta $2119
    lda #$03           ; D
    sta $2118
    lda #$00
    sta $2119
    lda #$02           ; A
    sta $2118
    lda #$00
    sta $2119

    ; Write "PRESS A TO PLAY" at row 14, column 8 (centered)
    ; Row 14, col 8 = $0400 + (14*32 + 8) = $0400 + $01C8 = $05C8
    rep #$20
    lda #$05C8
    sta $2116
    sep #$20

    ; P=4, R=5, E=6, S=7, S=7, space=0, A=2, space=0, T=1, O=8, space=0, P=4, L=9, A=2, Y=10
    lda #$04           ; P
    sta $2118
    lda #$00
    sta $2119
    lda #$05           ; R
    sta $2118
    lda #$00
    sta $2119
    lda #$06           ; E
    sta $2118
    lda #$00
    sta $2119
    lda #$07           ; S
    sta $2118
    lda #$00
    sta $2119
    lda #$07           ; S
    sta $2118
    lda #$00
    sta $2119
    lda #$00           ; space
    sta $2118
    lda #$00
    sta $2119
    lda #$02           ; A
    sta $2118
    lda #$00
    sta $2119
    lda #$00           ; space
    sta $2118
    lda #$00
    sta $2119
    lda #$01           ; T
    sta $2118
    lda #$00
    sta $2119
    lda #$08           ; O
    sta $2118
    lda #$00
    sta $2119
    lda #$00           ; space
    sta $2118
    lda #$00
    sta $2119
    lda #$04           ; P
    sta $2118
    lda #$00
    sta $2119
    lda #$09           ; L
    sta $2118
    lda #$00
    sta $2119
    lda #$02           ; A
    sta $2118
    lda #$00
    sta $2119
    lda #$0A           ; Y
    sta $2118
    lda #$00
    sta $2119

    pld
    plb
    plp
    rtl

.ENDS
