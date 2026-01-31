;==============================================================================
; OpenSNES DMA Functions (Assembly)
;
; These functions handle DMA transfers from ROM to VRAM/CGRAM.
;
; IMPORTANT: Source addresses are 24-bit (bank + offset) to support data
; in any ROM bank. The compiler passes 16-bit pointers, and we extract
; the bank from the high byte of the 24-bit far address.
;
; For LoROM with SUPERFREE sections, data can be in any bank.
; The linker places data and we need to DMA from the correct bank.
;==============================================================================

.include "lib_memmap.inc"

.SECTION ".dma_asm" SUPERFREE

;------------------------------------------------------------------------------
; void dmaCopyVram(u8 *source, u16 vramAddr, u16 size)
;
; NOTE: In the current cproc compiler, pointers are 16-bit.
; For ROM data, we assume bank 0 (LoROM $00:8000-$FFFF).
; Data in SUPERFREE sections MUST be in bank 0 or use dmaCopyVramBank().
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

    lda 9,s                 ; source address (16-bit)
    sta.l $4302             ; DMA source address

    sep #$20
    lda #$80
    sta.l $2115             ; REG_VMAIN: increment after high byte write

    ; Get bank from source address high bits
    ; For LoROM, addresses $8000+ are in ROM
    ; Bank = (source >> 16) but we only have 16-bit address
    ; So we need to figure out the bank from the address
    ; If address >= $8000, it's in ROM bank determined by linker
    ; For now, assume bank 0 for addresses < $8000 (shouldn't happen for ROM)
    ; and let the linker handle bank placement
    lda 10,s                ; high byte of source address
    cmp #$80
    bcc +
    ; Address is $8000+, which is LoROM space
    ; In LoROM, bank 0 $8000-$FFFF = first 32KB of ROM
    lda #$00
    bra ++
+   ; Address is < $8000, use it as-is (shouldn't happen for ROM data)
    lda #$00
++  sta.l $4304             ; DMA source bank

    lda #$01
    sta.l $4300             ; DMA mode: 2-register write (word)

    lda #$18
    sta.l $4301             ; Destination: VMDATAL ($2118)

    lda #$01
    sta.l $420B             ; Start DMA channel 0

    plp
    rtl

;------------------------------------------------------------------------------
; void dmaCopyVramBank(u8 *source, u8 bank, u16 vramAddr, u16 size)
;
; DMA with explicit bank byte for data in banks other than 0.
;
; Stack layout (after PHP):
;   5-6,s = size
;   7-8,s = vramAddr
;   9,s = bank (8-bit, padded to 16-bit push)
;   10-11,s = source
;------------------------------------------------------------------------------
dmaCopyVramBank:
    php

    rep #$20
    lda 7,s                 ; vramAddr
    sta.l $2116             ; REG_VMADDL/H

    lda 5,s                 ; size
    sta.l $4305             ; DMA size

    lda 11,s                ; source address (16-bit)
    sta.l $4302             ; DMA source address

    sep #$20
    lda #$80
    sta.l $2115             ; REG_VMAIN

    lda 9,s                 ; bank byte
    sta.l $4304             ; DMA source bank

    lda #$01
    sta.l $4300             ; DMA mode

    lda #$18
    sta.l $4301             ; Destination: VMDATAL

    lda #$01
    sta.l $420B             ; Start DMA

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
    lda #$00                ; Source bank = 0 (same limitation as dmaCopyVram)
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

;------------------------------------------------------------------------------
; void clearNmiFlag(void)
;
; Reads REG_RDNMI ($4210) to clear the pending NMI flag.
; This is needed because the C compiler optimizes away volatile reads
; when the result is discarded.
;------------------------------------------------------------------------------
clearNmiFlag:
    php
    sep #$20
    lda.l $4210             ; Read REG_RDNMI to clear flag
    plp
    rtl

.ENDS
