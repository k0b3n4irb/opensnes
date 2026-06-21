;==============================================================================
; Tiled Map Example — Asset Data
;==============================================================================

;------------------------------------------------------------------------------
; Tileset graphics and palette (from gfx4snes)
;------------------------------------------------------------------------------
.section ".rodata1" superfree

tileset:
.incbin "res/tileslevel1.pic"
tileset_end:

tilesetpal:
.incbin "res/tileslevel1.pal"
tilesetpal_end:

.ends

;------------------------------------------------------------------------------
; Map data (from tmxconv / Tiled editor)
;
; BG1.m16:        Full tilemap (224x30 tiles, 16-bit entries)
; maplevel01.t16: Metatile definitions (4 tiles per metatile)
; maplevel01.b16: Tile attributes (collision properties per metatile)
;------------------------------------------------------------------------------
; B1: pinned out of bank $00 (bank 2). mapLoad + the scroll runtime honour
; the pointer's bank byte, so the 13 KB map need not sit in bank $00.
.section ".rodata2" semifree bank 2

mapdata:
.incbin "res/BG1.m16"
mapdata_end:

tilesetatt:
.incbin "res/maplevel01.b16"
tilesetatt_end:

tilesetdef:
.incbin "res/maplevel01.t16"
tilesetdef_end:

.ends
