;---------------------------------------------------------------------------------
;
; LZSS (LZ77) Decompression to VRAM
;
; Decompresses LZ77-encoded data directly to VRAM via PPU registers.
; Based on decompression routines for SNES by mukunda (2009-2010),
; ported from PVSnesLib by Alekmaul (2014-2022).
;
; Copyright (C) 2014-2022 Alekmaul
;
; This software is provided 'as-is', without any express or implied
; warranty.  In no event will the authors be held liable for any
; damages arising from the use of this software.
;
; Permission is granted to anyone to use this software for any
; purpose, including commercial applications, and to alter it and
; redistribute it freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you
;    must not claim that you wrote the original software.
; 2. Altered source versions must be plainly marked as such, and
;    must not be misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source
;    distribution.
;
;---------------------------------------------------------------------------------

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

.EQU REG_VMAIN          $2115   ; Video Port Control          1B/W
.EQU REG_VMADDL         $2116   ; VRAM Address (low)
.EQU REG_VMDATAL        $2118   ; VRAM Data Write (low)
.EQU REG_VMDATAH        $2119   ; VRAM Data Write (high)
.EQU REG_VMDATALREAD    $2139   ; VRAM Data Read Low          1B/R
.EQU REG_VMDATAHREAD    $213A   ; VRAM Data Read High         1B/R

;---------------------------------------------------------------------------------
; Work RAM for decompression state (bank $7E)
;---------------------------------------------------------------------------------
.RAMSECTION ".lzss_state" BANK $7E SLOT 2

lzss_m0     DW      ; VRAM target address (byte address, doubled from word)
lzss_m4     DW      ; copy count
lzss_m5     DW      ; compression flags byte
lzss_m6     DW      ; bit counter

.ENDS

;---------------------------------------------------------------------------------
; LZ77 decompression code (SUPERFREE — placed in any ROM bank)
;---------------------------------------------------------------------------------
.SECTION ".lzss_text" SUPERFREE

.accu 16
.index 16
.16bit

;---------------------------------------------------------------------------------
; void LzssDecodeVram(u8 *source, u16 address)
;
; Decompresses LZ77-encoded data directly to VRAM.
;
; Format: Tag byte 0x10 followed by 3-byte little-endian length,
;         then LZ77 stream (flag bytes + literal/reference pairs).
;         12-bit distance, 4-bit length encoding.
;
; Parameters (on stack, cc65816 calling convention — 16-bit pointers):
;
;   cc65816 pushes args left-to-right, so after php+phb+phx+phy (6B)
;   and 3-byte JSL return address:
;     10-11,s = address  (rightmost arg, pushed last = closest to return)
;     12-13,s = source   (leftmost arg, pushed first = farthest from return)
;
; BANK LIMITATION: Source data must be in bank $00 (same as dmaCopyVram).
; For data in other banks, use an assembly wrapper with :label bank byte.
;
; IMPORTANT: Disables interrupts during decompression to prevent
;            NMI handler from corrupting VRAM writes.
;---------------------------------------------------------------------------------
LzssDecodeVram:
    php
    phb
    phx
    phy
    sei                             ; Disable interrupts (VRAM writes must not be interrupted)

    sep #$20
    lda #$7e                        ; Set data bank to $7E for work RAM access
    pha
    plb

    rep #$20
    lda 12,s                        ; Source address (leftmost arg, higher stack offset)
    sta tcc__r0                     ; tcc__r0 = source address
    sep #$20
    lda #$00                        ; Bank $00 (cc65816 passes 16-bit pointers)
    sta tcc__r0h

    rep #$20
    lda 10,s                        ; VRAM word address (rightmost arg, lower stack offset)
    asl a                           ; Convert to byte address (x2)
    sta lzss_m0                     ; lzss_m0 = VRAM target (byte address)

    sep #$20
    lda #$00                        ; Setup VRAM access (increment on read low)
    sta.l REG_VMAIN

    ldy #0                          ; y = 0 (source index)
    lda [tcc__r0]                   ; Read compression tag byte
    and #$F0
    cmp #$10                        ; 0x1x = LZ77 format
    beq @LZ77source

