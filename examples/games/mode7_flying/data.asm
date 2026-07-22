;----------------------------------------------------------------------
; mode7_flying — terrain assets (gen_terrain.py + gfx4snes -M 7) and
; the landing class map with its banked-data accessor.
;----------------------------------------------------------------------

.section ".rodata1" superfree

terrain_til:
.incbin "res/terrain.pc7"
terrain_til_end:

terrain_pal:
.incbin "res/terrain.pal"
terrain_pal_end:

.ends

.section ".rodata2" superfree

terrain_map:
.incbin "res/terrain.mp7"
terrain_map_end:

.ends

.section ".rodata3" superfree

; 128x128 bytes: 0 = field, 1 = water, 2 = landing pad.
terrain_class:
.incbin "res/terrain_class.bin"
terrain_class_end:

.ends

;----------------------------------------------------------------------
; u8 terrain_class_at(u16 idx) — banked-data accessor (B2 escape,
; same pattern as mode7_racing: C pointer derefs are bank-$00-
; hardcoded, one absolute-long indexed load reads the real bank).
; cc65816 ABI: idx at 5,s after php alone; u8 return in tcc__r0.
;----------------------------------------------------------------------
.section ".terracc_text" superfree

terrain_class_at:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 5,s             ; idx
    tax
    sep #$20
    .ACCU 8
    lda.l terrain_class,x
    rep #$20
    .ACCU 16
    and #$00FF
    sta.b tcc__r0
    plp
    rtl

.ends
