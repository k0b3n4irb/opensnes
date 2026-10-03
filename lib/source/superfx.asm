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
gsu_prog_bank:   dsb 1      ; GSU program bank byte (set by gsuSetProgram)
gsu_prog_addr:   dsb 2      ; GSU program offset (set by gsuSetProgram)
gsu_cfgr:        dsb 1      ; CFGR value ($80=default, $A0=fast multiply)
gsu_scmr:        dsb 1      ; SCMR value ($18=RAN+RON, $19=4bpp+RAN+RON)
gsu_scbr:        dsb 1      ; SCBR value ($00=bufA, $10=bufB)
gsu_dma_src_hi:  dsb 1      ; DMA source high byte ($00=bufA, $40=bufB)
gsu_hdma_table:  dsb 20     ; HDMA table for INIDISP blanking
gsu_tm_rows:     dsb 2      ; gsuSetupBitmapTilemap: tile rows per column
; SCMR is write-only: the value last written, kept by every lib path that
; starts or stops a job (set BEFORE the buses go to the GSU, cleared BEFORE
; they come back). The present step restores it after taking RAN.
gsu_scmr_live:   dsb 1
; gsuPresent (phase D, 2026-09-29) — shared by superfx.c and the NMI step
gsu_pres_busy:   dsb 1      ; 1 while a queued frame is being moved
gsu_pres_flags:  dsb 1      ; gsuPresentInit flags (bit 0: lag frames too)
gsu_pres_nba_back:  dsb 1   ; BG1 char-base nibble of the block being filled
gsu_pres_nba_front: dsb 1   ; ...and of the block on screen
gsu_pres_top:    dsb 1      ; letterbox bands (gsuSetupHdmaBlanking)
gsu_pres_bottom: dsb 1
gsu_pres_scbr_a: dsb 1      ; SCBR of the two Game Pak RAM framebuffers
gsu_pres_scbr_b: dsb 1
gsu_pres_src:    dsb 2      ; Game Pak RAM offset of the queued frame
gsu_pres_off:    dsb 2      ; bytes of it already in VRAM
gsu_pres_size:   dsb 2      ; framebuffer bytes (0 = gsuPresentInit not called)
gsu_pres_vram_back:  dsb 2  ; VRAM word address of the block being filled
gsu_pres_vram_front: dsb 2  ; ...and of the block on screen
gsu_pres_frames: dsb 2      ; frames put on screen
gsu_pres_last:   dsb 2      ; bytes the last transferring NMI moved
gsu_pres_vtotal: dsb 2      ; lines per frame: 262 NTSC, 312 PAL
gsu_pres_tmp:    dsb 2      ; NMI step scratch
gsu_dff_v:       dsb 2      ; gsuDmaFullFrame scratch (main thread only)
gsu_dff_first:   dsb 2      ; its start window, first and last line
gsu_dff_last:    dsb 2
.ENDS

;==============================================================================
; gsuLaunch — Universal WRAM-stub GSU launcher
;==============================================================================
; Reads config from WRAM variables: gsu_prog_bank, gsu_prog_addr,
; gsu_cfgr, gsu_scmr, gsu_scbr. Calls the wait loop in the RAM code window
; (the Super FX build reserves it, make/common.mk RAM_CODE_SDK). Until
; 2026-09-29 each call copied a 128-byte stub to bank-0 RAM first.
; Returns after GSU stops (SFR GO bit cleared).
;==============================================================================
.SECTION ".gsu_launch" SEMIFREE

.ACCU 16
.INDEX 16

gsuLaunch:
    php
    jsl gsu_launch_wait      ; $7E:xxxx, the RAM code window
    plp
    rtl
.ENDS

;--- the wait loop, run from WRAM while the GSU owns the ROM -----------------
.SECTION "ram_code.gsu_launch" BASE $7D APPENDTO ".ram_code"
.ACCU 16
.INDEX 16
gsu_launch_wait:
    sep #$20
    .ACCU 8

    ; The NMI stays enabled (phase B, 2026-09-25): flag that the GSU is about
    ; to own the Game Pak, and the $0108 WRAM NMI does the ROM-free VBlank
    ; work until the flag drops. Before, this stub disabled NMI — one VBlank
    ; in three was lost in superfx_3d — and re-enabled it with a hardcoded
    ; $81, silently cancelling any IRQ the game had armed.
    lda #$01
    sta.l gsu_owns_cart

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

    ; Set SCMR (bus ownership + PLOT mode); the shadow first
    lda.l gsu_scmr
    sta.l gsu_scmr_live
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

    ; Reclaim all buses, then drop the flag: from here the normal NMI runs.
    ; The shadow is cleared first, so an NMI in between never hands a
    ; stopped GSU the ROM back.
    lda #$00
    sta.l gsu_scmr_live
    sta.l $303A
    sta.l gsu_owns_cart

    rtl

