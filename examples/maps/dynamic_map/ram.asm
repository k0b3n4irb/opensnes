;==============================================================================
; DynamicMap - RAM Sections + Assembly Helpers
;
; Large buffers live in extended WRAM (bank $7E/$7F) because the compiler
; generates sta.l $0000,x which can only reach bank $00:$0000-$1FFF (8KB).
;
; SLOT 2 = $2000-$FFFF (56KB) — used for both banks.
;
; Spritemap buffer:  bank $7E:$2000+  (u16 array, max 0x4000 bytes = 16KB)
; Sprite graphics:   bank $7F:$2000+  (u8 array, 0x4000 bytes = 16KB)
;==============================================================================

;------------------------------------------------------------------------------
; RAM Sections
;------------------------------------------------------------------------------

.RAMSECTION "spritemap_buf" BANK $7E SLOT 2
    spritemap_buf: dsb $4000    ; 16KB spritemap buffer (u16[0x2000])
.ENDS

.RAMSECTION "sprites_gfx" BANK $7F SLOT 2
    sprites_ram: dsb $4000      ; 16KB sprite graphics buffer
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
; u16 smapRead(u16 byte_offset)
; Read a u16 value from spritemap_buf at given byte offset. Returns in A.
;------------------------------------------------------------------------------
smapRead:
    php
    rep #$30
    lda 5,s             ; byte_offset
    tax
    lda.l spritemap_buf,x
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

;------------------------------------------------------------------------------
; void sramWrite(u16 byte_offset, u8 value)
; Write a byte to sprites_ram at given offset.
;------------------------------------------------------------------------------
sramWrite:
    php
    rep #$30
    lda 7,s             ; byte_offset
    tax
    sep #$20            ; 8-bit A for byte write
    lda 5,s             ; value (low byte)
    sta.l sprites_ram,x
    plp
    rtl

;------------------------------------------------------------------------------
; void sramCopy(u8 *src, u16 dst_offset, u16 len)
; Copy len bytes from src (bank $00) to sprites_ram+dst_offset (bank $7F).
;
; IMPORTANT: MVN changes the Data Bank Register (DBR) to the destination
; bank ($7F). We must save/restore DBR with PHB/PLB.
;------------------------------------------------------------------------------
sramCopy:
    php
    phb                 ; save DBR (MVN changes it!)
    rep #$30

    ; Stack layout after php+phb (+1 byte shift):
    ;   1,s = DBR
    ;   2,s = P
    ;   3-5,s = return address
    ;   6-7,s = len      (was 5-6,s)
    ;   8-9,s = dst_off  (was 7-8,s)
    ;   10-11,s = src    (was 9-10,s)

    lda 6,s             ; len
    beq @done           ; skip if zero length

    ; Set up destination: X = sprites_ram + dst_offset
    lda 8,s             ; dst_offset
    clc
    adc #sprites_ram
    tax

    lda 10,s            ; src pointer (bank $00)
    tay                 ; Y = source address

    lda 6,s             ; len
    dec a               ; MVN moves A+1 bytes
    mvn $00, $7F        ; move from bank $00 (Y) to bank $7F (X)

@done:
    plb                 ; restore DBR
    plp
    rtl

;------------------------------------------------------------------------------
; void sramClear(u16 offset, u16 len)
; Clear len bytes at sprites_ram+offset (bank $7F) to zero.
;------------------------------------------------------------------------------
sramClear:
    php
    rep #$30

    lda 7,s             ; offset
    clc
    adc #sprites_ram    ; absolute address in bank $7F
    tax

    lda 5,s             ; len
    beq @done
    tay                 ; Y = counter

    sep #$20            ; 8-bit writes
    lda #0
-   sta.l $7F0000,x
    inx
    dey
    bne -

@done:
    plp
    rtl

;------------------------------------------------------------------------------
; void sramDma(u16 byte_offset, u16 vram_addr, u16 byte_count)
; DMA sprites_ram data to VRAM from bank $7F.
;------------------------------------------------------------------------------
sramDma:
    php
    rep #$20

    lda 7,s             ; vram_addr
    sta.l $2116         ; REG_VMADDL/H

    lda 5,s             ; byte_count
    sta.l $4305         ; DMA size

    ; Source address = sprites_ram + byte_offset
    lda 9,s             ; byte_offset
    clc
    adc #sprites_ram    ; add base address
    sta.l $4302         ; DMA source address

    sep #$20
    lda #$80
    sta.l $2115         ; VMAIN: increment after high byte write

    lda #$7F            ; Source bank = $7F
    sta.l $4304

    lda #$01
    sta.l $4300         ; DMA mode: 2-register write (word)

    lda #$18
    sta.l $4301         ; Destination: VMDATAL ($2118)

    lda #$01
    sta.l $420B         ; Start DMA channel 0

    plp
    rtl

;------------------------------------------------------------------------------
; void sramDmaPal(u8 *pal_src, u16 cgram_offset, u16 byte_count)
; DMA palette data to CGRAM from bank $00 ROM.
; (Convenience wrapper since bgInitTileSet can't handle bank $7F source.)
;------------------------------------------------------------------------------
sramDmaPal:
    php
    sep #$20

    lda 7,s             ; cgram_offset (low byte = color index)
    sta.l $2121         ; REG_CGADD

    rep #$20
    lda 5,s             ; byte_count
    sta.l $4305         ; DMA size

    lda 9,s             ; pal_src (16-bit pointer)
    sta.l $4302         ; DMA source address

    sep #$20
    lda 10,s            ; high byte of source
    cmp #$80
    bcc @ram_bank
    lda #$00            ; ROM address → bank $00
    bra @set_bank
@ram_bank:
    lda #$7E            ; RAM address → bank $7E
@set_bank:
    sta.l $4304         ; DMA source bank

    lda #$00
    sta.l $4300         ; DMA mode: 1-register write (byte)

    lda #$22
    sta.l $4301         ; Destination: CGDATA ($2122)

    lda #$01
    sta.l $420B         ; Start DMA channel 0

    plp
    rtl

.ENDS
