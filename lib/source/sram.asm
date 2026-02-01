;==============================================================================
; OpenSNES SRAM Functions (Assembly)
;
; Battery-backed SRAM save/load functions for LoROM cartridges.
;
; SRAM Memory Map (LoROM):
;   Bank $70: $0000-$7FFF (32KB SRAM)
;
; Calling convention (cdecl, right-to-left push):
;   - Rightmost argument is closest to SP after call
;   - Leftmost argument is furthest from SP
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

; SRAM bank for LoROM
.EQU SRAM_BANK $70

; Direct page temporaries
.EQU DP_SIZE   $00      ; 2 bytes - transfer size
.EQU DP_SRC    $02      ; 2 bytes - source pointer
.EQU DP_DEST   $04      ; 2 bytes - dest pointer
.EQU DP_TEMP   $06      ; 2 bytes - temporary

.SECTION ".sram_asm" SUPERFREE

;------------------------------------------------------------------------------
; void sramSave(u8 *data, u16 size)
;
; Copy data from Work RAM to SRAM using block move.
;
; Stack layout (after PHP/PHB):
;   1,s = P (processor status)
;   2,s = B (data bank)
;   3-5,s = return address (3 bytes from JSL)
;   6-7,s = size (rightmost arg)
;   8-9,s = data pointer (leftmost arg)
;------------------------------------------------------------------------------
sramSave:
    php
    phb

    rep #$30                    ; 16-bit A, X, Y

    lda 6,s                     ; size
    beq @done                   ; if size == 0, skip
    dec a                       ; MVN uses count-1
    sta.b DP_SIZE

    lda 8,s                     ; source pointer (WRAM)
    tax                         ; X = source

    ldy #$0000                  ; Y = dest (SRAM at $0000)

    lda.b DP_SIZE               ; A = byte count - 1

    ; Block move: source bank $7E, dest bank $70
    mvn $70, $7E

@done:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void sramLoad(u8 *data, u16 size)
;
; Copy data from SRAM to Work RAM using block move.
;
; Stack layout (after PHP/PHB):
;   6-7,s = size
;   8-9,s = data pointer
;------------------------------------------------------------------------------
sramLoad:
    php
    phb

    rep #$30                    ; 16-bit A, X, Y

    lda 6,s                     ; size
    beq @done                   ; if size == 0, skip
    dec a                       ; MVN uses count-1
    sta.b DP_SIZE

    ldx #$0000                  ; X = source (SRAM at $0000)

    lda 8,s                     ; dest pointer (WRAM)
    tay                         ; Y = dest

    lda.b DP_SIZE               ; A = byte count - 1

    ; Block move: source bank $70, dest bank $7E
    mvn $7E, $70

@done:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void sramSaveOffset(u8 *data, u16 size, u16 offset)
;
; Copy data from Work RAM to SRAM at specified offset.
;
; Stack layout (after PHP/PHB):
;   6-7,s = offset (rightmost)
;   8-9,s = size
;   10-11,s = data pointer (leftmost)
;------------------------------------------------------------------------------
sramSaveOffset:
    php
    phb

    rep #$30

    lda 8,s                     ; size
    beq @done
    dec a                       ; MVN uses count-1
    sta.b DP_SIZE

    lda 10,s                    ; source pointer (WRAM)
    tax                         ; X = source

    lda 6,s                     ; offset
    tay                         ; Y = dest (SRAM offset)

    lda.b DP_SIZE               ; A = byte count - 1

    ; Block move: source bank $7E, dest bank $70
    mvn $70, $7E

@done:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void sramLoadOffset(u8 *data, u16 size, u16 offset)
;
; Copy data from SRAM at specified offset to Work RAM.
;
; Stack layout (after PHP/PHB):
;   6-7,s = offset (rightmost)
;   8-9,s = size
;   10-11,s = data pointer (leftmost)
;------------------------------------------------------------------------------
sramLoadOffset:
    php
    phb

    rep #$30

    lda 8,s                     ; size
    beq @done
    dec a
    sta.b DP_SIZE

    lda 6,s                     ; offset
    tax                         ; X = source (SRAM offset)

    lda 10,s                    ; dest pointer (WRAM)
    tay                         ; Y = dest

    lda.b DP_SIZE               ; A = byte count - 1

    ; Block move: source bank $70, dest bank $7E
    mvn $7E, $70

@done:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void sramClear(u16 size)
;
; Clear SRAM to zero (byte-by-byte to avoid needing a source buffer).
;
; Stack layout (after PHP/PHB):
;   6-7,s = size
;------------------------------------------------------------------------------
sramClear:
    php
    phb

    rep #$30

    lda 6,s                     ; size
    beq @done
    sta.b DP_SIZE

    sep #$20                    ; 8-bit A
    lda #SRAM_BANK
    pha
    plb                         ; Set data bank to SRAM

    lda #$00                    ; Fill value
    rep #$10                    ; 16-bit X, Y
    ldy #$0000                  ; Start at offset 0

@clear_loop:
    sta $0000,y                 ; Store 0 to SRAM (bank register = $70)
    iny
    rep #$20
    tya
    cmp.b DP_SIZE
    sep #$20
    bcc @clear_loop

@done:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; u8 sramChecksum(u8 *data, u16 size)
;
; Calculate XOR checksum of data in WRAM.
; Returns 8-bit checksum in A (low byte).
;
; Stack layout (after PHP):
;   5-6,s = size (rightmost)
;   7-8,s = data pointer (leftmost)
;------------------------------------------------------------------------------
sramChecksum:
    php

    rep #$30

    lda 5,s                     ; size
    beq @zero
    sta.b DP_SIZE

    lda 7,s                     ; data pointer
    sta.b DP_SRC

    sep #$20
    lda #$00                    ; Initialize checksum
    sta.b DP_TEMP

    rep #$30
    ldx #$0000                  ; X = counter

@checksum_loop:
    ; Calculate source address and load byte
    txa
    clc
    adc.b DP_SRC                ; source + offset
    tay                         ; Y = address (in bank $7E)

    sep #$20
    lda.l $7E0000,x             ; Load byte - use X index
    ; We need to recalculate with proper address
    rep #$20
    txa
    clc
    adc.b DP_SRC
    tax                         ; X = actual address

    sep #$20
    lda.l $7E0000,x             ; Load byte from WRAM
    eor.b DP_TEMP               ; XOR with running checksum
    sta.b DP_TEMP

    rep #$20
    txa
    sec
    sbc.b DP_SRC                ; Get back to offset
    tax
    inx                         ; Next byte
    txa
    cmp.b DP_SIZE
    bcc @checksum_loop

    sep #$20
    lda.b DP_TEMP               ; Load final checksum
    rep #$20
    and #$00FF                  ; Ensure high byte is 0
    plp
    rtl

@zero:
    rep #$20
    lda #$0000
    plp
    rtl

.ENDS
