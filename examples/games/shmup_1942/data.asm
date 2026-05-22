;==============================================================================
; shmup_1942 — Graphics Data
;
; Asset-bundle symbols consumed by snes/asset.h's DECLARE_BG_ASSET (prefix
; `ground`). The .incbin files are produced by gfx4snes from res/ground.png.
;==============================================================================

.section ".rodata1" superfree

ground_tiles: .incbin "res/ground.pic"
ground_tiles_end:

ground_map: .incbin "res/ground.map"
ground_map_end:

ground_pal: .incbin "res/ground.pal"
ground_pal_end:

.ends