.ENDS

;==============================================================================
; gsuSetupBitmapTilemap — Column-major tilemap for SuperFX PLOT
;==============================================================================
; Input: VRAM word address for the 32x32 tilemap (typically $4000)
; PLOT stores tiles column-major: tile = col * rows + row, rows = H / 8 from
; gsu_scmr's height (128 -> 16, 160 -> 20, 192 -> 24; fullsnes SCMR). Until
; 2026-09-29 the map was always 16 rows.
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

    ; rows = 16 + 4 * HT, HT = SCMR bit 2 | bit 5 << 1 (OBJ mode: 16)
    lda.l gsu_scmr
    and #$0024
    cmp #$0004
    beq _tm_160
    cmp #$0020
    beq _tm_192
    lda #16
    bra _tm_rows_set
_tm_160:
    lda #20
    bra _tm_rows_set
_tm_192:
    lda #24
_tm_rows_set:
    sta.l gsu_tm_rows

    ; Column-major tilemap: 32 cols × rows
    ldy #$0000               ; row
_tm_row:
    ldx #$0000               ; col (0-31)
    tya                      ; A = row
_tm_col:
    sta $2118                ; tile = col*rows + row
    clc
    adc.l gsu_tm_rows        ; next column: +rows
    inx
    cpx #32
    bne _tm_col
    iny
    tya
    cmp.l gsu_tm_rows
    bne _tm_row

    ; Fill the rest of the 32x32 map with tile 0: 1024 - 32 * rows entries
    lda.l gsu_tm_rows
    asl a
    asl a
    asl a
    asl a
    asl a                    ; 32 * rows
    eor #$FFFF
    sec
    adc #$0400               ; 1024 - 32 * rows
    tax
    lda #$0000
-   sta $2118
    dex
    bne -

    plp
    rtl
.ENDS

;==============================================================================
; gsuDmaFullFrame — one 16 KB DMA from Game Pak RAM to VRAM, in the bands
;==============================================================================
; Starts the DMA only on a line from which the whole 16 KB lands before
; the display comes back: from the first line of the bottom band
; (225 - bottom: the HDMA write for the band's first entry lands one line
; after its count says, luna --dma-trace showed 116 bytes on line 184) to
; the last line that leaves ~110 lines of blank (the rest of this frame +
; the next frame's top band: 152 + top; 16 KB at 152 bytes per line is
; 108). Bands from gsuSetupHdmaBlanking — 40 + 40 gives lines 185-192. Called later than that, it waits for the next
; frame's window. Until 2026-09-29 it read OPVCT once per poll and never
; STAT78, the only reset of OPVCT's read-twice flip-flop (snesdev-wiki):
; every other call started on the high byte, PPU2 open bus in bits 1-7
; (anomie, fullsnes) = the previous exit value, and left at once. luna
; counted 481 866 of 1 359 872 bytes written on visible lines in
; superfx_3d, silently dropped.
; The VRAM and DMA registers are written after the wait, so an NMI during
; it cannot move VMADD under the transfer.
; Reads gsu_dma_src_hi for double-buffering support.
;==============================================================================
.SECTION ".gsu_dma_full" SEMIFREE

.ACCU 16
.INDEX 16

gsuDmaFullFrame:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    ; first = 225 - bottom, last = max(first, 152 + top)
    lda.l gsu_pres_bottom
    and #$00FF
    eor #$FFFF
    sec
    adc #225
    sta.l gsu_dff_first
    lda.l gsu_pres_top
    and #$00FF
    clc
    adc #152
    cmp.l gsu_dff_first
    bcs +
    lda.l gsu_dff_first
+   sta.l gsu_dff_last

