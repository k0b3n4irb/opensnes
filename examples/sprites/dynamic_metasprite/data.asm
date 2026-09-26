;==============================================================================
; Dynamic Metasprite Example - Graphics Data
;==============================================================================

ASSET_SECTION ".rodata1"

; 32x32 metasprite tiles (4bpp, transposed for OBJ VRAM layout)
; Source: spritehero32.png = 64x64 sheet, 3 animation frames
spritehero32_til:
.incbin "res/spritehero32.pic"
spritehero32_til_end:

; Sprite palette (shared by all 3 sprite sizes — same palette)
spritehero32_pal:
.incbin "res/spritehero32.pal"
spritehero32_pal_end:

.ends

ASSET_SECTION ".rodata2"

; 16x16 metasprite tiles (4bpp, transposed)
; Source: spritehero16.png = 32x48 sheet, 4 animation frames
spritehero16_til:
.incbin "res/spritehero16.pic"
spritehero16_til_end:

.ends

ASSET_SECTION ".rodata3"

; 8x8 metasprite tiles (4bpp, transposed)
; Source: spritehero8.png = 16x16 sheet, 4 animation frames
spritehero8_til:
.incbin "res/spritehero8.pic"
spritehero8_til_end:

.ends
