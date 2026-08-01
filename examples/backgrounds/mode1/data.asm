;==============================================================================
; Mode 1 Background - Graphics Data
;
; Symbols follow the asset bundle convention used by snes/asset.h's
; DECLARE_BG_ASSET macro: <name>_tiles, <name>_pal, <name>_map plus the
; matching _end labels. Here the prefix is `bg` (single static background).
;==============================================================================

.section ".rodata1" superfree

;------------------------------------------------------------------------------
; Background tiles (4bpp, 16 colors)
;------------------------------------------------------------------------------
bg_tiles: .incbin "res/opensnes.pic"
bg_tiles_end:

;------------------------------------------------------------------------------
; Background tilemap
;------------------------------------------------------------------------------
bg_map: .incbin "res/opensnes.map"
bg_map_end:

;------------------------------------------------------------------------------
; Background palette (16 colors)
;------------------------------------------------------------------------------
bg_pal: .incbin "res/opensnes.pal"
bg_pal_end:

.ends
