;==============================================================================
; Animation Example - Assembly Helpers
;
; This file provides:
; - RAM variables for game state
; - Sprite tile lookup table
; - game_init function for state initialization
;
; Other functions (input, OAM, VBlank) are now handled by the library.
;==============================================================================

;------------------------------------------------------------------------------
; RAM Variables (in low RAM)
;------------------------------------------------------------------------------
.RAMSECTION ".game_vars" BANK 0 SLOT 1
    monster_x       dsb 2
    monster_y       dsb 2
    monster_state   dsb 1
    monster_anim    dsb 1
    monster_flipx   dsb 1
    monster_tile    dsb 1
.ENDS

;------------------------------------------------------------------------------
; Sprite tile lookup table
;------------------------------------------------------------------------------
.SECTION ".sprite_data" SUPERFREE
sprite_tiles:
    .db 0, 2, 4      ; Walk down frames (state 0)
    .db 6, 8, 10     ; Walk up frames (state 1)
    .db 12, 14, 32   ; Walk right/left frames (state 2)
.ENDS

;------------------------------------------------------------------------------
; game_init - Initialize game state variables
;------------------------------------------------------------------------------
.SECTION ".game_init" SUPERFREE
game_init:
    php
    rep #$30

    ; Initialize monster position - centered on screen
    ; Screen is 256x224, sprite is 16x16
    ; X = (256-16)/2 = 120, Y = (224-16)/2 = 104
    lda #120
    sta.l monster_x
    lda #104
    sta.l monster_y

    sep #$20
    lda #0
    sta.l monster_state
    sta.l monster_anim
    sta.l monster_flipx
    sta.l monster_tile

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; calc_tile - Calculate sprite tile from state and animation
; tile = sprite_tiles[state * 3 + anim]
;------------------------------------------------------------------------------
.SECTION ".calc_tile" SUPERFREE
calc_tile:
    php
    sep #$20
    rep #$10

    ; Calculate state * 3 using shifts and add (avoids __mul16)
    ; state * 3 = state * 2 + state = (state << 1) + state
    lda.l monster_state
    asl a               ; state * 2
    clc
    adc.l monster_state ; state * 3
    clc
    adc.l monster_anim  ; + anim
    and #$0F            ; Clamp to valid range (0-8)
    tax
    lda.l sprite_tiles,x
    sta.l monster_tile

    plp
    rtl
.ENDS
