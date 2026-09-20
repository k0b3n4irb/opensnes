;==============================================================================
; OpenSNES SRAM Functions (Assembly)
;
; Battery-backed SRAM save/load functions for LoROM cartridges.
;
; SRAM Memory Map (LoROM):
;   Bank $70: $0000-$7FFF (32KB SRAM)
;
; Calling convention (cc65816, arguments pushed LEFT-TO-RIGHT):
;   - Rightmost argument is closest to SP after call
;   - Leftmost argument is furthest from SP
;   - A pointer is a 4-byte far slot: address, then bank byte, then padding
;==============================================================================

.ifdef SA1
.include "memmap_sa1.inc"
.else
.ifdef HIROM
.include "memmap_hirom.inc"
.else
.include "memmap.inc"
.endif
.endif

; Where the battery RAM sits, per mapping (fullsnes, "SNES Memory Map /
; Battery-backed SRAM": "HiROM ---> SRAM at 30h-3Fh,B0h-BFh:6000h-7FFFh ;small
; 8K SRAM bank(s) / LoROM ---> SRAM at 70h-7Dh,F0h-FFh:0000h-7FFFh ;big 32K
; SRAM bank(s)"). Until 2026-09-20 the LoROM values were used for every
; build, so a HiROM ROM saved into open bus and loaded garbage.
;
; HiROM exposes 8 KB per bank: this module addresses the FIRST bank only, so
; offset + size must stay within $2000 (the default SRAM_SIZE, 8 KB). Larger
; HiROM saves would have to step through banks $31-$3F; not implemented.
;
; SA-1 is not supported here: its save memory is BW-RAM ($40-$4F), which the
; SNES CPU may only write after enabling SBWE ($2226), and crt0 does not.
; make/common.mk refuses USE_SRAM=1 with USE_SA1=1 instead of building a
; module that would silently do nothing.
.ifdef HIROM
.EQU SRAM_BANK $30
.EQU SRAM_BASE $6000
.else
.EQU SRAM_BANK $70
.EQU SRAM_BASE $0000
.endif
.EQU SRAM_LONG (SRAM_BANK << 16) + SRAM_BASE

; Direct page temporaries
.EQU DP_SIZE   $00      ; 2 bytes - transfer size
.EQU DP_SRC    $02      ; 2 bytes - source pointer
.EQU DP_DEST   $04      ; 2 bytes - dest pointer
.EQU DP_TEMP   $06      ; 2 bytes - temporary

;------------------------------------------------------------------------------
; Block Copy Safety Macros
;------------------------------------------------------------------------------
.include "blockcopy.inc"

;------------------------------------------------------------------------------
; Bank-honouring block copies (2026-09-20)
;
; The four copy routines used a literal `mvn $7E, $70` / `mvn $70, $7E`: the
; bank byte of the far pointer they are handed was never read, so a save
; template declared `static const` — asset banks since #127.3 — was saved as
; whatever WRAM held at the same offset. MVN's banks are immediate operands,
; so they cannot follow a pointer; the macros keep MVN for the two banks that
; mean work RAM ($00, the low mirror, and $7E) and fall back to a long-
; indirect byte loop for anything else (ROM on save, $7F on load).
;
; In:  X = CPU-side address, Y = SRAM offset (SAVE) / X = SRAM offset,
;      Y = CPU-side address (LOAD); DP_SIZE = byte count - 1; 16-bit A/X/Y.
;      \1 = stack offset of the pointer's bank byte.
;------------------------------------------------------------------------------
.MACRO SRAM_SAVE_BLOCK
    sep #$20
    .ACCU 8
    lda \1,s                    ; bank byte of the source pointer
    beq @ssb_fast\@
    cmp #$7E
    beq @ssb_fast\@
    sta.b DP_SRC+2              ; [DP_SRC] = 24-bit source
    rep #$20
    .ACCU 16
    stx.b DP_SRC
    inc.b DP_SIZE               ; count-1 -> count
    tyx                         ; X = SRAM offset
    ldy #$0000
    sep #$20
    .ACCU 8
@ssb_loop\@:
    lda [DP_SRC],y
    sta.l SRAM_LONG,x
    inx
    iny
    cpy.b DP_SIZE
    bne @ssb_loop\@
    bra @ssb_done\@
@ssb_fast\@:
    rep #$20
    .ACCU 16
    tya
    clc
    adc #SRAM_BASE              ; SRAM offset -> CPU address in the SRAM bank
    tay
    lda.b DP_SIZE               ; A = byte count - 1
    mvn $7E, SRAM_BANK          ; WLA-DX: mvn src_bank, dst_bank
@ssb_done\@:
.ENDM

.MACRO SRAM_LOAD_BLOCK
    sep #$20
    .ACCU 8
    lda \1,s                    ; bank byte of the destination pointer
    beq @slb_fast\@
    cmp #$7E
    beq @slb_fast\@
    sta.b DP_SRC+2              ; [DP_SRC] = 24-bit destination
    rep #$20
    .ACCU 16
    sty.b DP_SRC
    inc.b DP_SIZE               ; count-1 -> count
    ldy #$0000                  ; X = SRAM offset already
    sep #$20
    .ACCU 8
@slb_loop\@:
    lda.l SRAM_LONG,x
    sta [DP_SRC],y
    inx
    iny
    cpy.b DP_SIZE
    bne @slb_loop\@
    bra @slb_done\@
@slb_fast\@:
    rep #$20
    .ACCU 16
    txa
    clc
    adc #SRAM_BASE              ; SRAM offset -> CPU address in the SRAM bank
    tax
    lda.b DP_SIZE               ; A = byte count - 1
    mvn SRAM_BANK, $7E          ; WLA-DX: mvn src_bank, dst_bank
@slb_done\@:
.ENDM

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
;   6-7,s  = size (rightmost arg)
;   8-11,s = data pointer (leftmost arg; 4-byte far pointer, bank at 10,s)
;------------------------------------------------------------------------------
sramSave:
    php
    phb

    rep #$30                    ; 16-bit A, X, Y
    .ACCU 16
    .INDEX 16

    lda 6,s                     ; size
    beq @done                   ; if size == 0, skip
    dec a                       ; MVN uses count-1
    sta.b DP_SIZE

    lda 8,s                     ; source pointer (WRAM)
    tax                         ; X = source

    ldy #$0000                  ; Y = dest (SRAM at $0000)

    SRAM_SAVE_BLOCK 10          ; pointer at 8-11,s: bank byte at 10,s

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
;   6-7,s  = size
;   8-11,s = data pointer (4-byte far pointer, bank at 10,s)
;------------------------------------------------------------------------------
sramLoad:
    php
    phb

    rep #$30                    ; 16-bit A, X, Y
    .ACCU 16
    .INDEX 16

    lda 6,s                     ; size
    beq @done                   ; if size == 0, skip
    dec a                       ; MVN uses count-1
    sta.b DP_SIZE

    ldx #$0000                  ; X = source (SRAM at $0000)

    lda 8,s                     ; dest pointer (WRAM)
    tay                         ; Y = dest

    SRAM_LOAD_BLOCK 10          ; pointer at 8-11,s: bank byte at 10,s

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
;   6-7,s   = offset (rightmost)
;   8-9,s   = size
;   10-13,s = data pointer (leftmost; 4-byte far pointer, bank at 12,s)
;------------------------------------------------------------------------------
sramSaveOffset:
    php
    phb

    rep #$30
    .ACCU 16
    .INDEX 16

    lda 8,s                     ; size
    beq @done
    dec a                       ; MVN uses count-1
    sta.b DP_SIZE

    lda 10,s                    ; source pointer (WRAM)
    tax                         ; X = source

    lda 6,s                     ; offset
    tay                         ; Y = dest (SRAM offset)

    SRAM_SAVE_BLOCK 12          ; pointer at 10-13,s: bank byte at 12,s

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
;   6-7,s   = offset (rightmost)
;   8-9,s   = size
;   10-13,s = data pointer (leftmost; 4-byte far pointer, bank at 12,s)
;------------------------------------------------------------------------------
sramLoadOffset:
    php
    phb

    rep #$30
    .ACCU 16
    .INDEX 16

    lda 8,s                     ; size
    beq @done
    dec a
    sta.b DP_SIZE

    lda 6,s                     ; offset
    tax                         ; X = source (SRAM offset)

    lda 10,s                    ; dest pointer (WRAM)
    tay                         ; Y = dest

    SRAM_LOAD_BLOCK 12          ; pointer at 10-13,s: bank byte at 12,s

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
    .ACCU 16
    .INDEX 16

    lda 6,s                     ; size
    beq @done
    sta.b DP_SIZE

    sep #$20                    ; 8-bit A
    .ACCU 8
    lda #SRAM_BANK
    pha
    plb                         ; Set data bank to SRAM

    lda #$00                    ; Fill value
    rep #$10                    ; 16-bit X, Y
    .INDEX 16
    ldy #$0000                  ; Start at offset 0

@clear_loop:
    sta.w SRAM_BASE,y           ; Store 0 to SRAM (data bank = SRAM_BANK)
    iny
    ; Compare the 16-bit index directly: the previous `tya / cmp` swapped
    ; the counter into A, so from byte 1 on the loop stored the OFFSET
    ; instead of zero (0,1,2,3,... — found by the libtest round trip,
    ; 2026-09-15). A stays $00 for the whole loop now.
    cpy.b DP_SIZE
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
;   5-6,s  = size (rightmost)
;   7-10,s = data pointer (leftmost; 4-byte far pointer, bank at 9,s)
;------------------------------------------------------------------------------
sramChecksum:
    php

    rep #$30
    .ACCU 16
    .INDEX 16

    lda 5,s                     ; size
    beq @zero
    sta.b DP_SIZE

    lda 7,s                     ; data pointer (4-byte far pointer at 7-10,s)
    sta.b DP_SRC

    sep #$20
    .ACCU 8
    lda 9,s                     ; its bank byte: the data may be const (ROM)
    sta.b DP_SRC+2              ; [DP_SRC] = 24-bit source
    lda #$00                    ; running checksum
    ldy #$0000

    ; One long-indirect read per byte. The old loop hard-coded bank $7E
    ; (`lda.l $7E0000,x`), loaded every byte twice and rebuilt the offset
    ; from the address with a subtraction each pass.
@checksum_loop:
    eor [DP_SRC],y
    iny
    cpy.b DP_SIZE
    bne @checksum_loop

    rep #$20
    .ACCU 16
    and #$00FF                  ; u8 result, clean high byte
    plp
    rtl

@zero:
    rep #$20
    .ACCU 16
    lda #$0000
    plp
    rtl

.ENDS
