;==============================================================================
; OpenSNES - Assembly oamSet()
;==============================================================================
; Hand-optimized 65816 replacement for C oamSet().
; Eliminates framesize=108 overhead (~100 cycles saved per call).
;
; Calling convention (cc65816, left-to-right push):
;   oamSet(id, x, y, tile, palette, priority, flags)
;   All parameters are u16 (2 bytes each).
;
;   Stack layout after JSL (no frame allocation):
;     16,s = id        (first pushed, highest on stack)
;     14,s = x
;     12,s = y
;     10,s = tile
;      8,s = palette
;      6,s = priority
;      4,s = flags     (last pushed, lowest above return addr)
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

.SECTION ".text.oamSet" SUPERFREE

oamSet:
    rep #$30            ; 16-bit A and X/Y

    ; Bounds check: id >= 128 → return
    lda 16,s            ; id
    cmp.w #128
    bcc @in_range
    rtl
@in_range:

    ; Compute OAM low table offset = id * 4
    asl a
    asl a
    tax                 ; X = id * 4

    ;------------------------------------------------------------------
    ; Write 4 bytes to OAM low table
    ;------------------------------------------------------------------
    sep #$20            ; 8-bit A for byte stores

    ; Byte 0: X position (low byte)
    lda 14,s            ; x low byte
    sta.l oamMemory,x

    ; Byte 1: Y position
    lda 12,s            ; y low byte
    sta.l oamMemory+1,x

    ; Byte 2: tile number (low 8 bits)
    lda 10,s            ; tile low byte
    sta.l oamMemory+2,x

    ; Byte 3: attributes (vhoopppc)
    ;   v = flags bit 7, h = flags bit 6
    ;   oo = priority bits 1-0
    ;   ppp = palette bits 2-0
    ;   c = tile bit 8
    lda 4,s             ; flags
    and #$C0            ; keep v,h bits
    sta.b tcc__r9

    lda 6,s             ; priority
    and #$03
    asl a
    asl a
    asl a
    asl a               ; (priority & 3) << 4
    ora.b tcc__r9
    sta.b tcc__r9

    lda 8,s             ; palette
    and #$07
    asl a               ; (palette & 7) << 1
    ora.b tcc__r9
    sta.b tcc__r9

    lda 11,s            ; tile high byte (bit 8)
    and #$01
    ora.b tcc__r9
    sta.l oamMemory+3,x

    ;------------------------------------------------------------------
    ; High table: set/clear X high bit
    ;------------------------------------------------------------------
    ; Extension table: oamMemory[512 + (id >> 2)]
    ; Each byte covers 4 sprites, 2 bits per sprite:
    ;   bit 0 of pair = X high bit, bit 1 = size
    ; Slot = id & 3, XHI mask = 1 << (slot * 2)

    rep #$20            ; 16-bit A (X/Y still 16-bit)

    ; Compute XHI bit mask from slot
    lda 16,s            ; id
    and.w #$0003        ; slot = id & 3
    asl a               ; slot * 2 = shift amount
    tay                 ; Y = shift count

    sep #$20            ; 8-bit A
    lda #$01
    cpy.w #0
    beq @mask_ready
@shift_mask:
    asl a
    dey
    bne @shift_mask
@mask_ready:
    sta.b tcc__r9       ; XHI bit mask

    ; Compute ext byte index = id >> 2
    rep #$20            ; 16-bit A
    lda 16,s            ; id
    lsr a
    lsr a               ; id >> 2
    tax                 ; X = ext byte index

    ; Check x bit 8 (high byte of x parameter)
    sep #$20            ; 8-bit A
    lda 15,s            ; x high byte
    and #$01
    beq @clear_xhi

    ; Set XHI bit
    lda.l oamMemory+512,x
    ora.b tcc__r9
    sta.l oamMemory+512,x
    bra @xhi_done

@clear_xhi:
    ; Clear XHI bit (preserve size bit)
    lda.b tcc__r9
    eor #$FF            ; ~mask
    and.l oamMemory+512,x
    sta.l oamMemory+512,x

@xhi_done:
    lda #$01
    sta.l oam_update_flag

    ; Track highest sprite ID for partial OAM DMA
    ; A is already 8-bit from sep #$20 above
    lda 16,s            ; id (low byte, already < 128)
    cmp.l oam_max_id
    bcc @skip_max       ; id < oam_max_id → skip
    sta.l oam_max_id
@skip_max:

    rep #$20            ; Restore 16-bit A (C calling convention)
    rtl

.ENDS
