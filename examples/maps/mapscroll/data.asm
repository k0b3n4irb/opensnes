;==============================================================================
; Map Scroll Example — Asset Data
;==============================================================================

;--- Tileset graphics and palette ---
.section ".rodata1" superfree

tileset:
.incbin "res/tilesMario.pic"
tileset_end:

tilesetpal:
.incbin "res/tilesMario.pal"
tilesetpal_end:

.ends

;--- Map data (from tmx2snes) ---
; B1 proof: pinned to bank 2 (out of bank $00). mapLoad + the scroll
; runtime now honour the pointer's bank byte, so the 25 KB map no longer
; has to sit in bank $00.
; /!\ bank 2 is for the MAP ENGINE (far pointers). Do NOT read these
; symbols directly from C: a near-pointer deref (`mapdata[6]`) silently
; reads bank $00. From C, consult the map via mapGetMetaTile()/
; mapGetMetaTilesProp() (bank-safe) — see mapLoad's doc in map.h.
.section ".rodata2" semifree bank 2

mapdata:
.incbin "res/BG1.m16"
mapdata_end:

tilesetatt:
.incbin "res/tiledMario.b16"

tilesetdef:
.incbin "res/tiledMario.t16"

.ends

;--- Sprite graphics ---
.section ".rodata3" superfree

gfxsprite:
.incbin "res/mario.pic"
gfxsprite_end:

palsprite:
.incbin "res/mario.pal"
palsprite_end:

.ends
