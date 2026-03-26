;==============================================================================
; OpenSNES SuperFX (GSU) Library
;==============================================================================
; Provides WRAM-safe GSU launch, DMA, HDMA blanking, and tilemap setup.
; All functions are callable from C (JSL calling convention).
;
; The WRAM stub is REQUIRED for ALL GSU launches because the SNES CPU
; cannot read ROM while the GSU is running (RON=1 in SCMR).
;==============================================================================

.ifdef SUPERFX
.include "memmap.inc"

;------------------------------------------------------------------------------
; Library RAMSECTION — shared state for all SuperFX functions
;------------------------------------------------------------------------------
.RAMSECTION ".gsu_lib_vars" BANK 0 SLOT 1
gsu_wram_area:   dsb 128    ; execution area for WRAM stub
gsu_prog_bank:   dsb 1      ; GSU program bank byte (set by gsuSetProgram)
gsu_prog_addr:   dsb 2      ; GSU program offset (set by gsuSetProgram)
gsu_cfgr:        dsb 1      ; CFGR value ($80=default, $A0=fast multiply)
gsu_scmr:        dsb 1      ; SCMR value ($18=RAN+RON, $19=4bpp+RAN+RON)
gsu_scbr:        dsb 1      ; SCBR value ($00=bufA, $10=bufB)
gsu_dma_src_hi:  dsb 1      ; DMA source high byte ($00=bufA, $40=bufB)
gsu_hdma_table:  dsb 20     ; HDMA table for INIDISP blanking
.ENDS

;==============================================================================
; gsuLaunch — Universal WRAM-stub GSU launcher
;==============================================================================
; Reads config from WRAM variables: gsu_prog_bank, gsu_prog_addr,
; gsu_cfgr, gsu_scmr, gsu_scbr. Copies stub to WRAM and executes.
; Returns after GSU stops (SFR GO bit cleared).
;==============================================================================
.SECTION ".gsu_launch" SEMIFREE

.ACCU 16
.INDEX 16

gsuLaunch:
    php

    ; Copy WRAM stub from ROM to WRAM
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16
    ldx #$0000
-   lda.l _gsu_wram_stub,x
    sta.l gsu_wram_area,x
    inx
    cpx #(_gsu_wram_stub_end - _gsu_wram_stub)
    bne -

    ; Execute stub from WRAM
    jsl gsu_wram_area

    plp
    rtl

;--- WRAM stub (parameterized, executed from WRAM) -------------------------
_gsu_wram_stub:
    sep #$20
    .ACCU 8

    ; Disable NMI (vector is in ROM — inaccessible when GSU runs)
    lda #$00
    sta.l $4200

    ; Configure GSU from WRAM variables
    lda.l gsu_cfgr
    sta.l $3037              ; CFGR

    lda #$01
    sta.l $3039              ; CLSR = 21.47 MHz

    lda.l gsu_scbr
    sta.l $3038              ; SCBR (screen base for PLOT)

    ; Compute R8 = SCBR * 1024 (buffer base for STW clear)
    rep #$20
    .ACCU 16
    lda.l gsu_scbr
    and #$00FF
    xba                      ; * 256
    asl a                    ; * 512
    asl a                    ; * 1024
    sta.l $3010              ; R8 = buffer base
    sep #$20
    .ACCU 8

    ; Set SCMR (bus ownership + PLOT mode)
    lda.l gsu_scmr
    sta.l $303A

    ; Set program bank from WRAM variable
    lda.l gsu_prog_bank
    sta.l $3034              ; PBR

    ; Start GSU (writing R15 high byte triggers execution!)
    rep #$20
    .ACCU 16
    lda.l gsu_prog_addr
    sta.l $301E              ; R15 → GO!

    ; Poll SFR until GO flag clears
    sep #$20
    .ACCU 8
-   lda.l $3030
    and #$20
    bne -

    ; Reclaim all buses
    lda #$00
    sta.l $303A

    ; Re-enable NMI
    lda #$81
    sta.l $4200

    rtl
_gsu_wram_stub_end:

.ENDS

;==============================================================================
; gsuSetupBitmapTilemap — Column-major tilemap for SuperFX PLOT
;==============================================================================
; Input: tcc__r0 = VRAM word address for tilemap (typically $4000)
; PLOT stores tiles column-major: tile = col * 16 + row (height=128)
; PPU tilemap is row-major: entry = row * 32 + col
;==============================================================================
.SECTION ".gsu_tilemap" SEMIFREE

.ACCU 16
.INDEX 16

gsuSetupBitmapTilemap:
    php
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115              ; VMAIN: word increment
    rep #$20
    .ACCU 16

    ; VRAM address from stack (first arg)
    lda 5,s
    sta.l $2116

    ; Column-major tilemap: 32 cols × 16 rows
    ldy #$0000               ; row (0-15)
