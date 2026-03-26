;==============================================================================
; GSU Code Loader — WRAM-safe launch + SRAM readback
;==============================================================================
; ALL GSU launches MUST use the WRAM stub. The SNES CPU cannot read ROM
; while the GSU is running (RON=1). Even a 3-instruction GSU program can
; race the CPU's instruction prefetch.
;==============================================================================

.ifdef SUPERFX

;------------------------------------------------------------------------------
; GSU program binary
;------------------------------------------------------------------------------
.SECTION ".gsu_code" SUPERFREE
gsu_program:
    .incbin "gsu_3d.sfx.bin"
gsu_program_end:
.ENDS

;------------------------------------------------------------------------------
; WRAM area + result variables
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_vars" BANK 0 SLOT 1
gsu_wram_area: dsb 96       ; must be >= stub size
gsu_scbr: dsb 1
gsu_dma_src_hi: dsb 1
edge_buffer: dsb 48         ; 12 edges × 4 bytes (x0,y0,x1,y1)
.ENDS

;------------------------------------------------------------------------------
; launchGSU + WRAM stub (same section for label arithmetic)
;------------------------------------------------------------------------------
.SECTION ".gsu_launcher" SEMIFREE

.ACCU 16
.INDEX 16

launchGSU:
    php

    ; Copy WRAM stub from ROM to WRAM
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l _wram_stub,x
    sta.l gsu_wram_area,x
    inx
    cpx #(_wram_stub_end - _wram_stub)
    bne -

    ; Execute from WRAM
    jsl gsu_wram_area

    ; GSU finished
    plp
    rtl

;--- WRAM stub (copied and executed from WRAM) ---
_wram_stub:
    sep #$20
    .ACCU 8

    ; Disable NMI (vector is in ROM)
    lda #$00
    sta.l $4200

    ; Configure GSU (ref: casfx, DOOM-FX, PeterLemon)
    lda #$80
    sta.l $3037              ; CFGR: IRQ mask (PeterLemon uses $80)

    ; 21.47 MHz clock (GSU-1/GSU-2 only, no effect on MC1)
    lda #$01
    sta.l $3039              ; CLSR = 21.47 MHz

    ; Set screen base for PLOT (from WRAM variable: $00=bufA, $10=bufB)
    lda.l gsu_scbr
    sta.l $3038              ; SCBR = parameterized

    ; Write buffer base to GSU R8 (for STW clear loop)
    ; R8 = gsu_scbr * 1024 (SCBR=$00→$0000, SCBR=$10→$4000)
    rep #$20
    .ACCU 16
    lda.l gsu_scbr
    and #$00FF               ; zero-extend SCBR
    xba                      ; * 256
    asl a                    ; * 512
    asl a                    ; * 1024
    sta.l $3010              ; R8 = buffer base
    sep #$20
    .ACCU 8

    ; Give buses to GSU + set PLOT mode (4bpp + RAN + RON)
    lda #$19
    sta.l $303A              ; SCMR: 4bpp + RAN + RON

    ; Set program bank
    lda #:gsu_program
    sta.l $3034              ; PBR

    ; Start GSU
    rep #$20
    .ACCU 16
    lda #gsu_program
    sta.l $301E              ; R15 → GO!

    ; Poll SFR
    sep #$20
    .ACCU 8
-   lda.l $3030
    and #$20
    bne -

    ; Reclaim buses
    lda #$00
    sta.l $303A

    ; Re-enable NMI
    lda #$81
    sta.l $4200

    rtl
_wram_stub_end:

.ENDS

;------------------------------------------------------------------------------
; setupBitmapTilemap — Column-major tilemap for SuperFX PLOT at VRAM $4000
;------------------------------------------------------------------------------
; PLOT stores tiles COLUMN-MAJOR: SRAM tile = col * 16 + row (height=128)
; PPU tilemap is ROW-MAJOR: entry = row * 32 + col
; So: tilemap[row*32 + col] = col * 16 + row
;------------------------------------------------------------------------------
.SECTION ".gsu_tilemap" SEMIFREE
.ACCU 16
.INDEX 16
setupBitmapTilemap:
    php
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115
    rep #$20
    .ACCU 16
    lda #$4000
    sta.l $2116

    ; Column-major tilemap: 32 cols x 16 rows
    ldy #$0000               ; row (0-15)
_tm_row:
    ldx #$0000               ; col (0-31)
    tya                      ; A = row
_tm_col:
    sta $2118                ; tile = col*16 + row
    clc
    adc #16                  ; next column
    inx
    cpx #32
    bne _tm_col
    iny
    cpy #16
    bne _tm_row

    ; Fill remaining 512 entries with tile 0
    ldx #$0000
    lda #$0000
-   sta $2118
    inx
    cpx #$0200
    bne -

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; dmaBitmapToVRAM — DMA 16KB from SRAM $70:0000 to VRAM $0000
;------------------------------------------------------------------------------
.SECTION ".gsu_dma" SEMIFREE
.ACCU 16
.INDEX 16
dmaBitmapToVRAM:
    php
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $2116
    lda #16384
    sta.l $4305
    ; Source offset: $0000 (bufA) or $4000 (bufB)
    lda.l gsu_dma_src_hi
    and #$00FF               ; zero-extend to 16-bit
    xba                      ; swap: high byte = src_hi, low = $00
    sta.l $4302              ; source = $xx00
    sep #$20
    .ACCU 8
    lda #$70
    sta.l $4304
    lda #$01
    sta.l $4300
    lda #$18
    sta.l $4301
    lda #$01
    sta.l $420B
    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; writeEdgesToSRAM — Copy edge_buffer (48 bytes WRAM) to SRAM $70:4000
;------------------------------------------------------------------------------
.SECTION ".gsu_edges" SEMIFREE
.ACCU 16
.INDEX 16
writeEdgesToSRAM:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l edge_buffer,x
    sta.l $704000,x
    inx
    cpx #48
    bne -
    plp
    rtl
.ENDS

.endif
