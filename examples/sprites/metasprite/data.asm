;; Sprite tile data and palettes for metasprite demo
;; Each sprite sheet gets its own SUPERFREE section to avoid bank $00 overflow

.section ".rodata1" superfree

spritehero32_til:
.incbin "res/spritehero32.pic"
spritehero32_tilend:

spritehero32_pal:
.incbin "res/spritehero32.pal"
spritehero32_palend:

.ends

.section ".rodata2" superfree

spritehero16_til:
.incbin "res/spritehero16.pic"
spritehero16_tilend:

spritehero16_pal:
.incbin "res/spritehero16.pal"
spritehero16_palend:

.ends

.section ".rodata3" superfree

spritehero8_til:
.incbin "res/spritehero8.pic"
spritehero8_tilend:

spritehero8_pal:
.incbin "res/spritehero8.pal"
spritehero8_palend:

.ends
