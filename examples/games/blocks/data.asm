;==============================================================================
; Blocks Game Assets
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
; ROM assets — DMA sources and const-read strings, asset banks (2026-09-23)
;------------------------------------------------------------------------------
ASSET_SECTION ".rodata1"
font2bpp_gfx:
.incbin "res/font2bpp.bin"
font2bpp_gfx_end:

; 4bpp Palette 1 (CGRAM 16-31): the cabinet's frame (1-7, breakout's colours),
; the HUD's white font (11-12); colours 8-11 are also BG3's 2bpp palette 6,
; the messages: 9 their colour (cycled at runtime), 10 their shadow.
; Tiles and colours: devtools/art_assets/blocks.py (BORDER).
border_pal:
.dw $0000    ; idx 0  (CGRAM 16): transparent
.dw $1462    ; idx 1  (CGRAM 17): #101828, unused by the frame
.dw $1C85    ; idx 2  (CGRAM 18): #282438, the beam's outer edge
.dw $2CE8    ; idx 3  (CGRAM 19): #403858, steel
.dw $458D    ; idx 4  (CGRAM 20): #686088, steel
.dw $6AD7    ; idx 5  (CGRAM 21): #b8b0d0, the beam's lit ridge
.dw $495C    ; idx 6  (CGRAM 22): #e05090, the neon tube
.dw $6A9F    ; idx 7  (CGRAM 23): #f8a0d0, the tube's core
.dw $0000    ; idx 8  (CGRAM 24): BG3 palette 6 color 0 (transparent)
.dw $7FFD    ; idx 9  (CGRAM 25): BG3 palette 6 color 1, the message (cycled at runtime)
.dw $0821    ; idx 10 (CGRAM 26): BG3 palette 6 color 2, the message's shadow #080810
.dw $7FFD    ; idx 11 (CGRAM 27): #e8f8ff, the HUD font's body
.dw $28E5    ; idx 12 (CGRAM 28): #283850, the HUD font's shadow
.dw $0000    ; idx 13 (CGRAM 29): unused
.dw $0000    ; idx 14 (CGRAM 30): unused
.dw $0000    ; idx 15 (CGRAM 31): unused
border_pal_end:

; 4bpp Palette 2 (CGRAM 32-47): pink font for the SCORE label (#e05090)
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
.dw $495C    ; idx 11 (CGRAM 43): pink #e05090 (font body)
.dw $1448    ; idx 12 (CGRAM 44): dark pink (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
red_pal_end:

; 4bpp Palette 3 (CGRAM 48-63): cyan font for the LEVEL label (#48d0e0)
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
.dw $7349    ; idx 11 (CGRAM 59): cyan #48d0e0 (font body)
.dw $20C2    ; idx 12 (CGRAM 60): dark cyan (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
green_pal_end:

; 4bpp Palette 4 (CGRAM 64-79): violet font for the LINES label (#a888d8)
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
.dw $6E35    ; idx 11 (CGRAM 75): violet #a888d8 (font body)
.dw $1C65    ; idx 12 (CGRAM 76): dark violet (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
purple_pal_end:

; 4bpp Palette 5 (CGRAM 80-95): amber font for the NEXT label (#f0b050)
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
.dw $2ADE    ; idx 11 (CGRAM 91): amber #f0b050 (font body)
.dw $08A8    ; idx 12 (CGRAM 92): dark amber (font shadow)
.dw $0000    ; idx 13
.dw $0000    ; idx 14
.dw $0000    ; idx 15
orange_pal_end:

.ENDS

;------------------------------------------------------------------------------
; String constants
;------------------------------------------------------------------------------
ASSET_SECTION ".strings"
str_title:
.db "BLOCKS", 0

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
