;==============================================================================
; libtest — map-module test data (issue #103 regression pin)
;
; Real tmx2snes output, shared with examples/maps/mapscroll (committed
; assets, referenced in place — no duplication in the repo). Pinned OUT of
; bank $00 on purpose: mapLoad honours the far pointer's bank byte (B1),
; and the #103 getters must work regardless of where the map data lives —
; the bug was in the WRAM-table reads, not the map reads.
;==============================================================================

.section ".rodata2" semifree bank 2

mapdata:
.incbin "../../examples/maps/mapscroll/res/BG1.m16"
mapdata_end:

tilesetdef:
.incbin "../../examples/maps/mapscroll/res/tiledMario.t16"
tilesetdef_end:

tilesetatt:
.incbin "../../examples/maps/mapscroll/res/tiledMario.b16"
tilesetatt_end:

.ends