_tm_row:
    ldx #$0000               ; col (0-31)
    tya                      ; A = row
_tm_col:
    sta $2118                ; tile = col*16 + row
    clc
    adc #16                  ; next column: +16
    inx
    cpx #32
    bne _tm_col
    iny
    cpy #16
    bne _tm_row

    ; Fill remaining 512 entries with tile 0 (bottom half of screen)
    ldx #$0000
    lda #$0000
-   sta $2118
    inx
    cpx #$0200
    bne -

    plp
    rtl
.ENDS

;==============================================================================
; gsuDmaFullFrame — Scanline-polled 16KB DMA from SRAM to VRAM
;==============================================================================
; Polls V-counter until scanline >= 184, then DMAs 16KB.
; Requires HDMA blanking (gsuSetupHdmaBlanking) for bandwidth.
; Reads gsu_dma_src_hi for double-buffering support.
;==============================================================================
.SECTION ".gsu_dma_full" SEMIFREE

.ACCU 16
.INDEX 16

gsuDmaFullFrame:
    php
    sep #$20
    .ACCU 8

    ; Pre-configure DMA channel 0
    lda #$80
    sta.l $2115              ; VMAIN: word increment
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $2116              ; VRAM dest = $0000
    lda #16384
    sta.l $4305              ; 16KB

    ; Source offset from gsu_dma_src_hi ($00→$0000, $40→$4000)
    lda.l gsu_dma_src_hi
    and #$00FF
    xba                      ; high byte = src_hi, low = $00
    sta.l $4302

    sep #$20
    .ACCU 8
    lda #$70
    sta.l $4304              ; source bank = SRAM
    lda #$01
    sta.l $4300              ; mode: 2-register write
    lda #$18
    sta.l $4301              ; dest: VMDATAL

    ; Poll V-counter until scanline >= 184 (bottom blank zone)
-   lda.l $2137              ; latch H/V counters
    lda.l $213D              ; V-counter low byte
    cmp #184
    bcc -

    ; Start DMA
    lda #$01
    sta.l $420B

    plp
    rtl
.ENDS

;==============================================================================
; gsuSetupHdmaBlanking — HDMA on INIDISP for DMA bandwidth
;==============================================================================
; Input: arg1 (5,s) = top blank scanlines, arg2 (7,s) = bottom blank scanlines
; Visible = 224 - top - bottom.
;==============================================================================
.SECTION ".gsu_hdma_setup" SEMIFREE

.ACCU 16
.INDEX 16

gsuSetupHdmaBlanking:
    php
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16

    ; Read parameters
    lda 5,s                  ; top blank scanlines
    sta.l gsu_hdma_table     ; entry 0: count = top
    lda #$80
    sta.l gsu_hdma_table+1   ; entry 0: value = forced blank

    ; Compute visible = 224 - top - bottom
    ; HDMA count max = 127 (bit 7 = repeat flag). Split if > 127.
    lda #224
    sec
    sbc 5,s                  ; 224 - top
    sbc 7,s                  ; 224 - top - bottom = visible

    ; Split visible into two entries if > 127
    cmp #128
    bcc _vis_single

    ; Two entries: 127 + remainder
    pha                      ; save visible
    lda #127
    sta.l gsu_hdma_table+2
    lda #$0F
    sta.l gsu_hdma_table+3
    pla
    sec
    sbc #127                 ; remainder
    sta.l gsu_hdma_table+4
    lda #$0F
    sta.l gsu_hdma_table+5

    ; Bottom blank
    lda 7,s
    sta.l gsu_hdma_table+6
    lda #$80
    sta.l gsu_hdma_table+7
    lda #$00
    sta.l gsu_hdma_table+8   ; terminator
    bra _hdma_config

_vis_single:
    ; Single entry (visible <= 127)
    sta.l gsu_hdma_table+2
    lda #$0F
    sta.l gsu_hdma_table+3

    ; Bottom blank
    lda 7,s
    sta.l gsu_hdma_table+4
    lda #$80
    sta.l gsu_hdma_table+5
    lda #$00
    sta.l gsu_hdma_table+6   ; terminator

_hdma_config:

    ; Configure HDMA channel 1 → REG_INIDISP ($2100)
    lda #$00
    sta.l $4310              ; transfer mode: 1 byte, direct
    lda #$00
    sta.l $4311              ; dest: $2100 (INIDISP)
    rep #$20
    .ACCU 16
    lda #gsu_hdma_table
    sta.l $4312              ; source: HDMA table in WRAM
    sep #$20
    .ACCU 8
    lda #$00
    sta.l $4314              ; source bank = 0 (WRAM)

    ; Enable HDMA channel 1
    lda #$02
    sta.l $420C

    plp
    rtl
.ENDS

.endif
