;; Sprite tiles + palette for the aseprite_pipeline demo.
;;
;; ASSET_SECTION packs this payload into a high bank (never bank $00): the
;; tiles/palette are handed to dmaCopyVram/dmaCopyCGram, which read the far
;; pointer's bank byte, so the source can live anywhere. See the bank $00
;; budget rule and templates/assets.inc.
;;
;; res/hero.pic and res/hero.pal are produced by gfx4snes -P from res/hero.png
;; (the same call that emits res/hero_meta.inc, the metasprite pointer table).

ASSET_SECTION "hero"

hero_til:
.incbin "res/hero.pic"
hero_tilend:

hero_pal:
.incbin "res/hero.pal"
hero_palend:

.ENDS
