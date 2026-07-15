;==============================================================================
; DynamicMap - RAM Sections + Assembly Helpers
;
; The tilemap buffer lives in extended WRAM (bank $7E) because the compiler
; generates sta.l $0000,x which can only reach bank $00:$0000-$1FFF (8KB).
;
; SLOT 2 = $2000-$FFFF (56KB).
;
; Spritemap buffer:  bank $7E:$2000+  (u16 array, max 0x4000 bytes = 16KB)
;==============================================================================

;------------------------------------------------------------------------------
; RAM Sections
;------------------------------------------------------------------------------

.RAMSECTION "spritemap_buf" BANK $7E SLOT 2
    spritemap_buf: dsb $4000    ; 16KB spritemap buffer (u16[0x2000])
.ENDS


;==============================================================================
; Assembly helper routines
;
; cc65816 pushes arguments LEFT-TO-RIGHT:
;   f(a, b, c) → push a, push b, push c, jsl f
; After JSL+PHP:
;   1,s     = P (from PHP)
;   2-4,s   = return address (3 bytes)
;   5-6,s   = c (last pushed = lowest address)
;   7-8,s   = b
;   9-10,s  = a (first pushed = highest address)
;==============================================================================

.SECTION ".ramhelpers" SUPERFREE

;------------------------------------------------------------------------------
; void smapWrite(u16 byte_offset, u16 value)
; Write a u16 value to spritemap_buf at given byte offset.
;------------------------------------------------------------------------------
smapWrite:
    php
    rep #$30            ; 16-bit A and X/Y
    lda 7,s             ; byte_offset
    tax
    lda 5,s             ; value
    sta.l spritemap_buf,x
    plp
    rtl

;------------------------------------------------------------------------------
; void smapClear(u16 byte_count)
; Clear byte_count bytes of spritemap_buf (set to 0).
;------------------------------------------------------------------------------
smapClear:
    php
    rep #$30
    lda 5,s             ; byte_count (in bytes)
    lsr a               ; convert to word count
    tay
    ldx #0
    lda #0
-   sta.l spritemap_buf,x
    inx
    inx
    dey
    bne -
    plp
    rtl

;------------------------------------------------------------------------------
; void smapDma(u16 byte_offset, u16 vram_addr, u16 byte_count)
; DMA spritemap_buf data to VRAM from bank $7E.
;------------------------------------------------------------------------------
smapDma:
    php
    rep #$20

    lda 7,s             ; vram_addr
    sta.l $2116         ; REG_VMADDL/H

    lda 5,s             ; byte_count
    sta.l $4305         ; DMA size

    ; Source address = spritemap_buf + byte_offset
    lda 9,s             ; byte_offset
    clc
    adc #spritemap_buf  ; add base address
    sta.l $4302         ; DMA source address

    sep #$20
    lda #$80
    sta.l $2115         ; VMAIN: increment after high byte write

    lda #$7E            ; Source bank = $7E
    sta.l $4304

    lda #$01
    sta.l $4300         ; DMA mode: 2-register write (word)

    lda #$18
    sta.l $4301         ; Destination: VMDATAL ($2118)

    lda #$01
    sta.l $420B         ; Start DMA channel 0

    plp
    rtl

.ENDS