@poll:
    jsr _gsu_dff_read_v
    cmp.l gsu_dff_first
    bcc @poll                ; not yet in the bottom band
    cmp.l gsu_dff_last
    beq @go
    bcc @go                  ; in the window
@late:                       ; too late for this frame: wait for the next
    jsr _gsu_dff_read_v
    cmp.l gsu_dff_first
    bcs @late
    bra @poll

@go:
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $2115              ; VMAIN: word increment
    rep #$20
    .ACCU 16
    lda #$0000
    sta.l $2116              ; VRAM dest = $0000
    lda #16384
    sta.l $4305              ; 16KB
    lda.l gsu_dma_src_hi     ; source offset: $00 -> $0000, $40 -> $4000
    and #$00FF
    xba
    sta.l $4302
    sep #$20
    .ACCU 8
    lda #$70
    sta.l $4304              ; source bank = Game Pak RAM
    lda #$01
    sta.l $4300              ; mode: 2-register write
    lda #$18
    sta.l $4301              ; dest: VMDATAL
    lda #$01
    sta.l $420B              ; start (channel 0)
    plp
    rtl

; V counter (9 bits) -> A (16-bit). STAT78 first: it clears the counter
; latch and the read-twice flip-flops; SLHV latches; OPVCT low, then high.
_gsu_dff_read_v:
    sep #$20
    .ACCU 8
    lda.l $00213F
    lda.l $002137
    lda.l $00213D
    sta.l gsu_dff_v
    lda.l $00213D
    and #$01
    sta.l gsu_dff_v+1
    rep #$20
    .ACCU 16
    lda.l gsu_dff_v
    rts
.ENDS

;==============================================================================
; gsuSetupHdmaBlanking — HDMA on INIDISP for DMA bandwidth
;==============================================================================
; void gsuSetupHdmaBlanking(u16 topBlank, u16 bottomBlank)
; Stack (php only, arguments pushed left to right): 5,s = bottomBlank,
; 7,s = topBlank. The two were read swapped until 2026-09-20 (PVSnesLib-order
; offsets); the only caller passed (40, 40), which is why nothing showed. The
; comments below name the parameters so check_asm_abi.py verifies the offsets.
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

    ; The present step trusts the bands only once they are on screen: none
    ; while they change (VBlank-only window), the new ones at the end
    lda #$00
    sta.l gsu_pres_bottom
    sta.l gsu_pres_top

    ; Read parameters
    lda 7,s                  ; topBlank
    sta.l gsu_hdma_table     ; entry 0: count = top
    lda #$80
    sta.l gsu_hdma_table+1   ; entry 0: value = forced blank

    ; Compute visible = 224 - top - bottom
    ; HDMA count max = 127 (bit 7 = repeat flag). Split if > 127.
    lda #224
    sec
    sbc 7,s                  ; topBlank: 224 - top
    sbc 5,s                  ; bottomBlank: 224 - top - bottom = visible

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
    lda 5,s                  ; bottomBlank
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
    lda 5,s                  ; bottomBlank
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

    ; Enable HDMA channel 1 through the hdma module's shadow of HDMAEN
    ; ($420C is write-only): a bare `sta $420C` switched every other
    ; channel off, and the next hdmaEnableMask/hdmaDisableMask, which rewrites
    ; $420C from the shadow, switched this one off (2026-09-26).
    lda #$02
    ora.l hdma_enabled_state
    sta.l hdma_enabled_state
    sta.l $420C

    ; Return only once the bands are on screen. HDMA starts a channel at
    ; the frame's line 0 (anomie: registers initialised at about V=0), so
    ; enabled mid-frame it does nothing until the next frame: a DMA at
    ; line 184 of the current frame (gsuDmaFullFrame) found no bottom band
    ; and wrote 3 417 VRAM bytes on visible lines (superfx_3d's first loop
    ; pass, luna --dma-trace, 2026-09-29). Wait for the end of the next
    ; VBlank (HVBJOY bit 7), then publish the bands to the present step.
-   lda.l $004212
    bpl -                    ; until VBlank starts
-   lda.l $004212
    bmi -                    ; until it ends: line 0, HDMA initialised
    lda 5,s                  ; bottomBlank
    sta.l gsu_pres_bottom
    lda 7,s                  ; topBlank
    sta.l gsu_pres_top

    plp
    rtl
.ENDS

