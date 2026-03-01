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

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
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
; Stack layout (after PHP/PHB):
;   1,s   = P (processor status, from PHP)
;   2,s   = B (data bank, from PHB)
;   3-5,s = return address (3 bytes from JSL)
;   6-7,s = msg pointer (16-bit, Bank $00)
;
; The message pointer is in Bank $00 (C compiler generates Bank $00 addresses).
; We use long addressing with Bank $00 to read the string bytes.
;------------------------------------------------------------------------------
consoleNocashMessage:
    php
    phb

    rep #$30                    ; 16-bit A, X, Y

    lda 6,s                     ; msg pointer (16-bit address in Bank $00)
    tax                         ; X = current character pointer

    sep #$20                    ; 8-bit A for character reads

@loop:
    lda.l $000000,x             ; read byte from Bank $00 + X
    beq @done                   ; null terminator? stop
    sta.l REG_DEBUG             ; write character to debug port
    inx
    bra @loop

@done:
    plb
    plp
    rtl

.ENDS
