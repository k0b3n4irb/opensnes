;==============================================================================
; Mode 7 Graphics Data and Loading Routines
;==============================================================================

.SECTION ".mode7data" SUPERFREE

mode7_tiles:
.INCBIN "assets/mode7bg.pc7"
mode7_tiles_end:

mode7_map:
.INCBIN "assets/mode7bg.mp7"
mode7_map_end:

mode7_pal:
.INCBIN "assets/mode7bg.pal"

.ENDS

;==============================================================================
; Mode 7 VRAM Loading Function
;==============================================================================
; Mode 7 VRAM layout:
;   - Tilemap in low bytes of VRAM words $0000-$3FFF
;   - Character data in high bytes of VRAM words $0000-$3FFF
;==============================================================================

.SECTION ".mode7code" SUPERFREE

;------------------------------------------------------------------------------
; load_mode7_vram - Load Mode 7 tiles and map to VRAM
;------------------------------------------------------------------------------
; Called from C as: load_mode7_vram()
;------------------------------------------------------------------------------
load_mode7_vram:
    php
    phb

    sep #$20            ; 8-bit A
    rep #$10            ; 16-bit X/Y

    ; Set data bank to 0 for register access
    lda #$00
    pha
    plb

    ; Set VRAM address to 0
    stz $2116           ; VMADDL
    stz $2117           ; VMADDH

    ; VMAIN: increment after low byte write ($2118), step 1
    stz $2115

    ;--------------------------------------------------
    ; DMA tilemap to VRAM low bytes
    ;--------------------------------------------------
    lda #$00            ; DMA mode: 1 byte, A->B, no increment mode
    sta $4300           ; DMAP0

    lda #$18            ; B-bus address: $2118 (VMDATAL)
    sta $4301           ; BBAD0

    ; A-bus address (source) - 24-bit pointer to mode7_map
    lda #<mode7_map
    sta $4302           ; A1T0L
    lda #>mode7_map
    sta $4303           ; A1T0H
    lda #:mode7_map     ; Bank byte
    sta $4304           ; A1B0

    ; Transfer size: 16384 bytes (128x128 tilemap)
    rep #$20
    lda #$4000
    sta $4305           ; DAS0L/H
    sep #$20

    ; Start DMA on channel 0
    lda #$01
    sta $420B           ; MDMAEN

    ;--------------------------------------------------
    ; DMA tile data to VRAM high bytes
    ;--------------------------------------------------
    ; Reset VRAM address to 0
    stz $2116
    stz $2117

    ; VMAIN: increment after high byte write ($2119), step 1
    lda #$80
    sta $2115

    lda #$00            ; DMA mode
    sta $4300

    lda #$19            ; B-bus address: $2119 (VMDATAH)
    sta $4301

    ; A-bus address - mode7_tiles
    lda #<mode7_tiles
    sta $4302
    lda #>mode7_tiles
    sta $4303
    lda #:mode7_tiles   ; Bank byte
    sta $4304

    ; Transfer size: 7168 bytes (112 tiles * 64 bytes)
    ; Actually use the full size for safety
    rep #$20
    lda #(mode7_tiles_end - mode7_tiles)
    sta $4305
    sep #$20

    ; Start DMA
    lda #$01
    sta $420B

    ;--------------------------------------------------
    ; DMA palette to CGRAM
    ;--------------------------------------------------
    stz $2121           ; CGADD = 0

    lda #$00            ; DMA mode
    sta $4300

    lda #$22            ; B-bus address: $2122 (CGDATA)
    sta $4301

    ; A-bus address - mode7_pal
    lda #<mode7_pal
    sta $4302
    lda #>mode7_pal
    sta $4303
    lda #:mode7_pal     ; Bank byte
    sta $4304

    ; Transfer size: 512 bytes (256 colors * 2)
    rep #$20
    lda #$0200
    sta $4305
    sep #$20

    ; Start DMA
    lda #$01
    sta $420B

    plb
    plp
    rtl

.ENDS