@ddv_exit:
    ply
    plx
    plb
    plp
    rtl

@LZ77source:
    iny                             ; Skip tag byte, read 3-byte data length
    rep #$20
    lda [tcc__r0], y                ; x = uncompressed data length
    tax
    sep #$20
    iny
    iny
    iny

@LZ77_DecompressLoop:
    lda [tcc__r0], y                ; Read flag byte
    iny
    sta lzss_m5                     ; lzss_m5 = compression flags
    lda #8                          ; 8 bits per flag byte
    sta lzss_m6                     ; lzss_m6 = bit counter

@next_bit:
    asl lzss_m5                     ; Shift out next flag bit
    bcs @lz_byte                    ; 1 = compressed reference, 0 = raw literal

@raw_byte:
    rep #$20                        ; Setup VRAM address for write
    lda lzss_m0
    inc lzss_m0
    lsr                             ; Convert byte addr to word addr
    sta.l REG_VMADDL
    sep #$20
    lda [tcc__r0], y                ; Copy one literal byte
    iny
    bcs +                           ; carry = H/L byte select from lsr

    sta.l REG_VMDATAL               ; Write low byte
    bra ++
+:
    sta.l REG_VMDATAH               ; Write high byte

++:
    dex
    beq @ddv_exit                   ; Done when all bytes written
    dec lzss_m6
    bne @next_bit
    bra @LZ77_DecompressLoop        ; Next flag byte

@lz_byte:
    rep #$20                        ; Read 2-byte reference (disp + count)
    lda [tcc__r0], y
    iny
    iny

    phy                             ; Preserve source index
    sep #$20
    pha                             ; y = target - displacement - 1
    and #$0F
    xba
    rep #$20
    sec
    sbc lzss_m0
    eor #$FFFF
    tay                             ; y = source position in VRAM (byte addr)
    sep #$20
    pla                             ; a = count (top 4 bits + 3)
    lsr
    lsr
    lsr
    lsr
    clc
    adc #3

    sta lzss_m4                     ; lzss_m4 = copy count (16-bit)
    stz lzss_m4+1
    rep #$20                        ; Clamp count to remaining data length
    cpx lzss_m4
    bcs +
    stx lzss_m4
    sec

+:
    txa                             ; Push "x - count" for later
    sbc lzss_m4
    pha
    sep #$20

@copyloop:
    rep #$21                        ; Copy one byte from VRAM back-reference
    tya
    iny
    lsr                             ; Convert source byte addr to word addr
    sta.l REG_VMADDL
    lda lzss_m0
    inc lzss_m0
    bcs @copy_readH

@copy_readL:
    lsr                             ; Convert dest byte addr to word addr
    tax
    sep #$20
    phb
    lda.b #0                        ; Switch to bank $00 for register access
    pha
    plb
    lda.l REG_VMDATALREAD           ; Read low byte from VRAM
    stx REG_VMADDL                  ; Set dest address
    plb
    bcs @copy_writeH

@copy_writeL:
    sta.l REG_VMDATAL
    dec lzss_m4
    bne @copyloop
    bra @copyexit

@copy_readH:
    lsr
    tax
    sep #$20
    phb
    lda.b #0
    pha
    plb
    lda.l REG_VMDATAHREAD           ; Read high byte from VRAM
    stx REG_VMADDL
    plb
    bcc @copy_writeL

@copy_writeH:
    sta.l REG_VMDATAH
    dec lzss_m4
    bne @copyloop

@copyexit:
    plx                             ; Restore remaining count
    ply                             ; Restore source index
    cpx #0

@next_block:
    beq @ddv_exit2                  ; Exit when all data processed
    dec lzss_m6
    beq +
    jmp @next_bit

+:
    jmp @LZ77_DecompressLoop

@ddv_exit2:
    ply
    plx
    plb
    plp
    rtl

.ENDS
