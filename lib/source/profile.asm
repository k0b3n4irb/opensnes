;==============================================================================
; OpenSNES Performance Profiling
;==============================================================================
;
; Provides:
;   - Visual color-bar profiling (COLDATA $2132)
;   - Scanline-based timing (OPVCT $213D)
;   - Frame/lag counters (from crt0.asm system vars)
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

;------------------------------------------------------------------------------
; Constants
;------------------------------------------------------------------------------

.EQU REG_COLDATA   $2132       ; Fixed color data (backdrop for color math)
.EQU REG_SLHV      $2137       ; Latch H/V position counters
.EQU REG_OPVCT     $213D       ; Vertical position counter (read twice)

;------------------------------------------------------------------------------
; Local storage for scanline timing
;------------------------------------------------------------------------------

.RAMSECTION ".profile_vars" BANK 0 SLOT 1
    profile_scanline_start  dsb 2   ; Saved scanline for timing
.ENDS

;==============================================================================
; CODE
;==============================================================================

.SECTION ".profile_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void profileColorStart(u16 color)
;
; Set fixed color (COLDATA) for visual profiling.
; The color constant encodes R+G+B channel select + intensity.
;
; Stack: 5,s = color (16-bit, only low byte used)
;------------------------------------------------------------------------------
profileColorStart:
    php
    sep #$20                    ; 8-bit A
    lda 5,s                     ; color (low byte)
    ora #$20                    ; ensure red channel select
    sta.l REG_COLDATA           ; set red component
    lda 5,s
    ora #$40                    ; green channel select
    sta.l REG_COLDATA           ; set green component
    lda 5,s
    ora #$80                    ; blue channel select
    sta.l REG_COLDATA           ; set blue component
    plp
    rtl

;------------------------------------------------------------------------------
; void profileColorEnd(void)
;
; Clear fixed color back to black (all channels = 0).
;------------------------------------------------------------------------------
profileColorEnd:
    php
    sep #$20                    ; 8-bit A
    lda #$20                    ; red channel, intensity 0
    sta.l REG_COLDATA
    lda #$40                    ; green channel, intensity 0
    sta.l REG_COLDATA
    lda #$80                    ; blue channel, intensity 0
    sta.l REG_COLDATA
    plp
    rtl

;------------------------------------------------------------------------------
; u16 profileGetScanline(void)
;
; Latch and read the PPU vertical position counter.
; Returns 9-bit value (0-261 NTSC, 0-311 PAL) in A.
;------------------------------------------------------------------------------
profileGetScanline:
    php
    sep #$20                    ; 8-bit A for register reads
    lda.l REG_SLHV              ; latch H/V counters
    lda.l REG_OPVCT             ; read low byte
    rep #$20                    ; 16-bit A
    and #$00FF                  ; mask to 8 bits
    pha                         ; save low byte
    sep #$20
    lda.l REG_OPVCT             ; read high bit
    rep #$20
    and #$0001                  ; bit 0 only
    .repeat 8
    asl a
    .endr                       ; shift to bit 8
    ora 1,s                     ; combine with low byte
    sta 1,s
    pla                         ; result in A
    plp
    rtl

;------------------------------------------------------------------------------
; void profileScanlineStart(void)
;
; Save current scanline for later measurement.
;------------------------------------------------------------------------------
profileScanlineStart:
    php
    sep #$20
    lda.l REG_SLHV              ; latch
    lda.l REG_OPVCT             ; low byte
    rep #$20
    and #$00FF
    sta.w profile_scanline_start
    sep #$20
    lda.l REG_OPVCT             ; high bit
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
;
; Returns elapsed scanlines since profileScanlineStart().
; Result in A (16-bit).
;------------------------------------------------------------------------------
profileScanlineEnd:
    php
    ; Read current scanline
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
    sta 1,s                     ; current scanline on stack

    ; Subtract start scanline
    pla
    sec
    sbc.w profile_scanline_start

    ; Handle wrap (current < start means we crossed frame boundary)
    bpl @no_wrap
    clc
    adc #262                    ; NTSC frame = 262 scanlines
@no_wrap:
    plp
    rtl

;------------------------------------------------------------------------------
; u16 profileGetFrameCount(void)
;
; Returns frame_count from crt0.asm system variables.
;------------------------------------------------------------------------------
profileGetFrameCount:
    rep #$20
    lda.w frame_count
    rtl

;------------------------------------------------------------------------------
; u16 profileGetLagFrames(void)
;
; Returns lag_frame_counter from crt0.asm system variables.
;------------------------------------------------------------------------------
profileGetLagFrames:
    rep #$20
    lda.w lag_frame_counter
    rtl

.ENDS
