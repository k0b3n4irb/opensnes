;----------------------------------------------------------------------
; rpg — assets generated from the Tiled maps (res/town.tmj and
; res/house.tmj) by gen_assets.py. The maps are the single source of
; truth: terrain, per-tile collision and entity positions come from them.
;
; EVERY asset here is pinned OUT of bank $00. Nothing needs to be there:
; tiles, maps and palettes are handed to lib DMA functions, which take
; far pointers and honour the bank byte; the collision maps are read
; from C through `const` pointers, which post-#121 compile to far reads.
; Left as SUPERFREE the linker packs them into bank $00 — it is the
; first bank that fits — and this example was down to 12 free bytes
; there, with 12 KB of map data sitting in the code bank for no reason.
; See issue #127.
;----------------------------------------------------------------------

ASSET_SECTION "rodata_townmap"
town_map:    .incbin "res/town_map.bin"
town_map_end:
.ends

ASSET_SECTION "rodata_coll"
town_collision: .incbin "res/town_collision.bin"
town_collision_end:
.ends

ASSET_SECTION "rodata_interior"
house_map:   .incbin "res/house_map.bin"
house_map_end:
.ends

ASSET_SECTION "rodata_hcoll"
house_collision: .incbin "res/house_collision.bin"
house_collision_end:
.ends

ASSET_SECTION "rpgpal"
npc_pal:   .incbin "res/npc.pal"
npc_pal_end:
.ends
