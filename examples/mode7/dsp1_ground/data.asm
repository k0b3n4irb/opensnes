; DSP-1 ground — asset data only. Everything dynamic (HDMA tables, matrices)
; is built in C from what the DSP-1 streams.
;
; ASSET_SECTION keeps the payload out of bank $00 (see templates/assets.inc):
; the lib DMA helpers read the bank byte of the far pointer C hands them.

;----------------------------------------------------------------------
; Ground: Mode 7 interleaved format (tilemap .mp7 + 8bpp tiles .pc7)
;----------------------------------------------------------------------
ASSET_SECTION "dsp1_ground_floor"

ground_tiles:
.incbin "res/ground.pc7"
ground_tiles_end:

ground_map:
.incbin "res/ground.mp7"
ground_map_end:

ground_pal:
.incbin "res/ground.pal"
ground_pal_end:

.ENDS

;----------------------------------------------------------------------
; Sky: Mode 3 BG2, 4bpp tiles + 64x32 tilemap (palette 0 = ground colours 0-15)
;----------------------------------------------------------------------
ASSET_SECTION "dsp1_ground_sky"

sky_tiles:
.incbin "res/sky.pic"
sky_tiles_end:

sky_map:
.incbin "res/sky.map"
sky_map_end:

.ENDS