;==============================================================================
; gsuCacheLoad — copy a GSU program into the code cache ($3100-$32FF)
;==============================================================================
; void gsuCacheLoad(const void *code, u16 size)
; Stack (php only, arguments pushed left to right): 5,s = size, 7,s = code
; (a 4-byte far pointer, its bank byte at 9,s). GO = 0 first — the GSU
; stops, CBR becomes 0, every cache line is empty — then the bytes, then
; NOP ($01) up to the end of the last 16-byte line, so every line used is
; written in full (krom's cache-injection test does the same). At most 512
; bytes. In asm since 2026-09-29: the C version took a third of a frame.
;==============================================================================
.SECTION ".gsu_cache_load" SEMIFREE
.ACCU 16
.INDEX 16
gsuCacheLoad:
    php
    rep #$30
    .ACCU 16
    .INDEX 16
    lda 7,s                  ; code
    sta tcc__r0
    lda 9,s                  ; the pointer's bank byte (and its top byte)
    sta tcc__r0h
    lda 5,s                  ; size
    cmp #513
    bcc +
    lda #512
+   sta tcc__r1              ; bytes to copy
    sep #$20
    .ACCU 8
    lda #$00
    sta.l $003030            ; SFR low: GO = 0, the cache empties
    sta.l $003031
    ldy #$0000
-   cpy tcc__r1
    beq @pad
    lda [tcc__r0],y
    tyx
    sta.l $003100,x
    iny
    bra -
@pad:
    rep #$20
    .ACCU 16
    lda tcc__r1
    clc
    adc #15
    and #$FFF0
    sta tcc__r1              ; the end of the last 16-byte line
    sep #$20
    .ACCU 8
    lda #$01                 ; NOP
-   cpy tcc__r1
    beq @done
    tyx
    sta.l $003100,x
    iny
    bra -
@done:
    plp
    rtl
.ENDS

;==============================================================================
; gsu_present_step — move a presented frame from Game Pak RAM to VRAM
;==============================================================================
; Called by crt0's NMI after the other VBlank work: the ROM NmiHandler when
; the main thread waits in WaitForVBlank, the WRAM NMI (gsu_nmi_blob) when a
; job owns the cart. Lives in the RAM code window for the second case.
;
; If a frame is queued (gsuPresent): reads the V counter, computes how many
; lines of blank are left — the VBlank, extended into the top letterbox of
; gsuSetupHdmaBlanking when its bottom band keeps force blank across line 0
; — and DMAs up to 152 bytes per line of it on channel 7 (8 master cycles a
; byte, 1324 usable per line: 152 leaves room for refresh and HDMA). The GSU
; loses Game Pak RAM for the transfer (RAN = 0: Nintendo manual Book II 5.3,
; the GSU waits on its next RAM access) and gets it back after. When the
; whole frame is in the back VRAM block, BG1's char base swaps to it.
;
; No hardware multiplier (the main thread may be mid-multiply), no $2180.
; Every access is long: DB and D are whatever the NMI path left.
;==============================================================================
.SECTION "ram_code.gsu_present" BASE $7D APPENDTO ".ram_code"
.ACCU 16
.INDEX 16
; gsu_present_step_lag — the ROM NMI's lag-frame path (the main thread is
; not in WaitForVBlank): moves the frame only if the game asked for it with
; GSU_PRESENT_ON_LAG_FRAMES, promising its main thread leaves VRAM, VMAIN,
; VMADD, DMA channel 7 and the H/V counter latch alone while a frame is in
; flight. Falls through into gsu_present_step.
gsu_present_step_lag:
    php
    sep #$20
    .ACCU 8
    lda.l gsu_pres_flags
    and #$01
    bne +
    plp
    rtl
+   plp
gsu_present_step:
    php
    sep #$20
    .ACCU 8
    lda.l gsu_pres_busy
    bne @go
    plp
    rtl
@go:
    rep #$30
    .ACCU 16
    .INDEX 16
    phx
    phy
    ; V counter: STAT78 resets the latch and the read-twice flip-flops,
    ; SLHV latches, OPVCT is read low then high (bit 8)
    sep #$20
    .ACCU 8
    lda.l $00213F
    lda.l $002137
    lda.l $00213D
    sta.l gsu_pres_tmp
    lda.l $00213D
    and #$01
    sta.l gsu_pres_tmp+1
    rep #$20
    .ACCU 16
    lda.l gsu_pres_tmp
    cmp #200
    bcs +                       ; this frame's VBlank
    clc
    adc.l gsu_pres_vtotal       ; already in the next frame's top lines
