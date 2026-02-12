;==============================================================================
; sprite32_diag - Sprite Data + Pure Assembly Sprite Setup
;==============================================================================

.section ".rodata1" superfree

sprite32:
.incbin "res/sprite32.pic"
sprite32_end:

palsprite32:
.incbin "res/sprite32.pal"

.ends

;==============================================================================
; Pure assembly function: sets up a 32x32 sprite without ANY compiled C code.
; Bypasses oamSet, oamSetSize, oamSetEx, oamUpdate, dmaCopyVram entirely.
; Writes directly to PPU OAM via $2104 during forced blank.
;
; Call from C: extern void asm_setupSpriteDirectPPU(void);
; MUST be called during forced blank (before setScreenOn).
;==============================================================================

.section ".text_asmtest" superfree

asm_setupSpriteDirectPPU:
    php
    rep #$10        ; 16-bit X/Y
    sep #$20        ; 8-bit A

    ;--------------------------------------------------
    ; 1. DMA sprite32 tile data to VRAM $2100
    ;    (Using DMA channel 1 to avoid any conflict)
    ;--------------------------------------------------
    lda #$80
    sta $2115        ; VMAIN: increment on high byte write

    rep #$20         ; 16-bit A for VRAM address
    lda #$2100
    sta $2116        ; VMADDL/H: VRAM word address $2100
    sep #$20         ; back to 8-bit

    lda #$01
    sta $4310        ; DMA mode: 2-register sequential (VMDATAL/VMDATAH)
    lda #$18
    sta $4311        ; BBAD: $2118 (VMDATAL)

    rep #$20
    lda #sprite32
    sta $4312        ; Source address low + high
    sep #$20
    lda #:sprite32
    sta $4314        ; Source bank

    rep #$20
    lda #(sprite32_end - sprite32)
    sta $4315        ; Transfer size (2048 bytes)
    sep #$20

    lda #$02         ; Channel 1 bit
    sta $420B        ; Start DMA

    ;--------------------------------------------------
    ; 2. DMA palette to CGRAM 128 (sprite palette 0)
    ;--------------------------------------------------
    lda #128
    sta $2121        ; CGADD: start at color 128

    lda #$00
    sta $4310        ; DMA mode: 1-register sequential
    lda #$22
    sta $4311        ; BBAD: $2122 (CGDATA)

    rep #$20
    lda #palsprite32
    sta $4312        ; Source address
    sep #$20
    lda #:palsprite32
    sta $4314        ; Source bank

    rep #$20
    lda #32          ; 32 bytes = 16 colors
    sta $4315
    sep #$20

    lda #$02
    sta $420B        ; Start DMA

    ;--------------------------------------------------
    ; 3. Set OBSEL: small=8, large=32, name base=1
    ;    OBSEL = (1 << 5) | 1 = 0x21
    ;--------------------------------------------------
    lda #$21
    sta $2101        ; OBJSEL

    ;--------------------------------------------------
    ; 4. Write sprite entry DIRECTLY to PPU OAM
    ;    (Not through buffer — directly to $2104)
    ;    Sprite 0: X=100, Y=80, tile=$10, attr=$30
    ;              (tile=$10 = ($2100-$2000)/16, priority 3)
    ;--------------------------------------------------
    ; Set OAM address to word 0 (sprite 0)
    stz $2102        ; OAMADDL = 0
    stz $2103        ; OAMADDH = 0

    ; Write 4 bytes (2 words) for sprite 0
    lda #100         ; X low = 100
    sta $2104
    lda #80          ; Y = 80
    sta $2104
    lda #$10         ; Tile number low = $10
    sta $2104
    lda #$30         ; Attributes: priority 3, palette 0, no flip, tile high=0
    sta $2104

    ;--------------------------------------------------
    ; 5. Write extension table for sprite 0
    ;    OAM word address 256 = extension table start
    ;    Byte 0: sprite 0-3 extension bits
    ;    Sprite 0 = bits 0-1: bit0=X high(0), bit1=size(1=large)
    ;    Value = 0x02 (size=large, X high=0)
    ;--------------------------------------------------
    ; Set OAM address to word 256 (extension table start)
    stz $2102        ; OAMADDL = 0
    lda #$01
    sta $2103        ; OAMADDH = 1 (bit 0 set = address bit 8 = word 256)

    ; Extension table write: each byte write goes through the latch mechanism
    ; Write two bytes: first = low byte of word (sprites 0-3), second = high (sprites 4-7)
    lda #$02         ; Sprite 0: size=large(bit1=1), X high=0(bit0=0)
                     ; Sprites 1-3: all 0 (hidden via Y=0 default)
    sta $2104        ; First byte (low) of extension word 0
    lda #$00         ; Sprites 4-7: all 0
    sta $2104        ; Second byte (high) of extension word 0

    plp
    rtl

.ends
