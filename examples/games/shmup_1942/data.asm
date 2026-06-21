;==============================================================================
; shmup_1942 — Graphics Data
;
; Asset-bundle symbols consumed by snes/asset.h's DECLARE_BG_ASSET (prefix
; `scene`) plus a sprite tile + palette pair for the player ship.
;==============================================================================

.section ".rodata1" superfree

scene_tiles: .incbin "res/scene.pic"
scene_tiles_end:

scene_map: .incbin "res/scene.map"
scene_map_end:

scene_pal: .incbin "res/scene.pal"
scene_pal_end:

;------------------------------------------------------------------------------
; Player ship sprite (32×32, 4bpp, 16-colour palette).
; Tiles laid out for OBJ_SIZE32_L64 (small = 32×32 = our ship).
;------------------------------------------------------------------------------
player_tiles: .incbin "res/player.pic"
player_tiles_end:

player_pal: .incbin "res/player.pal"
player_pal_end:

;------------------------------------------------------------------------------
; Enemy ship sprite (32×32, 4bpp, 16-colour palette).
; Loaded to its own VRAM region so multiple enemies share one tile copy.
;------------------------------------------------------------------------------
enemy_tiles: .incbin "res/enemy.pic"
enemy_tiles_end:

enemy_pal: .incbin "res/enemy.pal"
enemy_pal_end:

;------------------------------------------------------------------------------
; Bullet sprite (32×32, mostly transparent — drawn programmatically by
; res/extract_sprites.py). Same VRAM-row stride as player/enemy.
;------------------------------------------------------------------------------
bullet_tiles: .incbin "res/bullet.pic"
bullet_tiles_end:

bullet_pal: .incbin "res/bullet.pal"
bullet_pal_end:

.ends
