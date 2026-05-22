;==============================================================================
; shmup_1942 — Graphics Data
;
; Asset-bundle symbols consumed by snes/asset.h's DECLARE_BG_ASSET (prefix
; `scene`). The .incbin files are produced by gfx4snes from res/scene.png,
; itself composed from ground.png by res/compose_scene.py.
;==============================================================================

.section ".rodata1" superfree

scene_tiles: .incbin "res/scene.pic"
scene_tiles_end:

scene_map: .incbin "res/scene.map"
scene_map_end:

scene_pal: .incbin "res/scene.pal"
scene_pal_end:

.ends
