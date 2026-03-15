;==============================================================================
; Tetris Game Assets
;==============================================================================

;------------------------------------------------------------------------------
; RAM Buffers — 2 full tilemap buffers + 1 small message row buffer
; Placed at $0800+ to avoid OAM buffer overlap at $0300-$051F
; Stack starts at $1FFF downward — must not overlap!
;
; Layout:
;   $0800-$0FFF: BG1 tilemap (playfield)   2048 bytes
;   $1000-$17FF: BG2 tilemap (HUD)         2048 bytes
;   $1800-$183F: BG3 message row buffer       64 bytes (1 row = 32 entries)
;   $1840-$1FFF: stack (~1984 bytes)
;------------------------------------------------------------------------------

.RAMSECTION ".buf_bg1" BANK 0 SLOT 1 ORGA $0800 FORCE
    tilemap_bg1  dsb $800    ; BG1: playfield (border + blocks) — 2KB
.ENDS

.RAMSECTION ".buf_bg2" BANK 0 SLOT 1 ORGA $1000 FORCE
    tilemap_bg2  dsb $800    ; BG2: HUD (labels + values + next box) — 2KB
.ENDS

.RAMSECTION ".buf_bg3_row" BANK 0 SLOT 1 ORGA $1800 FORCE
    bg3_msgrow   dsb 64      ; BG3: single message row (32 x u16) — 64 bytes
.ENDS

.RAMSECTION ".hdma_grad" BANK 0 SLOT 1 ORGA $1840 FORCE
    hdma_grad_tbl dsb 288    ; HDMA gradient table (56 entries × 5 bytes + terminator)
.ENDS

;------------------------------------------------------------------------------
; ROM assets (bank 0 for C accessibility)
;------------------------------------------------------------------------------
.SECTION ".rodata1" SEMIFREE BANK 0 SLOT 0

tiles_gfx:
.incbin "res/tiles.pic"
tiles_gfx_end:

tiles_pal:
.incbin "res/tiles.pal"
tiles_pal_end:

font2bpp_gfx:
.incbin "res/font2bpp.bin"
font2bpp_gfx_end:

; 4bpp Palette 1 (CGRAM 16-31): border grey gradient + HUD font colors
; Also serves as BG3 2bpp palette 4 (color 0=transparent, 1=white)
border_pal:
.dw $0000    ; idx 0  (CGRAM 16): transparent
.dw $7FFF    ; idx 1  (CGRAM 17): white (BG3 font color 1)
.dw $6739    ; idx 2  (CGRAM 18): light grey (border)
.dw $4E73    ; idx 3  (CGRAM 19): medium grey (border)
.dw $3DEF    ; idx 4  (CGRAM 20): medium-dark grey (border)
.dw $2D6B    ; idx 5  (CGRAM 21): dark grey (border)
.dw $1CE7    ; idx 6  (CGRAM 22): darker grey (border)
.dw $0421    ; idx 7  (CGRAM 23): near black (border)
.dw $0000    ; idx 8  (CGRAM 24): BG3 palette 6 color 0 (transparent)
.dw $7FFF    ; idx 9  (CGRAM 25): BG3 palette 6 color 1 (white — cycled at runtime)
.dw $0000    ; idx 10 (CGRAM 26): unused
.dw $7FFF    ; idx 11 (CGRAM 27): white (font body)
.dw $4C21    ; idx 12 (CGRAM 28): mid-blue (font shadow)
.dw $0000    ; idx 13 (CGRAM 29): unused
.dw $0000    ; idx 14 (CGRAM 30): unused
.dw $0000    ; idx 15 (CGRAM 31): unused
border_pal_end:

; 4bpp Palette 2 (CGRAM 32-47): magenta font for SCORE label (#e71861)
; Only indices 11-12 used (font body + shadow)
red_pal:
.dw $0000    ; idx 0  (CGRAM 32): transparent
.dw $0000    ; idx 1
.dw $0000    ; idx 2
.dw $0000    ; idx 3
.dw $0000    ; idx 4
.dw $0000    ; idx 5
.dw $0000    ; idx 6
.dw $0000    ; idx 7
.dw $0000    ; idx 8
.dw $0000    ; idx 9
.dw $0000    ; idx 10
.dw $307C    ; idx 11 (CGRAM 43): bright magenta #e71861 (font body)
.dw $102B    ; idx 12 (CGRAM 44): dark magenta (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
red_pal_end:

; 4bpp Palette 3 (CGRAM 48-63): green font for LEVEL label (#0bdb52)
; Only indices 11-12 used (font body + shadow)
green_pal:
.dw $0000    ; idx 0  (CGRAM 48): transparent
.dw $0000    ; idx 1
.dw $0000    ; idx 2
.dw $0000    ; idx 3
.dw $0000    ; idx 4
.dw $0000    ; idx 5
.dw $0000    ; idx 6
.dw $0000    ; idx 7
.dw $0000    ; idx 8
.dw $0000    ; idx 9
.dw $0000    ; idx 10
.dw $2B61    ; idx 11 (CGRAM 59): neon green #0bdb52 (font body)
.dw $1160    ; idx 12 (CGRAM 60): dark green (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
green_pal_end:

; 4bpp Palette 4 (CGRAM 64-79): purple font for LINES label (#7161de)
; Only indices 11-12 used (font body + shadow)
purple_pal:
.dw $0000    ; idx 0  (CGRAM 64): transparent
.dw $0000    ; idx 1
.dw $0000    ; idx 2
.dw $0000    ; idx 3
.dw $0000    ; idx 4
.dw $0000    ; idx 5
.dw $0000    ; idx 6
.dw $0000    ; idx 7
.dw $0000    ; idx 8
.dw $0000    ; idx 9
.dw $0000    ; idx 10
.dw $6E2E    ; idx 11 (CGRAM 75): purple #7161de (font body)
.dw $3408    ; idx 12 (CGRAM 76): dark purple (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
purple_pal_end:

; 4bpp Palette 5 (CGRAM 80-95): orange font for NEXT label (#dc6f0f)
; Only indices 11-12 used (font body + shadow)
orange_pal:
.dw $0000    ; idx 0  (CGRAM 80): transparent
.dw $0000    ; idx 1
.dw $0000    ; idx 2
.dw $0000    ; idx 3
.dw $0000    ; idx 4
.dw $0000    ; idx 5
.dw $0000    ; idx 6
.dw $0000    ; idx 7
.dw $0000    ; idx 8
.dw $0000    ; idx 9
.dw $0000    ; idx 10
.dw $01BC    ; idx 11 (CGRAM 91): orange #dc6f0f (font body)
.dw $0097    ; idx 12 (CGRAM 92): dark orange (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
orange_pal_end:

.ENDS

;------------------------------------------------------------------------------
; String constants
;------------------------------------------------------------------------------
.SECTION ".strings" SEMIFREE BANK 0 SLOT 0

str_tetris:
.db "TETRIS", 0

str_score:
.db "SCORE", 0

str_level:
.db "LEVEL", 0

str_lines:
.db "LINES", 0

str_next:
.db "NEXT", 0

str_paused:
.db "PAUSED", 0

str_gameover:
.db "GAME OVER", 0

str_start:
.db "PRESS START", 0

.ENDS
