;==============================================================================
; OpenSNES DMA Functions (Assembly)
;
; These functions handle DMA transfers from ROM to VRAM/CGRAM.
;
; IMPORTANT: Our compiler uses cdecl calling convention (right-to-left push).
; For dmaCopyVram(source, vramAddr, size):
;   - size is pushed first (closest to SP after call)
;   - vramAddr is pushed second
;   - source is pushed last (furthest from SP)
;==============================================================================

.include "lib_memmap.inc"

.SECTION ".dma_asm" SUPERFREE

;------------------------------------------------------------------------------
; void dmaCopyVram(u8 *source, u16 vramAddr, u16 size)
;
; Stack layout (after PHP):
;   1,s = P (processor status)
;   2-4,s = return address (3 bytes from JSL)
;   5-6,s = size (pushed first - rightmost arg)
;   7-8,s = vramAddr (pushed second)
;   9-10,s = source (pushed last - leftmost arg)
;------------------------------------------------------------------------------
dmaCopyVram:
    php

    rep #$20
    lda 7,s                 ; vramAddr
    sta.l $2116             ; REG_VMADDL/H

    lda 5,s                 ; size
    sta.l $4305             ; DMA size

    lda 9,s                 ; source address
    sta.l $4302             ; DMA source address

    sep #$20
    lda #$80
    sta.l $2115             ; REG_VMAIN: increment after high byte write

    lda #$00                ; Source bank = 0 (LoROM ROM bank)
    sta.l $4304             ; DMA source bank

    lda #$01
    sta.l $4300             ; DMA mode: 2-register write (word)

    lda #$18
    sta.l $4301             ; Destination: VMDATAL ($2118)

    lda #$01
    sta.l $420B             ; Start DMA channel 0

    plp
    rtl

;------------------------------------------------------------------------------
; void dmaCopyCGram(u8 *source, u16 startColor, u16 size)
;
; Stack layout (after PHP):
;   5-6,s = size (rightmost arg)
;   7-8,s = startColor
;   9-10,s = source (leftmost arg)
;------------------------------------------------------------------------------
dmaCopyCGram:
    php

    sep #$20
    lda 7,s                 ; startColor (low byte = color index)
    sta.l $2121             ; REG_CGADD

    rep #$20
    lda 5,s                 ; size
    sta.l $4305             ; DMA size

    lda 9,s                 ; source address
    sta.l $4302             ; DMA source address

    sep #$20
    lda #$00                ; Source bank = 0
    sta.l $4304             ; DMA source bank

    lda #$00
    sta.l $4300             ; DMA mode: 1-register write (byte)

    lda #$22
    sta.l $4301             ; Destination: CGDATA ($2122)

    lda #$01
    sta.l $420B             ; Start DMA channel 0

    plp
    rtl

;------------------------------------------------------------------------------
; void dmaCopyOam(u8 *source, u16 size)
;
; Stack layout (after PHP):
;   5-6,s = size (rightmost arg)
;   7-8,s = source (leftmost arg)
;------------------------------------------------------------------------------
dmaCopyOam:
    php

    rep #$20
    lda #$0000
    sta.l $2102             ; REG_OAMADDL/H: OAM address = 0

    lda 5,s                 ; size
    sta.l $4305             ; DMA size

    lda 7,s                 ; source address
    sta.l $4302             ; DMA source address

    sep #$20
    lda #$7E                ; Source bank = $7E (Work RAM)
    sta.l $4304             ; DMA source bank

    lda #$00
    sta.l $4300             ; DMA mode: 1-register write

    lda #$04
    sta.l $4301             ; Destination: OAMDATA ($2104)

    lda #$01
    sta.l $420B             ; Start DMA channel 0

    plp
    rtl

.ENDS
