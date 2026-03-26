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
hdma_table: dsb 20          ; HDMA table: 8 entries × 2 + terminator = 17 bytes
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
;------------------------------------------------------------------------------
; dmaChunkToVRAM — DMA 4KB chunk from SRAM to VRAM (fits in VBlank)
;------------------------------------------------------------------------------
; Input: tcc__r0 = chunk index (0-3)
; Chunk 0: SRAM $70:0000 → VRAM $0000, 4096 bytes
; Chunk 1: SRAM $70:1000 → VRAM $0800, 4096 bytes
; Chunk 2: SRAM $70:2000 → VRAM $1000, 4096 bytes
; Chunk 3: SRAM $70:3000 → VRAM $1800, 4096 bytes
;------------------------------------------------------------------------------
.SECTION ".gsu_dma" SEMIFREE
.ACCU 16
.INDEX 16
dmaChunkToVRAM:
    php
    rep #$20
    .ACCU 16

    ; Compute SRAM source = chunk * $1000
    lda 5,s                  ; chunk index from stack (cc65816 left-to-right)
    asl a
    asl a
    asl a
    asl a                    ; chunk * 16
    xba                      ; chunk * 4096 = $x000
    sta.l $4302              ; source offset

    ; Compute VRAM dest = chunk * $0800 (word address)
    lda 5,s
    xba                      ; chunk * 256
    lsr a
    lsr a                    ; chunk * 256 / 4... hmm

    ; Simpler: chunk * $0800
    lda 5,s
    asl a
    asl a
    asl a                    ; chunk * 8
    xba                      ; chunk * 2048 = $x800... no, xba swaps bytes

    ; Even simpler approach: multiply by shifting
    ; chunk=0: VRAM=$0000, chunk=1: $0800, chunk=2: $1000, chunk=3: $1800
    ; $0800 = 2048. chunk * 2048.
    ; In 16-bit: chunk << 11
    lda 5,s
    and #$0003               ; mask to 0-3
    ; shift left 11: asl ×11 is too many. Use: (chunk << 8) << 3
    xba                      ; << 8 (chunk in high byte)
    asl a                    ; << 9
    asl a                    ; << 10
    asl a                    ; << 11
    sta.l $2116              ; VRAM word dest

    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115              ; word increment
    lda #$70
    sta.l $4304              ; source bank
    rep #$20
    .ACCU 16
    lda #4096
    sta.l $4305              ; 4KB
    sep #$20
    .ACCU 8
    lda #$01
    sta.l $4300              ; mode: 2-reg write
    lda #$18
    sta.l $4301              ; dest: VMDATAL
    lda #$01
    sta.l $420B              ; start DMA

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

;------------------------------------------------------------------------------
; setupHDMABlanking — HDMA on channel 1 controls INIDISP per scanline
;------------------------------------------------------------------------------
; PeterLemon pattern: blank top 19 scanlines + last scanline for DMA bandwidth.
; The visible area is scanlines 19-204 (186 lines of display).
; HDMA table written to WRAM (hdma_table), channel 1 → REG_INIDISP ($2100).
;------------------------------------------------------------------------------
.SECTION ".gsu_hdma" SEMIFREE
.ACCU 16
.INDEX 16
setupHDMABlanking:
    php
    sep #$20
    .ACCU 8

    ; Build HDMA table in WRAM
    ; Format: [count, value] pairs. count bit 7=0 → direct mode.
    ; $80 = forced blank (bit 7 of INIDISP), $0F = full brightness
    ldx #$0000

    ; Budget: 40 (top) + 40 (bottom) + 38 (VBlank) = 118 lines = 7.5ms
    ; DMA 16KB needs ~6.1ms + ~97 bytes HDMA overhead → ~6.2ms. Margin: ~1.3ms

    lda #40                  ; 40 scanlines: forced blank (top)
    sta.l hdma_table,x
    inx
    lda #$80
    sta.l hdma_table,x
    inx

    lda #32                  ; 144 visible scanlines (32×4 + 16)
    sta.l hdma_table,x
    inx
    lda #$0F
    sta.l hdma_table,x
    inx

    lda #32
    sta.l hdma_table,x
    inx
    lda #$0F
    sta.l hdma_table,x
    inx

    lda #32
    sta.l hdma_table,x
    inx
    lda #$0F
    sta.l hdma_table,x
    inx

    lda #32
    sta.l hdma_table,x
    inx
    lda #$0F
    sta.l hdma_table,x
    inx

    lda #16                  ; remaining 16 visible scanlines
    sta.l hdma_table,x
    inx
    lda #$0F
    sta.l hdma_table,x
    inx

    lda #40                  ; 40 scanlines: forced blank (bottom)
    sta.l hdma_table,x
    inx
    lda #$80
    sta.l hdma_table,x
    inx

    lda #$00                 ; end HDMA table
    sta.l hdma_table,x

    ; Configure HDMA channel 1 → REG_INIDISP ($2100)
    lda #$00
    sta.l $4310              ; transfer mode: 1 byte, direct
    lda #$00
    sta.l $4311              ; dest: $2100 (INIDISP)
    rep #$20
    .ACCU 16
    lda #hdma_table
    sta.l $4312              ; source: HDMA table address
    sep #$20
    .ACCU 8
    lda #$00                 ; source bank (bank 0 = WRAM)
    sta.l $4314

    ; Enable HDMA channel 1
    lda #$02                 ; bit 1 = channel 1
    sta.l $420C

    plp
    rtl
.ENDS

;------------------------------------------------------------------------------
; dmaFullFrameToVRAM — Wait for scanline 194, then DMA 16KB
;------------------------------------------------------------------------------
; PeterLemon technique: poll V-counter until scanline >= 194 (start of
; bottom forced blank). DMA runs through: bottom blank (30 lines) +
; VBlank (38 lines) + top blank (30 lines) = 98 lines = ~6.2ms.
; 16KB DMA needs ~6.1ms → fits perfectly.
;------------------------------------------------------------------------------
.SECTION ".gsu_dma_full" SEMIFREE
.ACCU 16
.INDEX 16
dmaFullFrameToVRAM:
    php
    sep #$20
    .ACCU 8

    ; Pre-configure DMA channel 0 (everything except start)
    lda #$80
    sta.l $2115              ; VMAIN: word increment
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $2116              ; VRAM dest = $0000
    lda #16384              ; full 16KB framebuffer
    sta.l $4305              ; 16KB
    lda #$0000
    sta.l $4302              ; source offset
    sep #$20
    .ACCU 8
    lda #$70
    sta.l $4304              ; source bank = SRAM
    lda #$01
    sta.l $4300              ; mode: 2-reg write
    lda #$18
    sta.l $4301              ; dest: VMDATAL

    ; Poll V-counter until scanline >= 184 (start of bottom blank zone)
    ; 40 top + 144 visible = 184. DMA runs through: 40 blank + 38 VBlank + 40 blank = 7.5ms
-   lda.l $2137              ; latch H/V counters
    lda.l $213D              ; V-counter low byte
    cmp #184
    bcc -                    ; loop while scanline < 184

    ; Start DMA — we're in forced blank zone
    lda #$01
    sta.l $420B              ; GO!

    plp
    rtl
.ENDS

.endif
