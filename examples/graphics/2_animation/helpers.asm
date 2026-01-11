;==============================================================================
; Animation Example - Assembly Helpers
;==============================================================================

;------------------------------------------------------------------------------
; RAM Variables (in low RAM for direct page access)
;------------------------------------------------------------------------------
.RAMSECTION ".game_vars" BANK 0 SLOT 1
    pad_current     dsb 2
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
    .db 0, 2, 4      ; Walk down frames
    .db 6, 8, 10     ; Walk up frames
    .db 12, 14, 32   ; Walk right/left frames
.ENDS

;------------------------------------------------------------------------------
; game_init - Initialize game state and clear OAM
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
    sta.l pad_current
    sta.l pad_current+1

    ; Clear OAM hardware - hide all 128 sprites
    stz $2102
    stz $2103

    rep #$10
    ldx #0
@clear_loop:
    sep #$20
    stz $2104           ; X = 0
    lda #240
    sta $2104           ; Y = 240 (hidden)
    stz $2104           ; Tile = 0
    stz $2104           ; Attr = 0
    rep #$20
    inx
    cpx #128
    bne @clear_loop

    ; Clear high table (32 bytes)
    sep #$20
    ldx #0
@clear_high:
    stz $2104
    inx
    cpx #32
    bne @clear_high

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; read_pad - Read joypad 1
;------------------------------------------------------------------------------
.SECTION ".read_pad" SUPERFREE
read_pad:
    php
    sep #$20

    ; Wait for auto-read to complete
-   lda $4212
    and #$01
    bne -

    ; Read and store joypad
    rep #$20
    lda $4218
    sta.l pad_current

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; update_monster - Handle input and update monster state
; Simplified logic - no complex branching
;------------------------------------------------------------------------------
.SECTION ".update_monster" SUPERFREE
update_monster:
    php
    phx
    rep #$30

    ; Load pad once at the start
    lda.l pad_current
    bne +
    jmp @calc_tile          ; No input - skip everything
+

    ; Store pad in X for repeated checks
    tax

    ; === Check UP (bit 11 = 0x0800) ===
    txa
    and #$0800
    beq @skip_up
    ; UP is pressed - set state and move
    sep #$20
    lda #1
    sta.l monster_state
    lda #0
    sta.l monster_flipx
    rep #$20
    lda.l monster_y
    dec a
    bmi @skip_up
    sta.l monster_y
@skip_up:

    ; === Check LEFT (bit 9 = 0x0200) ===
    rep #$20
    txa
    and #$0200
    beq @skip_left
    ; LEFT is pressed
    sep #$20
    lda #2
    sta.l monster_state
    lda #1
    sta.l monster_flipx
    rep #$20
    lda.l monster_x
    dec a
    bmi @skip_left
    sta.l monster_x
@skip_left:

    ; === Check RIGHT (bit 8 = 0x0100) ===
    rep #$20
    txa
    and #$0100
    beq @skip_right
    ; RIGHT is pressed
    sep #$20
    lda #2
    sta.l monster_state
    lda #0
    sta.l monster_flipx
    rep #$20
    lda.l monster_x
    inc a
    cmp #256
    bcs @skip_right
    sta.l monster_x
@skip_right:

    ; === Check DOWN (bit 10 = 0x0400) ===
    rep #$20
    txa
    and #$0400
    beq @skip_down
    ; DOWN is pressed
    sep #$20
    lda #0
    sta.l monster_state
    sta.l monster_flipx
    rep #$20
    lda.l monster_y
    inc a
    cmp #224
    bcs @skip_down
    sta.l monster_y
@skip_down:

    ; Advance animation
    sep #$20
    lda.l monster_anim
    inc a
    cmp #3
    bcc +
    lda #0
+   sta.l monster_anim

@calc_tile:
    ; Calculate tile = sprTiles[state*3 + anim]
    sep #$20
    rep #$10
    lda.l monster_state
    asl a
    clc
    adc.l monster_state     ; state * 3
    clc
    adc.l monster_anim
    and #$0F                ; Ensure valid index 0-8
    tax
    lda.l sprite_tiles,x
    sta.l monster_tile

    plx
    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; update_oam - Write monster sprite to OAM
;------------------------------------------------------------------------------
.SECTION ".update_oam" SUPERFREE
update_oam:
    php
    sep #$20

    ; Set OAM address to sprite 0
    stz $2102
    stz $2103

    ; Sprite 0: monster at (monster_x, monster_y)
    lda.l monster_x
    sta $2104           ; X position
    lda.l monster_y
    sta $2104           ; Y position
    lda.l monster_tile
    sta $2104           ; tile number
    ; Attributes: vhoopppc (v=vflip, h=hflip, oo=priority, ppp=palette, c=tile bit9)
    ; Priority = 3 (in front of BG), palette = 0
    lda.l monster_flipx
    beq +
    lda #$70            ; hflip + priority 3
    bra ++
+   lda #$30            ; no flip + priority 3
++  sta $2104           ; attr

    ; Hide remaining sprites 1-127 by setting Y=240
    ; (actually just hide sprite 1 for now to save cycles)
    lda #0
    sta $2104           ; X = 0
    lda #240
    sta $2104           ; Y = 240 (hidden)
    stz $2104           ; tile = 0
    stz $2104           ; attr = 0

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; wait_vblank - Wait for VBlank
;------------------------------------------------------------------------------
.SECTION ".wait_vbl" SUPERFREE
wait_vblank:
    php
    sep #$20
-   lda $4212
    bmi -
-   lda $4212
    bpl -
    plp
    rtl
.ENDS
