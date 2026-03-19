;==============================================================================
; OpenSNES Performance Profiling
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

.EQU REG_CGADSUB   $2131       ; Color math designation
.EQU REG_COLDATA   $2132       ; Fixed color data
.EQU REG_SLHV      $2137       ; Latch H/V counters
.EQU REG_OPVCT     $213D       ; Vertical position counter

.RAMSECTION ".profile_vars" BANK 0 SLOT 1
    profile_scanline_start  dsb 2
.ENDS

.SECTION ".profile_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; Color table: 3 COLDATA writes per color (red, green, blue channels)
;
; COLDATA format: ccciiiii
;   bit 7 = blue channel select
;   bit 6 = green channel select
;   bit 5 = red channel select
;   bits 4-0 = intensity (0-31)
;
; To set red to 31:   write $3F ($20 | $1F)
; To set green to 31: write $5F ($40 | $1F)
; To set blue to 31:  write $9F ($80 | $1F)
; To clear a channel:  write $20/$40/$80 (intensity 0)
;------------------------------------------------------------------------------
color_table:
;           R     G     B
.db $3F, $40, $80       ; 0 = RED:     R=31, G=0,  B=0
.db $20, $5F, $80       ; 1 = GREEN:   R=0,  G=31, B=0
.db $20, $40, $9F       ; 2 = BLUE:    R=0,  G=0,  B=31
.db $3F, $5F, $80       ; 3 = YELLOW:  R=31, G=31, B=0
.db $20, $5F, $9F       ; 4 = CYAN:    R=0,  G=31, B=31
.db $3F, $40, $9F       ; 5 = MAGENTA: R=31, G=0,  B=31
.db $3F, $5F, $9F       ; 6 = WHITE:   R=31, G=31, B=31

;------------------------------------------------------------------------------
; void profileInit(void)
;
; Enable color math: add fixed color to backdrop.
; CGADSUB = $20: addition mode, backdrop enabled.
;------------------------------------------------------------------------------
profileInit:
    php
    sep #$20
    lda #$20                    ; add to backdrop
    sta.l REG_CGADSUB
    ; Clear fixed color to black
    lda #$20
    sta.l REG_COLDATA           ; red = 0
    lda #$40
    sta.l REG_COLDATA           ; green = 0
    lda #$80
    sta.l REG_COLDATA           ; blue = 0
    plp
    rtl

;------------------------------------------------------------------------------
; void profileColorStart(u16 color)
;
; Set fixed color from the color table.
; Stack: 5,s = color index (0-6)
;------------------------------------------------------------------------------
profileColorStart:
    php
    phb
    rep #$30                    ; 16-bit A, X
    lda 6,s                     ; color index (php+phb = +2, JSL ret = +3, arg at +6)
    and #$0007                  ; clamp to 0-7
    ; Multiply by 3 (table entry size)
    sta.l tcc__r9               ; temp
    asl a                       ; *2
    clc
    adc.l tcc__r9               ; *3
    tax                         ; X = table offset

    sep #$20                    ; 8-bit A for register writes
    lda.l color_table,x         ; red channel COLDATA byte
    sta.l REG_COLDATA
    lda.l color_table+1,x       ; green channel
    sta.l REG_COLDATA
    lda.l color_table+2,x       ; blue channel
    sta.l REG_COLDATA

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void profileColorEnd(void)
;
; Clear fixed color to black (all channels intensity 0).
;------------------------------------------------------------------------------
profileColorEnd:
    php
    sep #$20
    lda #$20                    ; red = 0
    sta.l REG_COLDATA
    lda #$40                    ; green = 0
    sta.l REG_COLDATA
    lda #$80                    ; blue = 0
    sta.l REG_COLDATA
    plp
    rtl

;------------------------------------------------------------------------------
; u16 profileGetScanline(void)
;------------------------------------------------------------------------------
profileGetScanline:
    php
    sep #$20
    lda.l REG_SLHV              ; latch
    lda.l REG_OPVCT             ; low byte
    rep #$20
    and #$00FF
    pha
    sep #$20
    lda.l REG_OPVCT             ; high bit
    rep #$20
    and #$0001
    .repeat 8
    asl a
    .endr
    ora 1,s
    sta 1,s
    pla
    plp
    rtl

;------------------------------------------------------------------------------
; void profileScanlineStart(void)
;------------------------------------------------------------------------------
profileScanlineStart:
    php
    sep #$20
    lda.l REG_SLHV
    lda.l REG_OPVCT
    rep #$20
    and #$00FF
    sta.w profile_scanline_start
    sep #$20
    lda.l REG_OPVCT
    rep #$20
    and #$0001
    .repeat 8
    asl a
    .endr
    ora.w profile_scanline_start
    sta.w profile_scanline_start
    plp
    rtl

;------------------------------------------------------------------------------
; u16 profileScanlineEnd(void)
;------------------------------------------------------------------------------
profileScanlineEnd:
    php
    sep #$20
    lda.l REG_SLHV
    lda.l REG_OPVCT
    rep #$20
    and #$00FF
    pha
    sep #$20
    lda.l REG_OPVCT
    rep #$20
    and #$0001
    .repeat 8
    asl a
    .endr
    ora 1,s
    sta 1,s
    pla
    sec
    sbc.w profile_scanline_start
    bpl @no_wrap
    clc
    adc #262
@no_wrap:
    plp
    rtl

;------------------------------------------------------------------------------
; u16 profileGetFrameCount(void)
;------------------------------------------------------------------------------
profileGetFrameCount:
    rep #$20
    lda.w frame_count
    rtl

;------------------------------------------------------------------------------
; u16 profileGetLagFrames(void)
;------------------------------------------------------------------------------
profileGetLagFrames:
    rep #$20
    lda.w lag_frame_counter
    rtl

.ENDS
