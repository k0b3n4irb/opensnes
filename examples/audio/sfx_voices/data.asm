;----------------------------------------------------------------------
; sfx_voices - three filler "samples" for the piecewise-load recipe.
; Each is 4800 bytes (533 BRR blocks' worth of ROM, never played): bytes
; counted off the assembler's own INDEX so the content is a fixed, checkable
; pattern - the manifest compares them with ARAM after the load.
;----------------------------------------------------------------------

ASSET_SECTION ".rodata1"

fill_a:
.rept 4800 INDEX i
    .db (i * 7 + 1) & $FF
.endr
fill_a_end:

fill_b:
.rept 4800 INDEX i
    .db (i * 13 + 5) & $FF
.endr
fill_b_end:

.ends

ASSET_SECTION ".rodata2"

fill_c:
.rept 4800 INDEX i
    .db (i * 29 + 11) & $FF
.endr
fill_c_end:

.ends
