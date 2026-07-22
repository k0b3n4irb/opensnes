;----------------------------------------------------------------------
; HiColor x pseudo-hires — Mode 1 dual-BG data (original 512x224 art)
;
; devtools/hicolor64hires.py splits the image by column parity: odd
; columns -> BG1 (main), even -> BG2 (sub); SETINI pseudo-hires
; re-interleaves them into 512 pixels. One shared 3584-byte palette
; streamed per scanline by the H-IRQ handler (hicolor_1792's).
;----------------------------------------------------------------------

.section ".rodata1" superfree

bg1_tiles:      .incbin "res/sunset_bg1.pic"
bg1_tiles_end:

.ends

.section ".rodata2" superfree

bg2_tiles:      .incbin "res/sunset_bg2.pic"
bg2_tiles_end:

.ends

.section ".rodata3" superfree

hicolor_pal:    .incbin "res/sunset.pal"
hicolor_pal_end:

.ends
