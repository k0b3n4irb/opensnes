;==============================================================================
; DynamicMap - ROM Data
;
; Sprite tile graphics (8bpp, 256 colors) and C64 sprite data.
;==============================================================================

;------------------------------------------------------------------------------
; Sprite graphics for 32x32 mode (8x8 tile size, 256 bytes per 16x16 sprite)
;------------------------------------------------------------------------------
.section ".rodata1" superfree

sprite16:
.incbin "res/sprite16.pic"
sprite16_end:

palsprite16:
.incbin "res/sprite16.pal"
palsprite16_end:

.ends

;------------------------------------------------------------------------------
; Sprite graphics for 64x64 mode (16x16 tile size, 2048 bytes per sprite)
;------------------------------------------------------------------------------
.section ".rodata2" superfree

sprite16_64x64:
.incbin "res/sprite16_64x64.pic"
sprite16_64x64_end:

palsprite16_64x64:
.incbin "res/sprite16_64x64.pal"
palsprite16_64x64_end:

.ends

;------------------------------------------------------------------------------
; C64 sprite data (from Boulder Dash, reverse engineered by DrHonz)
; https://csdb.dk/release/?id=145094
;
; C64 sprite format: 8x16 pixels (4 quarter tiles of 4x8 pixels each)
; 2-bit color depth per pixel, stretched to 16x16 on screen.
;------------------------------------------------------------------------------
.section ".rodata3" superfree

c64_sprite:
                  .byte $00 ; ........  top left
                  .byte $08 ; ....#...
                  .byte $0a ; ....#.#.
                  .byte $22 ; ..#...#.
                  .byte $22 ; ..#...#.
                  .byte $0a ; ....#.#.
                  .byte $02 ; ......#.
                  .byte $0a ; ....#.#.

                  .byte $00 ; ........  top right
                  .byte $20 ; ..#.....
                  .byte $a0 ; #.#.....
                  .byte $88 ; #...#...
                  .byte $88 ; #...#...
                  .byte $a0 ; #.#.#...
                  .byte $80 ; #.......
                  .byte $a0 ; #.#.....

.dsb 112, $00 ; 112 zero bytes (gap between top and bottom halves)

                  .byte $23 ; ..#...##  bottom left
                  .byte $32 ; ..##..#.
                  .byte $03 ; ......##
                  .byte $02 ; ......#.
                  .byte $07 ; .....###
                  .byte $04 ; .....#..
                  .byte $04 ; .....#..
                  .byte $3c ; ..####..

                  .byte $c8 ; ##..#...  bottom right
                  .byte $8c ; #...##..
                  .byte $c0 ; ##......
                  .byte $80 ; #.......
                  .byte $d0 ; ##.#....
                  .byte $10 ; ...#....
                  .byte $10 ; ...#....
                  .byte $3c ; ..####..

c64_sprite_end:

.ends
