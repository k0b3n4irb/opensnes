;==============================================================================
; OAM Helper Functions (Standalone version)
;==============================================================================
; Use this file when NOT linking with the OpenSNES library.
; These functions write to the OAM shadow buffer in RAM.
; The NMI handler transfers the buffer to OAM via DMA.
;==============================================================================

.SECTION ".oam_helpers" SEMIFREE

;------------------------------------------------------------------------------
; oam_set_tile - Set sprite 0's tile number in buffer
; void oam_set_tile(u8 tile)
; Stack: [P:1] [retaddr:3] [tile:2] -> tile at offset 5
;------------------------------------------------------------------------------
oam_set_tile:
    php
    sep #$20            ; 8-bit A
    lda 5,s             ; Get tile param
    sta oamMemory+2     ; Write to buffer byte 2 (tile)
    lda #$01
    sta oam_update_flag ; Mark buffer as needing update
    plp
    rtl

;------------------------------------------------------------------------------
; oam_init_sprite0 - Initialize sprite 0 with full attributes
; void oam_init_sprite0(u8 x, u8 y, u8 tile, u8 attr)
; Stack: [P:1] [retaddr:3] [attr:2] [tile:2] [y:2] [x:2]
;        attr at 5, tile at 7, y at 9, x at 11
; Note: With OBJSEL=0x60, small=16x16, large=32x32. We use small size.
;------------------------------------------------------------------------------
oam_init_sprite0:
    php
    sep #$20            ; 8-bit A
    lda 11,s            ; Get X
    sta oamMemory+0
    lda 9,s             ; Get Y
    sta oamMemory+1
    lda 7,s             ; Get tile
    sta oamMemory+2
    lda 5,s             ; Get attr (priority, palette, etc)
    sta oamMemory+3
    ; High table byte 0: sprite 0 uses bits 0-1
    ; Bit 0 = X high bit (keep 0 for X < 256)
    ; Bit 1 = size (0=small=16x16, 1=large=32x32)
    stz oamMemory+512   ; Small size, X bit 8 = 0
    lda #$01
    sta oam_update_flag
    plp
    rtl

;------------------------------------------------------------------------------
; oam_hide_all - Hide all 128 sprites (set Y=240 for each)
; Call this during initialization
;------------------------------------------------------------------------------
oam_hide_all:
    php
    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A
    ldx #0
-   stz oamMemory,x     ; X = 0
    lda #240
    sta oamMemory+1,x   ; Y = 240 (off screen)
    stz oamMemory+2,x   ; Tile = 0
    stz oamMemory+3,x   ; Attr = 0
    inx
    inx
    inx
    inx
    cpx #512            ; 128 sprites * 4 bytes
    bne -
    ; Clear high table (32 bytes)
    ldx #512
-   stz oamMemory,x
    inx
    cpx #544
    bne -
    lda #$01
    sta oam_update_flag
    plp
    rtl

;------------------------------------------------------------------------------
; oam_set_pos - Set sprite 0's X and Y position in buffer
; void oam_set_pos(u8 x, u8 y)
; Stack: [P:1] [retaddr:3] [y:2] [x:2] -> x at 7, y at 5
;------------------------------------------------------------------------------
oam_set_pos:
    php
    sep #$20            ; 8-bit A
    lda 7,s             ; Get X param
    sta oamMemory+0     ; Write to buffer byte 0 (X)
    lda 5,s             ; Get Y param
    sta oamMemory+1     ; Write to buffer byte 1 (Y)
    lda #$01
    sta oam_update_flag ; Mark buffer as needing update
    plp
    rtl

;------------------------------------------------------------------------------
; oam_set_attr - Set sprite 0's attribute byte
; void oam_set_attr(u8 attr)
; Attribute byte format: vhoopppc
;   v=vflip, h=hflip, oo=priority, ppp=palette, c=tile high bit
; Stack: [P:1] [retaddr:3] [attr:2] -> attr at 5
;------------------------------------------------------------------------------
oam_set_attr:
    php
    sep #$20            ; 8-bit A
    lda 5,s             ; Get attr param
    sta oamMemory+3     ; Write to buffer byte 3 (attr)
    lda #$01
    sta oam_update_flag
    plp
    rtl

;------------------------------------------------------------------------------
; oamUpdate - DMA transfer OAM buffer to hardware
; Called during VBlank (can be called from C code where DBR=$7E)
; Sets DBR to $00 temporarily to ensure hardware register access
;------------------------------------------------------------------------------
oamUpdate:
    php
    phb                 ; Save current data bank

    ; Set DBR to $00 for hardware register access
    pea $0000
    plb
    plb                 ; DBR = $00

    rep #$20            ; 16-bit A

    lda #$0000
    sta $2102           ; OAM address = 0

    ; Use DMA channel 7 (like pvsneslib)
    lda #$0400          ; DMA mode: CPU->PPU, auto-increment, target $2104
    sta $4370

    lda #544            ; Transfer size = 544 bytes (full OAM)
    sta $4375

    lda #oamMemory
    sta $4372           ; DMA source address (low word)

    sep #$20            ; 8-bit A
    lda #:oamMemory     ; Get bank of oamMemory
    sta $4374           ; DMA source bank

    lda #$80            ; Enable DMA channel 7
    sta $420B

    plb                 ; Restore data bank
    plp
    rtl                 ; Return long (called with JSL)

.ENDS
