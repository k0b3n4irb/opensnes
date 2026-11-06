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

.ifdef SA1
.include "memmap_sa1.inc"
.else
.ifdef HIROM
.include "memmap_hirom.inc"
.else
.include "memmap.inc"
.endif
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
; void lzssDecodeVram(const u8 *source, u16 address)   (the capitalised 0.x spelling was removed on 2026-10-05)
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
lzssDecodeVram:
    php
    phb
    phx
    phy
    sei                             ; Disable interrupts (VRAM writes must not be interrupted)

    sep #$20
    .ACCU 8
    lda #$7e                        ; Set data bank to $7E for work RAM access
    pha
    plb

    rep #$20
    .ACCU 16
    lda 12,s                        ; Source address (leftmost arg, higher stack offset)
    sta tcc__r0                     ; tcc__r0 = source address
    sep #$20
    .ACCU 8
    lda 14,s                        ; bank byte of the far source pointer (12-15,s).
                                    ; Was a literal $00 ("cc65816 passes 16-bit
                                    ; pointers", pre-A6): compressed data outside
                                    ; bank $00 — the default since #127.3 —
                                    ; decompressed as garbage (fixed 2026-09-20).
    sta tcc__r0h

    rep #$20
    .ACCU 16
    lda 10,s                        ; VRAM word address (rightmost arg, lower stack offset)
    asl a                           ; Convert to byte address (x2)
    sta lzss_m0                     ; lzss_m0 = VRAM target (byte address)

    sep #$20
    .ACCU 8
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
    .ACCU 16
    lda [tcc__r0], y                ; x = uncompressed data length
    tax
    sep #$20
    .ACCU 8
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
    .ACCU 16
    lda lzss_m0
    inc lzss_m0
    lsr                             ; Convert byte addr to word addr
    sta.l REG_VMADDL
    sep #$20
    .ACCU 8
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
    .ACCU 16
    lda [tcc__r0], y
    iny
    iny

    phy                             ; Preserve source index
    sep #$20
    .ACCU 8
    pha                             ; y = target - displacement - 1
    and #$0F
    xba
    rep #$20
    .ACCU 16
    sec
    sbc lzss_m0
    eor #$FFFF
    tay                             ; y = source position in VRAM (byte addr)
    sep #$20
    .ACCU 8
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
    .ACCU 16
    cpx lzss_m4
    bcs +
    stx lzss_m4
    sec

+:
    txa                             ; Push "x - count" for later
    sbc lzss_m4
    pha
    sep #$20
    .ACCU 8

@copyloop:
    rep #$21                        ; Copy one byte from VRAM back-reference
    .ACCU 16
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
    .ACCU 8
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
    .ACCU 8
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

;---------------------------------------------------------------------------------
; u16 lzssDecodeRam(const u8 *source, u8 FAR *dest)
;
; Decompresses an LZ77 (type 0x10) stream into RAM of any bank, $7E and $7F
; included. Its own section: wlalink -d drops it from a ROM that calls only
; lzssDecodeVram, which therefore costs nothing more than before.
;
; Format: tag byte exactly 0x10, 3-byte little-endian output length, then
; flag bytes read MSB first (1 = reference, 0 = literal); a reference is two
; bytes b0 b1: length = (b0 >> 4) + 3, distance = ((b0 & $0F) << 8 | b1) + 1.
;
; Refused (returns 0, writes nothing): a tag other than 0x10, a length of 0
; or above 65535, an output that would run past the end of the destination
; bank. Otherwise returns the length, every byte of it written, none after.
;
; Writes no PPU register and leaves the interrupt flag alone: an NMI may run
; during the decode. The NMI handler saves A/X/Y/D/DBR and runs on its own
; direct page (tcc__nmi_registers), so the tcc__r* slots used here are the
; main thread's alone; an interrupted MVN resumes where it stopped.
;
; Stack (cc65816, args pushed left to right) after php + phb:
;   1,s DBR   2,s P   3-5,s return
;   6-9,s   = dest    (rightmost arg: address, bank, padding)
;   10-13,s = source  (leftmost arg: address, bank, padding)
;
; Registers during the decode (A 8-bit, X/Y 16-bit):
;   DBR = source bank, X = source address (the stream must not cross the end
;   of its bank, as no asset section does), Y = destination address,
;   [tcc__r0],y = destination byte (tcc__r0 = 0, tcc__r0h = dest bank).
; Direct page (caller-saved tcc__r* slots, bank $00):
;   tcc__r1  = destination end address (dest + length, mod $10000)
;   tcc__r1h = flag byte with a sentinel bit (sec + rol: 0 = reload)
;   tcc__r2  = source address saved across a reference copy
;   tcc__r2h = scratch (reference bytes, copy count - 1)
;   tcc__r3  = scratch (Y)
;   tcc__r3h = copy routine for this destination bank (jmp (abs))
;   tcc__r4  = byte count of a byte-loop copy
;   tcc__r4h = length, the return value
;   tcc__r5  = fast end: below it, a whole flag group (8 items of at most
;              18 bytes, 144) fits before the end, so the fast loop checks
;              the end once per flag byte instead of after every item
;
; References: MVN copies one byte at a time upwards, reading each source byte
; after the previous one is written, which is exactly LZ77's semantics when
; the distance is shorter than the length (distance 1 = one byte repeated).
; MVN's banks are immediates: banks $7E and $7F get their own MVN, any other
; bank a byte loop through the data bank. The last flag groups (the careful
; loop) copy byte by byte and cut a reference at the end of the output.
;---------------------------------------------------------------------------------
.SECTION ".lzss_ram_text" SUPERFREE

lzssDecodeRam:
    php
    phb
    rep #$30
    .ACCU 16
    .INDEX 16
    stz.b tcc__r0                   ; [tcc__r0],y: base 0, Y is the address
    lda 8,s                         ; bank byte of dest (6-9,s)
    and #$00FF
    sta.b tcc__r0h
    lda 10,s                        ; source address (leftmost arg)
    tax
    sep #$20
    .ACCU 8
    lda 12,s                        ; bank byte of source (10-13,s)
    pha
    plb                             ; DBR = source bank
    lda.w $0000,x                   ; tag
    cmp #$10
    bne @refuse
    lda.w $0003,x                   ; length bits 16-23: above 65535 is refused
    bne @refuse
    rep #$20
    .ACCU 16
    lda.w $0001,x                   ; length
    beq @refuse
    sta.b tcc__r4h
    clc
    adc 6,s                         ; dest address + length
    bcc +
    bne @refuse                     ; past the end of the bank ($10000 is the last fit)
+:  sta.b tcc__r1                   ; end address
    lda 6,s                         ; dest address
    tay
    lda.b tcc__r4h
    cmp #145
    bcc +                           ; 144 bytes or fewer: careful loop from the start
    tya
    clc
    adc.b tcc__r4h
    sec
    sbc #144
    bra ++
+:  tya                             ; fast end = dest: Y is never below it
++: sta.b tcc__r5
    lda #@cpslow                    ; copy routine for the destination bank
    ldy.b tcc__r0h
    cpy #$007E
    bne +
    lda #@cp7e
+:  cpy #$007F
    bne +
    lda #@cp7f
+:  sta.b tcc__r3h
    lda 6,s                         ; dest address
    tay
    inx                             ; past the 4-byte header
    inx
    inx
    inx
    sep #$20
    .ACCU 8
    bra @reload

@refuse:                            ; within reach of the header checks above
    rep #$20
    .ACCU 16
    lda #0
    plb
    plp
    rtl

;--- fast loop: no end check inside a flag group ---------------------------------
@reload:                            ; next flag byte (A 8-bit)
    .ACCU 8
    cpy.b tcc__r5
    bcc +
    brl @creload                    ; 144 bytes or fewer left: careful loop
+:  lda.w $0000,x
    bne @flags
    ; Flag byte 0: eight literals, two at a time.
    rep #$20
    .ACCU 16
    lda.w $0001,x
    sta [tcc__r0],y
    iny
    iny
    lda.w $0003,x
    sta [tcc__r0],y
    iny
    iny
    lda.w $0005,x
    sta [tcc__r0],y
    iny
    iny
    lda.w $0007,x
    sta [tcc__r0],y
    iny
    iny
    txa
    clc
    adc #9                          ; the flag byte and eight literals
    tax
    sep #$20
    .ACCU 8
    bra @reload

@flags:
    inx
    sec
    rol a                           ; carry = first flag, sentinel in bit 0
    sta.b tcc__r1h
    bcs @ref
@lit:
    lda.w $0000,x
    inx
    sta [tcc__r0],y
    iny
@next:
    asl.b tcc__r1h
    beq @reload                     ; the sentinel went out: 8 flags done
    bcc @lit

@ref:
    rep #$20
    .ACCU 16
    lda.w $0000,x                   ; A = b1 << 8 | b0
    inx
    inx
    stx.b tcc__r2
    sta.b tcc__r2h
    xba
    and #$0FFF                      ; distance - 1
    eor #$FFFF                      ; -distance
    sty.b tcc__r3
    clc
    adc.b tcc__r3
    tax                             ; X = Y - distance, the copy's source
    lda.b tcc__r2h
    and #$00F0
    lsr a
    lsr a
    lsr a
    lsr a                           ; carry clear: the low nibble was zero
    adc #2                          ; A = count - 1 (2..17)
    phb
    jmp (tcc__r3h)

@cp7e:
    mvn $7E, $7E                    ; WLA-DX: mvn src_bank, dst_bank
    bra @cpdone
@cp7f:
    mvn $7F, $7F
    bra @cpdone
@cpslow:
    inc a                           ; count (3..18)
    sta.b tcc__r4
    sep #$20
    .ACCU 8
    lda.b tcc__r0h
    pha
    plb                             ; DBR = destination bank
-:  lda.w $0000,x
    sta.w $0000,y
    inx
    iny
    dec.b tcc__r4
    bne -
@cpdone:
    plb                             ; DBR = source bank again
    ldx.b tcc__r2
    sep #$20
    .ACCU 8
    bra @next

;--- careful loop: the last 144 bytes or fewer, end checked after every item -----
@creload:
    .ACCU 8
    lda.w $0000,x
    inx
    sec
    rol a
    sta.b tcc__r1h
    bcs @cref
@clit:
    lda.w $0000,x
    inx
    sta [tcc__r0],y
    iny
    cpy.b tcc__r1
    beq @done
@cnext:
    asl.b tcc__r1h
    beq @creload
    bcc @clit

@cref:
    rep #$20
    .ACCU 16
    lda.w $0000,x                   ; A = b1 << 8 | b0
    inx
    inx
    stx.b tcc__r2
    sta.b tcc__r2h
    xba
    and #$0FFF
    eor #$FFFF
    sty.b tcc__r3
    clc
    adc.b tcc__r3
    tax                             ; X = Y - distance
    lda.b tcc__r2h
    and #$00F0
    lsr a
    lsr a
    lsr a
    lsr a
    adc #3                          ; count (3..18)
    sta.b tcc__r4
    lda.b tcc__r1
    sec
    sbc.b tcc__r3                   ; what is left to write
    cmp.b tcc__r4
    bcs +
    sta.b tcc__r4                   ; a reference past the end is cut there
+:  sep #$20
    .ACCU 8
    phb
    lda.b tcc__r0h
    pha
    plb                             ; DBR = destination bank
-:  lda.w $0000,x
    sta.w $0000,y
    inx
    iny
    dec.b tcc__r4
    bne -
    plb                             ; DBR = source bank
    ldx.b tcc__r2
    cpy.b tcc__r1
    bne @cnext

@done:
    rep #$20
    .ACCU 16
    lda.b tcc__r4h                  ; bytes written
    plb
    plp
    rtl

.ENDS
