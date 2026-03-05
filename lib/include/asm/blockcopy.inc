;==============================================================================
; OpenSNES Block Copy Safety Macros
;==============================================================================
;
; Self-documenting MVN/MVP macros that make operand order impossible to get
; wrong. Use these instead of raw MVN instructions.
;
; WLA-DX MVN syntax reminder:
;   mvn src_bank, dst_bank    (WDC convention: source first, dest second)
;
; The 65816 MVN instruction:
;   - X = source address (within source bank)
;   - Y = destination address (within destination bank)
;   - A = byte count - 1
;   - After execution: X, Y point past the last byte transferred,
;     DBR is set to the destination bank.
;
; IMPORTANT: Caller must save/restore DBR (phb/plb) around these macros,
; since MVN changes DBR to the destination bank.
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

;------------------------------------------------------------------------------
; BLOCK_COPY_7E_TO_00 src_addr, dst_addr, size
;
; Copy 'size' bytes from Bank $7E to Bank $00.
; Used for: objbuffers → objWorkspace (workspace sync)
;
; Requires: 16-bit A, X, Y (rep #$30)
; Clobbers: A, X, Y, DBR
;------------------------------------------------------------------------------
.MACRO BLOCK_COPY_7E_TO_00
    ldx \1                      ; X = source address (Bank $7E)
    ldy \2                      ; Y = dest address (Bank $00)
    lda #\3 - 1                 ; A = byte count - 1
    mvn $7E, $00                ; WLA-DX: mvn src_bank, dst_bank
.ENDM

;------------------------------------------------------------------------------
; BLOCK_COPY_00_TO_7E src_addr, dst_addr, size
;
; Copy 'size' bytes from Bank $00 to Bank $7E.
; Used for: objWorkspace → objbuffers (workspace sync)
;
; Requires: 16-bit A, X, Y (rep #$30)
; Clobbers: A, X, Y, DBR
;------------------------------------------------------------------------------
.MACRO BLOCK_COPY_00_TO_7E
    ldx \1                      ; X = source address (Bank $00)
    ldy \2                      ; Y = dest address (Bank $7E)
    lda #\3 - 1                 ; A = byte count - 1
    mvn $00, $7E                ; WLA-DX: mvn src_bank, dst_bank
.ENDM

;------------------------------------------------------------------------------
; BLOCK_COPY src_bank, src_addr, dst_bank, dst_addr, size
;
; Generic block copy between arbitrary banks.
; Used for: SRAM save/load (banks $7E ↔ $70)
;
; Requires: 16-bit A, X, Y (rep #$30)
; Clobbers: A, X, Y, DBR
;------------------------------------------------------------------------------
.MACRO BLOCK_COPY
    ldx \2                      ; X = source address
    ldy \4                      ; Y = dest address
    lda #\5 - 1                 ; A = byte count - 1
    mvn \1, \3                  ; WLA-DX: mvn src_bank, dst_bank
.ENDM
