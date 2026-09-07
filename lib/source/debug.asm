;==============================================================================
; OpenSNES Debug Utilities
;==============================================================================
;
; Provides emulator debug features:
;   - Mesen2 breakpoint (WDM $00)
;   - Nocash debug message output ($21FC)
;
; Based on: PVSnesLib consoles.asm by Alekmaul (lines 112-155)
; Simplified: no vsprintf/variadic — const string only.
;
; Author: OpenSNES Team
; License: MIT (original code zlib by Alekmaul)
;
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

;------------------------------------------------------------------------------
; Constants
;------------------------------------------------------------------------------

.EQU REG_DEBUG      $21FC       ; Nocash/Mesen2 debug message port

;==============================================================================
; CODE
;==============================================================================

.SECTION ".debug_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void consoleMesenBreakpoint(void)
;
; Emit WDM $00 which Mesen2 treats as a debugger breakpoint.
; On real hardware, WDM is a 2-byte NOP (does nothing).
;------------------------------------------------------------------------------
consoleMesenBreakpoint:
    .db $42, $00                ; WDM $00 — Mesen2 breakpoint signature
    rtl

;------------------------------------------------------------------------------
; void consoleNocashMessage(const char *msg)
;
; Write a null-terminated string to the Nocash debug port ($21FC).
; Each byte is written individually; the null terminator is NOT sent.
;
;Stack layout (after PHP/PHB):
;   1,s   = P (processor status, from PHP)
;   2,s   = B (data bank, from PHB)
;   3-5,s = return address (3 bytes from JSL)
;   6-7,s = msg pointer low 16 bits
;   8,s   = msg pointer bank byte (post-A6 4-byte pointer)
;
; The string is read through a 24-bit pointer: since #127.3 string
; literals live in the asset banks (bank $07 on a LoROM map), and the old
; `lda.l $000000,x` bank-$00 read returned garbage — caught by the
; debug_channel runtime fixture at the flip.
;------------------------------------------------------------------------------
consoleNocashMessage:
    php
    phb

    rep #$30                    ; 16-bit A, X, Y
    .ACCU 16
    .INDEX 16

    lda 6,s                     ; msg pointer low 16
    sta.b tcc__r0
    lda 8,s                     ; msg pointer bank byte (high byte = pad)
    and #$00FF
    sta.b tcc__r0+2
    ldy #0

    sep #$20                    ; 8-bit A for character reads
    .ACCU 8

@loop:
    lda [tcc__r0],y             ; read byte through the 24-bit pointer
    beq @done                   ; null terminator? stop
    sta.l REG_DEBUG             ; write character to debug port
    iny
    bra @loop

@done:
    plb
    plp
    rtl

.ENDS
