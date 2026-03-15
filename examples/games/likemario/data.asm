; data.asm - LikeMario asset data
;
; Tile/sprite graphics go in SUPERFREE (any bank, accessed via DMA only).
; Map/collision data go in bank 0 (accessed directly from C code).
;
; IMPORTANT: Since SUPERFREE data may land in bank $01+, C library
; functions like dmaCopyVram() (which hardcode bank $00) cannot be used.
; Instead, loadGraphics() below handles all DMA with explicit bank bytes.

;----------------------------------------------------------------------
; Tile and sprite graphics (DMA-only access, any bank is fine)
;----------------------------------------------------------------------
.section ".rodata1" superfree

tiles_til:        .incbin "res/tiles.pic"
tiles_tilend:

tiles_pal:        .incbin "res/tiles.pal"
tiles_palend:

mario_sprite_til: .incbin "res/mario_sprite.pic"
mario_sprite_tilend:

mario_sprite_pal: .incbin "res/mario_sprite.pal"
mario_sprite_palend:

.ends

;----------------------------------------------------------------------
; Map and collision data (C code reads directly, MUST be in bank 0)
;----------------------------------------------------------------------
.section ".rodata2" semifree bank 0

mapmario:       .incbin "res/BG1.m16"

tilesetatt:     .incbin "res/map_1_1.b16"

.ends

;----------------------------------------------------------------------
; loadGraphics - Load all tile/palette data via DMA with correct bank bytes
;
; Must be called during force blank (REG_INIDISP = 0x80).
; Uses DMA channel 0 for all transfers.
;
; Transfers:
;   1. BG tiles (tiles_til) → VRAM $2000
;   2. BG palette (tiles_pal) → CGRAM color 0
;   3. Sprite palette (mario_sprite_pal) → CGRAM color 128
;----------------------------------------------------------------------
.section ".loader" superfree

loadGraphics:
    php
    sep #$20                    ; 8-bit A
    rep #$10                    ; 16-bit X/Y

    ;-- 1. BG tiles → VRAM $2000 --
    lda #$80
    sta.l $2115                 ; VMAIN: word increment

    rep #$20
    lda #$2000
    sta.l $2116                 ; VRAM destination

    lda #(tiles_tilend - tiles_til)
    sta.l $4305                 ; byte count

    lda #tiles_til
    sta.l $4302                 ; source address (16-bit)

    sep #$20
    lda #:tiles_til             ; BANK BYTE from linker
    sta.l $4304                 ; source bank

    lda #$01                    ; DMA mode: word write ($2118/$2119)
    sta.l $4300
    lda #$18                    ; B-bus destination: VMDATAL
    sta.l $4301
    lda #$01
    sta.l $420B                 ; start DMA channel 0

    ;-- 2. BG palette → CGRAM color 0 (16 colors = 32 bytes) --
    sep #$20
    lda #$00
    sta.l $2121                 ; CGADD = 0 (start at color 0)

    rep #$20
    lda #(tiles_palend - tiles_pal)
    sta.l $4305                 ; byte count

    lda #tiles_pal
    sta.l $4302                 ; source address

    sep #$20
    lda #:tiles_pal             ; BANK BYTE
    sta.l $4304

    lda #$00
    sta.l $4300                 ; DMA mode: byte write
    lda #$22                    ; B-bus destination: CGDATA
    sta.l $4301
    lda #$01
    sta.l $420B                 ; start DMA

    ;-- 3. Sprite palette → CGRAM color 128 (16 colors = 32 bytes) --
    sep #$20
    lda #128
    sta.l $2121                 ; CGADD = 128

    rep #$20
    lda #(mario_sprite_palend - mario_sprite_pal)
    sta.l $4305                 ; byte count

    lda #mario_sprite_pal
    sta.l $4302                 ; source address

    sep #$20
    lda #:mario_sprite_pal      ; BANK BYTE
    sta.l $4304

    lda #$00
    sta.l $4300                 ; DMA mode: byte write
    lda #$22                    ; B-bus destination: CGDATA
    sta.l $4301
    lda #$01
    sta.l $420B                 ; start DMA

    plp
    rtl

.ends

;----------------------------------------------------------------------
; u8 getSpriteTilBank(void)
;
; Returns the bank byte of mario_sprite_til so C code can use
; OAM_SET_GFX_BANK() for correct dynamic sprite DMA.
;----------------------------------------------------------------------
.section ".bank_helpers" superfree

getSpriteTilBank:
    php
    sep #$20
    lda #:mario_sprite_til      ; bank byte from linker
    rep #$20
    and #$00FF                  ; zero-extend to 16-bit return value
    plp
    rtl

.ends
