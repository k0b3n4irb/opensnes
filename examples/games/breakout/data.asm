;==============================================================================
; Breakout Game Assets
;==============================================================================

;------------------------------------------------------------------------------
; RAM Buffers (placed at $0800 to avoid OAM buffer overlap at $0300-$051F)
;------------------------------------------------------------------------------
; OAM buffer is at $7E:0300-$051F (544 bytes), which mirrors to $00:0300-$051F.
; C static arrays were placed starting at $00A0, overlapping with OAM.
; Fix: explicitly place large buffers at $0800 (after OAM).
;------------------------------------------------------------------------------

.RAMSECTION ".game_buffers" BANK 0 SLOT 1 ORGA $0800 FORCE
    blockmap    dsb $800    ; BG1 tilemap buffer (2KB)
    backmap     dsb $800    ; BG2 tilemap buffer (2KB)
    pal         dsb $200    ; Palette buffer (512 bytes)
    blocks      dsb 100     ; Current brick state (100 bytes)
.ENDS

;------------------------------------------------------------------------------
; String constants (must be in bank 0 ROM section for correct addressing)
; SLOT 0 = ROM area $8000-$FFFF in bank 0
;------------------------------------------------------------------------------
.SECTION ".strings" SEMIFREE BANK 0 SLOT 0

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
; Default brick layout (must be in bank 0 ROM section for mycopy to work)
; Values: 0-7 = brick colors, 8 = empty
; SLOT 0 = ROM area $8000-$FFFF in bank 0
;------------------------------------------------------------------------------
.SECTION ".brickmap" SEMIFREE BANK 0 SLOT 0

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

.section ".rodata1" SEMIFREE BANK 0 SLOT 0

; Palette data (512 bytes - full 256 color palette)
palette:
.incbin "res/palette.dat"
palette_end:

; Background color palette variations (7 sets x 16 bytes = 112 bytes)
backpal:
.incbin "res/backpal.dat"
backpal_end:

; BG1 tilemap (foreground - bricks and HUD)
bg1map:
.incbin "res/bg1map.dat"
bg1map_end:

; BG2 tilemap (background patterns - 4 variations)
; Split into separate labels for each 0x800-byte pattern to avoid pointer arithmetic
bg2map:
bg2map0:
.incbin "res/bg2map.dat" SKIP 0 READ $800
bg2map1:
.incbin "res/bg2map.dat" SKIP $800 READ $800
bg2map2:
.incbin "res/bg2map.dat" SKIP $1000 READ $800
bg2map3:
.incbin "res/bg2map.dat" SKIP $1800 READ $800
bg2map_end:

; Tiles for BG1 (foreground graphics)
tiles1:
.incbin "res/tiles1.dat"
tiles1_end:

; Tiles for sprites (ball, paddle)
tiles2:
.incbin "res/tiles2.dat"
tiles2_end:

.ends
