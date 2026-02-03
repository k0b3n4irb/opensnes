;==============================================================================
; HDMA Gradient Example - Graphics Data
;==============================================================================

.section ".rodata1" superfree

;------------------------------------------------------------------------------
; Background tiles (4bpp, 16 colors)
;------------------------------------------------------------------------------
tiles: .incbin "res/pvsneslib.pic"
tiles_end:

;------------------------------------------------------------------------------
; Background tilemap
;------------------------------------------------------------------------------
tilemap: .incbin "res/pvsneslib.map"
tilemap_end:

;------------------------------------------------------------------------------
; Background palette (16 colors)
;------------------------------------------------------------------------------
palette: .incbin "res/pvsneslib.pal"
palette_end:

;------------------------------------------------------------------------------
; HDMA Gradient Table
;
; For a clean gradient, we set ALL color channels (R+G+B) to the same value.
; This creates a brightness gradient from dark to bright.
;
; COLDATA format ($2132):
;   Bit 7: Blue, Bit 6: Green, Bit 5: Red
;   Bits 4-0: Intensity (0-31)
;
; $E0 = bits 7,6,5 all set = write to R, G, and B simultaneously
;------------------------------------------------------------------------------
hdma_gradient_table:
    ; Top - dark (low intensity on all channels)
    .db 14, $E0 | 0       ; All channels = 0
    .db 14, $E0 | 1       ; All channels = 1
    .db 14, $E0 | 2       ; All channels = 2
    .db 14, $E0 | 3       ; All channels = 3

    ; Upper middle - getting brighter
    .db 14, $E0 | 4
    .db 14, $E0 | 5
    .db 14, $E0 | 6
    .db 14, $E0 | 7

    ; Middle
    .db 14, $E0 | 8
    .db 14, $E0 | 9
    .db 14, $E0 | 10
    .db 14, $E0 | 11

    ; Lower - bright
    .db 14, $E0 | 12
    .db 14, $E0 | 13
    .db 14, $E0 | 14
    .db 28, $E0 | 15      ; Last section

    .db 0                 ; End of table

.ends
