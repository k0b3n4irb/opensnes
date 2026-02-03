;==============================================================================
; OpenSNES HDMA Functions (Assembly)
;
; HDMA requires 24-bit source addresses for tables in ROM.
; The C version can't handle this correctly, so we need assembly.
;
;==============================================================================
; BANK LIMITATION (same as DMA)
;==============================================================================
; HDMA table bank handling:
;   - ROM addresses ($8000+): Bank is set to $00
;   - RAM addresses (< $8000): Bank is set to $7E
;
; This means HDMA tables in ROM must be in bank 0 for correct operation.
; For most projects this is not a limitation since HDMA tables are small
; and typically placed via SUPERFREE which lands in bank 0.
;
; If you need HDMA tables in higher banks, you would need to modify
; hdmaSetup() to accept an explicit bank parameter.
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

.SECTION ".hdma_asm" SUPERFREE

;------------------------------------------------------------------------------
; void hdmaSetup(u8 channel, u8 mode, u8 destReg, const void *table)
;
; Sets up an HDMA channel with proper 24-bit address handling.
;
; Stack layout (after PHP):
;   1,s = P (saved status)
;   2-4,s = return address (3 bytes from JSL)
;   5-6,s = table address low 16 bits (leftmost = last pushed)
;   7,s = destReg (8-bit, padded to 16)
;   8,s = padding
;   9,s = mode (8-bit, padded to 16)
;   10,s = padding
;   11,s = channel (8-bit, padded to 16)
;
; Actually, cproc pushes args right-to-left, so:
;   5-6,s = channel (rightmost = first pushed... no wait)
;
; Let me check the calling convention...
; Looking at dma.asm: dmaCopyVram(u8 *source, u16 vramAddr, u16 size)
;   5-6,s = size (rightmost)
;   7-8,s = vramAddr
;   9-10,s = source (leftmost)
;
; So for hdmaSetup(u8 channel, u8 mode, u8 destReg, const void *table):
;   5-6,s = table (rightmost)
;   7,s = destReg
;   9,s = mode
;   11,s = channel
;
; Wait, 8-bit args are pushed as 16-bit on the stack.
; Let me be more precise...
;------------------------------------------------------------------------------
hdmaSetup:
    php
    rep #$30                ; 16-bit A and X/Y

    ; Stack layout after PHP (1 byte) + JSL return (3 bytes):
    ;   1,s = P
    ;   2-4,s = return address
    ;   5-6,s = table (rightmost arg, pushed first)
    ;   7-8,s = destReg (8-bit in 16-bit slot)
    ;   9-10,s = mode (8-bit in 16-bit slot)
    ;   11-12,s = channel (leftmost arg, pushed last)

    ; Calculate DMA register base address for this channel
    ; Channel registers are at $4300 + (channel * $10)
    sep #$20
    lda 11,s                ; channel (8-bit)
    cmp #8
    bcs @done               ; Invalid channel, bail out

    rep #$20
    and #$00FF              ; Mask to 8-bit
    asl a
    asl a
    asl a
    asl a                   ; channel * 16
    clc
    adc #$4300              ; Base address
    tax                     ; X = register base ($43x0)

    ; Set HDMA mode (DMAPx at $43x0)
    sep #$20
    lda 9,s                 ; mode (8-bit)
    sta.l $0000,x           ; $43x0 = DMAP

    ; Set destination register (BBADx at $43x1)
    lda 7,s                 ; destReg (8-bit)
    sta.l $0001,x           ; $43x1 = BBAD

    ; Set table address (A1Tx at $43x2-$43x4)
    rep #$20
    lda 5,s                 ; table address (16-bit)
    sta.l $0002,x           ; $43x2-$43x3 = A1TL/A1TH

    ; For bank: if address >= $8000, it's ROM in bank 0 (LoROM)
    ; For addresses < $8000, assume bank $7E (RAM)
    sep #$20
    lda 6,s                 ; High byte of table address
    cmp #$80
    bcc @use_ram_bank
    ; ROM address ($8000+) - use bank 0
    lda #$00
    bra @set_bank
@use_ram_bank:
    ; RAM address (< $8000) - use bank $7E
    lda #$7E
@set_bank:
    sta.l $0004,x           ; $43x4 = A1B (bank)

@done:
    plp
    rtl

;------------------------------------------------------------------------------
; void hdmaEnable(u8 channelMask)
;
; Enables specified HDMA channels.
;
; Stack layout (after PHP):
;   5,s = channelMask (8-bit)
;------------------------------------------------------------------------------
hdmaEnable:
    php
    sep #$20

    ; Read current HDMAEN, OR with mask, write back
    ; Note: HDMAEN at $420C is write-only, so we track state in RAM
    lda 5,s                 ; channelMask
    ora.l hdma_enabled_state
    sta.l hdma_enabled_state
    sta.l $420C             ; REG_HDMAEN

    plp
    rtl

;------------------------------------------------------------------------------
; void hdmaDisable(u8 channelMask)
;
; Disables specified HDMA channels.
;------------------------------------------------------------------------------
hdmaDisable:
    php
    sep #$20

    lda 5,s                 ; channelMask
    eor #$FF                ; Invert to create AND mask
    and.l hdma_enabled_state
    sta.l hdma_enabled_state
    sta.l $420C

    plp
    rtl

;------------------------------------------------------------------------------
; void hdmaDisableAll(void)
;
; Disables all HDMA channels.
;------------------------------------------------------------------------------
hdmaDisableAll:
    php
    sep #$20

    lda #$00
    sta.l hdma_enabled_state
    sta.l $420C

    plp
    rtl

;------------------------------------------------------------------------------
; u8 hdmaGetEnabled(void)
;
; Returns current HDMA enable state.
;------------------------------------------------------------------------------
hdmaGetEnabled:
    php
    sep #$20

    lda.l hdma_enabled_state

    plp
    rtl

;------------------------------------------------------------------------------
; void hdmaSetTable(u8 channel, const void *table)
;
; Updates the table pointer for an HDMA channel.
;
; Stack layout (after PHP):
;   5-6,s = table (rightmost, 16-bit)
;   7-8,s = channel (leftmost, 8-bit in 16-bit slot)
;------------------------------------------------------------------------------
hdmaSetTable:
    php
    rep #$30

    ; Calculate register base
    sep #$20
    lda 7,s                 ; channel
    cmp #8
    bcs @done

    rep #$20
    and #$00FF
    asl a
    asl a
    asl a
    asl a
    clc
    adc #$4300
    tax

    ; Set table address
    lda 5,s                 ; table address
    sta.l $0002,x

    ; Set bank (same logic as hdmaSetup)
    sep #$20
    lda 6,s                 ; High byte of address
    cmp #$80
    bcc @use_ram
    lda #$00
    bra @set
@use_ram:
    lda #$7E
@set:
    sta.l $0004,x

@done:
    plp
    rtl

.ENDS

;------------------------------------------------------------------------------
; RAM for tracking HDMA state
;------------------------------------------------------------------------------
.RAMSECTION ".hdma_state" BANK 0 SLOT 1
    hdma_enabled_state: dsb 1
.ENDS
