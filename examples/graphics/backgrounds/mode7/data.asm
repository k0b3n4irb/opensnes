; Mode 7 - Asset data and VRAM loader
;
; Mode 7 VRAM layout is interleaved:
;   Low bytes  (VMDATAL) = tilemap (128x128 tile indices)
;   High bytes (VMDATAH) = tile pixel data (8bpp, 256 tiles x 64 bytes)
;
; Requires two separate DMAs with different VMAIN settings.

; Mode 7 tile pixel data (8bpp)
.section ".rodata1" superfree
mode7_tiles:
.incbin "res/mode7bg.pc7"
mode7_tiles_end:
.ends

; Mode 7 tilemap (128x128 = 16384 bytes)
.section ".rodata2" superfree
mode7_map:
.incbin "res/mode7bg.mp7"
mode7_map_end:

mode7_pal:
.incbin "res/mode7bg.pal"
mode7_pal_end:
.ends

; Assembly helper: load Mode 7 data to VRAM
; Must be called during forced blank (INIDISP = $80)
.section ".mode7loader" superfree

asm_loadMode7Data:
    php
    rep #$10            ; 16-bit index registers
    sep #$20            ; 8-bit accumulator

    ; Step 1: Load tilemap to VRAM low bytes
    lda #$00
    sta $2115           ; VMAIN = 0 (increment after low byte write)
    stz $2116           ; VMADDL = 0
    stz $2117           ; VMADDH = 0

    ; DMA channel 0: mode 0 (1 byte, A->B), destination VMDATAL ($2118)
    stz $4300           ; DMA mode 0
    lda #$18            ; B-bus = $2118 (VMDATAL)
    sta $4301
    ldx #mode7_map
    stx $4302           ; Source address low word
    lda #:mode7_map
    sta $4304           ; Source bank
    ldx #(mode7_map_end - mode7_map)
    stx $4305           ; Transfer size
    lda #$01
    sta $420B           ; Start DMA channel 0

    ; Step 2: Load tile pixels to VRAM high bytes
    lda #$80
    sta $2115           ; VMAIN = $80 (increment after high byte write)
    stz $2116           ; VMADDL = 0
    stz $2117           ; VMADDH = 0

    stz $4300           ; DMA mode 0
    lda #$19            ; B-bus = $2119 (VMDATAH)
    sta $4301
    ldx #mode7_tiles
    stx $4302           ; Source address low word
    lda #:mode7_tiles
    sta $4304           ; Source bank
    ldx #(mode7_tiles_end - mode7_tiles)
    stx $4305           ; Transfer size
    lda #$01
    sta $420B           ; Start DMA channel 0

    ; Step 3: Load palette to CGRAM
    stz $2121           ; CGADD = 0 (start at color 0)

    stz $4300           ; DMA mode 0
    lda #$22            ; B-bus = $2122 (CGDATA)
    sta $4301
    ldx #mode7_pal
    stx $4302           ; Source address low word
    lda #:mode7_pal
    sta $4304           ; Source bank
    ldx #(mode7_pal_end - mode7_pal)
    stx $4305           ; Transfer size
    lda #$01
    sta $420B           ; Start DMA channel 0

    ; Restore VMAIN for normal word access
    lda #$80
    sta $2115

    plp
    rtl
.ends
