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

    ; Clear oamMemory buffer - hide all 128 sprites (Y=240)
    rep #$10
    ldx #0
@clear_loop:
    sep #$20
    lda #0
    sta.l oamMemory,x     ; X = 0
    inx
    lda #240
    sta.l oamMemory,x     ; Y = 240 (hidden)
    inx
    lda #0
    sta.l oamMemory,x     ; Tile = 0
    inx
    sta.l oamMemory,x     ; Attr = 0
    inx
    rep #$20
    cpx #512              ; 128 sprites * 4 bytes
    bne @clear_loop

    ; Clear high table (32 bytes) - small size, X < 256 for all
    sep #$20
    lda #0
    ldx #0
@clear_high:
    sta.l oamMemory+512,x
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
    ; Use long addressing for CPU registers (DBR may be $7E)
-   lda.l $004212
    and #$01
    bne -

    ; Read and store joypad
    rep #$20
    lda.l $004218
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
; update_oam - Write monster sprite to OAM buffer and DMA transfer
;------------------------------------------------------------------------------
.SECTION ".update_oam" SUPERFREE
update_oam:
    php
    sep #$20
    rep #$10            ; 16-bit X/Y for indexing

    ; Write sprite 0 to oamMemory buffer
    ; OAM format: X, Y, Tile, Attr (4 bytes per sprite)
    lda.l monster_x
    sta.l oamMemory+0   ; X position (low 8 bits)
    lda.l monster_y
    sta.l oamMemory+1   ; Y position
    lda.l monster_tile
    sta.l oamMemory+2   ; Tile number
    ; Attributes: vhoopppc
    lda.l monster_flipx
    beq +
    lda #$70            ; hflip + priority 3
    bra ++
+   lda #$30            ; no flip + priority 3
++  sta.l oamMemory+3   ; attr

    ; Hide sprite 1 by setting Y=240
    lda #0
    sta.l oamMemory+4   ; X = 0
    lda #240
    sta.l oamMemory+5   ; Y = 240 (off-screen)
    lda #0
    sta.l oamMemory+6   ; tile = 0
    sta.l oamMemory+7   ; attr = 0

    ; High table byte 0: bits for sprites 0-3
    ; Each sprite uses 2 bits: bit0 = X high bit, bit1 = size
    ; Sprite 0: small size, X < 256 so high bit = 0 -> bits = 00
    ; Sprite 1: small size, X < 256 -> bits = 00
    ; Result: 0b00000000
    lda #$00
    sta.l oamMemory+512

    ; Set flag for NMI handler to DMA transfer buffer to OAM
    lda #$01
    sta.l oam_update_flag

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
    ; Use long addressing for CPU registers (DBR may be $7E)
-   lda.l $004212
    bmi -
-   lda.l $004212
    bpl -
    plp
    rtl
.ENDS
