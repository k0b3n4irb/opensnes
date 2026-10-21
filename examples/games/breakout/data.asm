;==============================================================================
; Breakout Game Assets
;==============================================================================

;------------------------------------------------------------------------------
; RAM Buffers — far RAM (bank $7E, above the 8 KB C band)
;------------------------------------------------------------------------------
; 4708 bytes of tilemap/palette/brick state. They used to sit in bank 0 at
; $0800 (ORGA, to dodge the OAM buffer) and left the C RAM band with 1436
; bytes free. Declared `FAR` on the C side, every access is
; bank-honouring — `blocks[b]` is an absolute-long-indexed read, the DMA
; helpers take the bank from the far pointer — and bank 0 gets the space
; back. SLOT 2 is $2000-$FFFF of bank $7E (templates/memmap.inc).
;------------------------------------------------------------------------------

.RAMSECTION ".game_buffers" BANK $7E SLOT 2
    blockmap    dsb $800    ; BG1 tilemap buffer (2KB)
    backmap     dsb $800    ; BG2 tilemap buffer (2KB)
    pal         dsb $200    ; Palette buffer (512 bytes)
    blocks      dsb 100     ; Current brick state (100 bytes)
.ENDS

;------------------------------------------------------------------------------
; String constants — read from C through const char * (far reads), so they
; live in the asset banks since 2026-09-23
;------------------------------------------------------------------------------
ASSET_SECTION ".strings"
str_ready:
.db "PLAYER 1", 10, 10, " READY", 0

str_gameover:
.db "GAME OVER", 0

str_paused:
.db "PAUSE", 0

str_blank:
.db "        ", 0

.ENDS

;------------------------------------------------------------------------------
; Default brick layout — read through a const pointer (far read)
; Values: 0-7 = brick colors, 8 = empty
; SLOT 0 = ROM area $8000-$FFFF in bank 0
;------------------------------------------------------------------------------
ASSET_SECTION ".brickmap"
brick_map:
.db 7, 8, 8, 8, 8, 8, 8, 8, 8, 7  ; Row 0
.db 8, 7, 8, 7, 8, 8, 7, 8, 7, 8  ; Row 1
.db 8, 8, 7, 8, 7, 7, 8, 7, 8, 8  ; Row 2
.db 8, 8, 8, 1, 3, 3, 1, 8, 8, 8  ; Row 3
.db 8, 0, 4, 8, 8, 8, 8, 4, 0, 8  ; Row 4
.db 8, 0, 8, 8, 5, 5, 8, 8, 0, 8  ; Row 5
.db 8, 0, 4, 8, 8, 8, 8, 4, 0, 8  ; Row 6
.db 8, 8, 8, 1, 3, 3, 1, 8, 8, 8  ; Row 7
.db 8, 6, 8, 6, 6, 8, 6, 8, 8, 7  ; Row 8
.db 7, 7, 7, 7, 8, 8, 7, 7, 7, 7  ; Row 9

.ENDS
ASSET_SECTION ".rodata1"
; Seven levels of BG3 colours (7 sets x 16 bytes = 112 bytes, CGRAM 16-23)
backpal:
.incbin "res/backpal.bin"
backpal_end:

; BG1 tilemap: the cabinet and the HUD labels (2KB)
bg1map:
.incbin "res/bg1map.bin"
bg1map_end:

; BG3 tilemap: the circuit board under the playfield (2KB)
bg2map:
.incbin "res/bg2map.bin"
bg2map_end:

.ends