+   sta.l gsu_pres_tmp          ; V, counted from this frame's line 0

    ; The window's last line: vtotal + top - 2 with a letterbox, else the
    ; last VBlank line, vtotal - 1
    sep #$20
    .ACCU 8
    lda.l gsu_pres_bottom
    beq @no_band
    lda.l gsu_pres_top
    cmp #3
    bcc @no_band
    rep #$20
    .ACCU 16
    and #$00FF
    clc
    adc.l gsu_pres_vtotal
    dec a
    dec a
    bra @have_end
@no_band:
    rep #$20
    .ACCU 16
    lda.l gsu_pres_vtotal
    dec a
@have_end:
    ; lines = end - V - 1 (one line for this setup)
    sec
    sbc.l gsu_pres_tmp
    dec a
    bmi @none
    bne @some
@none:
    brl @out
@some:
    ; bytes = lines * 152 = lines * (128 + 16 + 8)
    asl a
    asl a
    asl a                       ; * 8
    sta.l gsu_pres_tmp
    asl a                       ; * 16
    tax
    asl a
    asl a
    asl a                       ; * 128
    sta.l gsu_pres_last
    txa
    clc
    adc.l gsu_pres_last
    clc
    adc.l gsu_pres_tmp
    sta.l gsu_pres_tmp          ; bytes the window allows
    ; ... capped by what is left of the frame
    lda.l gsu_pres_size
    sec
    sbc.l gsu_pres_off
    cmp.l gsu_pres_tmp
    bcc +
    lda.l gsu_pres_tmp
+   sta.l gsu_pres_last

    ; Game Pak RAM to the CPU for the transfer, if the GSU holds it
    sep #$20
    .ACCU 8
    lda.l gsu_scmr_live
    and #$08
    beq +
    lda.l gsu_scmr_live
    and #$F7
    sta.l $00303A
+
    lda #$80
    sta.l $002115               ; VMAIN: word increment after $2119
    rep #$20
    .ACCU 16
    lda.l gsu_pres_off
    lsr a
    clc
    adc.l gsu_pres_vram_back
    sta.l $002116               ; VMADD
    lda #$1801
    sta.l $004370               ; ch7: mode 1 (two registers), B-bus $18
    lda.l gsu_pres_src
    clc
    adc.l gsu_pres_off
    sta.l $004372               ; A-bus address in Game Pak RAM
    lda.l gsu_pres_last
    sta.l $004375               ; byte count
    sep #$20
    .ACCU 8
    lda #$70
    sta.l $004374               ; bank $70
    lda #$80
    sta.l $00420B               ; MDMAEN ch7

    lda.l gsu_scmr_live         ; the RAM back to the GSU
    and #$08
    beq +
    lda.l gsu_scmr_live
    sta.l $00303A
+
    rep #$20
    .ACCU 16
    lda.l gsu_pres_off
    clc
    adc.l gsu_pres_last
    sta.l gsu_pres_off
    cmp.l gsu_pres_size
    bcs @landed
    brl @out
@landed:

    ; The whole frame is in the back block: show it, and the old front
    ; block becomes the next back one
    sep #$20
    .ACCU 8
    lda.l bg12nba_shadow
    and #$F0
    ora.l gsu_pres_nba_back
    sta.l bg12nba_shadow
    sta.l $00210B               ; BG12NBA: BG1 char base
    lda.l gsu_pres_nba_back
    pha
    lda.l gsu_pres_nba_front
    sta.l gsu_pres_nba_back
    pla
    sta.l gsu_pres_nba_front
    rep #$20
    .ACCU 16
    lda.l gsu_pres_vram_back
    pha
    lda.l gsu_pres_vram_front
    sta.l gsu_pres_vram_back
    pla
    sta.l gsu_pres_vram_front
    lda.l gsu_pres_frames
    inc a
    sta.l gsu_pres_frames
    sep #$20
    .ACCU 8
    lda #$00
    sta.l gsu_pres_busy
@out:
    rep #$30
    .ACCU 16
    .INDEX 16
    ply
    plx
    plp
    rtl
.ENDS

.endif
