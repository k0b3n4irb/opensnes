;==============================================================================
; ppu_reset.asm - ppuReset(): the PPU and the library's PPU shadows back to
; their power-on state (module `ppu_reset`, declared in console.h)
;==============================================================================
;
; crt0's InitHardware writes every PPU register at boot, once. consoleInit()
; rewrites only BGMODE, BG1SC, BG12NBA, MOSAIC, CGRAM and NMITIMEN, so a screen
; set up after another inherits everything else. ppuReset() writes the
; registers back to the values InitHardware writes and the library's shadows
; back to the value they hold when main() starts.
;
; A module of its own, not console: it writes the shadows of the background,
; window, color math, mosaic, Mode 7 and HDMA modules, which console cannot
; name without making every ROM link them (make/common.mk, _DEP_ppu_reset).
;
; Assembly, and the table in the function's own section, for the cost: a
; ROM that lists the module and never calls ppuReset() drops both (wlalink -d
; discards an unreferenced SUPERFREE section). A C `static const` table would
; go to a SEMISUPERFREE .rodata section, which wlalink -d never discards
; (wlalink/discard.c leaves SEMISUPERFREE out of the discardable statuses):
; measured, 140 bytes kept in a ROM that never called the C version.
;
; The register values are templates/crt0.asm's (InitHardware). The manifest
; testing/manifests/basics_ppu_reset.toml holds the two equal: it reads the
; PPU at main() and again after a screen that changed every register and a
; call to ppuReset(), against the same expected values.
;==============================================================================

.ifdef SA1
.include "memmap_sa1.inc"
.else
.ifdef HIROM
.include "memmap_hirom.inc"
.else
.include "memmap.inc"
.endif
.endif

.SECTION ".text.ppuReset" SUPERFREE

;------------------------------------------------------------------------------
; The register writes of InitHardware, in its order: pairs of
; (register - $2100, value). Write-twice registers appear twice (the scrolls,
; Mode 7). Left out on purpose: the data ports ($2104 OAMDATA, $2118/$2119
; VMDATA, $2122 CGDATA), which would write OAM, VRAM or CGRAM. One value
; differs from InitHardware: COLDATA ($2132) gets $E0 (the three planes
; selected, intensity 0) where crt0 writes 0, which selects no plane and
; leaves the fixed colour as it was; $E0 is what colorMathInit() writes, and
; gives black, the fixed colour luna reads at main() (fullsnes, COLDATA: bits 7-5 "0=No
; change, 1=Apply Intensity"; snesdev-wiki: "Black - 1 write: %111_00000").
;------------------------------------------------------------------------------
ppu_reset_writes:
    .db $00,$8F                         ; INIDISP: forced blank, brightness 15
    .db $01,$00, $02,$00, $03,$00       ; OBJSEL, OAMADDL, OAMADDH
    .db $05,$00, $06,$00                ; BGMODE 0, MOSAIC off
    .db $07,$00, $08,$00, $09,$00, $0A,$00  ; BG1SC-BG4SC
    .db $0B,$00, $0C,$00                ; BG12NBA, BG34NBA
    .db $0D,$00, $0D,$00, $0E,$00, $0E,$00  ; BG1 H/V scroll (and M7HOFS/M7VOFS)
    .db $0F,$00, $0F,$00, $10,$00, $10,$00  ; BG2
    .db $11,$00, $11,$00, $12,$00, $12,$00  ; BG3
    .db $13,$00, $13,$00, $14,$00, $14,$00  ; BG4
    .db $0E,$FF, $0E,$FF, $10,$FF, $10,$FF  ; vertical scrolls -1: tilemap
    .db $12,$FF, $12,$FF, $14,$FF, $14,$FF  ; row 0 on the first picture line
    .db $15,$80, $16,$00, $17,$00       ; VMAIN (+1 word after the high byte), VMADD 0
    .db $1A,$00                         ; M7SEL
    .db $1B,$00, $1B,$01                ; M7A = $0100
    .db $1C,$00, $1C,$00                ; M7B = 0
    .db $1D,$00, $1D,$00                ; M7C = 0
    .db $1E,$00, $1E,$01                ; M7D = $0100
    .db $1F,$00, $1F,$00, $20,$00, $20,$00  ; M7X, M7Y
    .db $21,$00                         ; CGADD 0 (no CGDATA write)
    .db $23,$00, $24,$00, $25,$00       ; W12SEL, W34SEL, WOBJSEL
    .db $26,$00, $27,$00, $28,$00, $29,$00  ; WH0-WH3
    .db $2A,$00, $2B,$00                ; WBGLOG, WOBJLOG
    .db $2C,$01                         ; TM: BG1
    .db $2D,$00, $2E,$00, $2F,$00       ; TS, TMW, TSW
    .db $30,$00, $31,$00                ; CGWSEL, CGADSUB
    .db $32,$E0                         ; COLDATA: fixed colour black (see above)
    .db $33,$00                         ; SETINI
ppu_reset_writes_end:

;------------------------------------------------------------------------------
; void ppuReset(void)
;------------------------------------------------------------------------------
ppuReset:
    php
    phb
    pea.w $0000
    plb
    plb                     ; bank $00: the registers and the shadows by `.w`
    sep #$20
    .ACCU 8
    rep #$10
    .INDEX 16

    ; HDMA first: a channel aimed at INIDISP or a scroll would otherwise
    ; rewrite the registers below on the next scanline.
    stz.w $420C             ; HDMAEN
    stz.w hdma_enabled_state
    stz.w hdma_wave_enabled
    stz.w hdma_active_buffer
    stz.w hdma_iris_buffer
    ; No scroll left for the NMI to write over the values below.
    stz.w bg_scroll_dirty

    ldx.w #0
@write:
    rep #$20
    .ACCU 16
    lda.l ppu_reset_writes,x        ; low byte: register - $2100
    and.w #$00FF
    tay
    sep #$20
    .ACCU 8
    lda.l ppu_reset_writes+1,x      ; its value
    sta.w $2100,y
    inx
    inx
    cpx.w #(ppu_reset_writes_end - ppu_reset_writes)
    bne @write

    ; The shadows, to their value at main(): what crt0 leaves (the WRAM
    ; clear, then the initialised data).
    lda #15
    sta.w console_brightness
    lda #1
    sta.w console_force_blanked
    stz.w console_fade_active
    stz.w setini_shadow
    stz.w bg3_opt
    stz.w bg12nba_shadow
    stz.w bg34nba_shadow
    stz.w mosaic_size
    stz.w mosaic_bg_mask
    stz.w colormath_cgwsel
    stz.w colormath_cgadsub
    stz.w window_w12sel
    stz.w window_w34sel
    stz.w window_wobjsel
    stz.w window_wbglog
    stz.w window_wobjlog
    stz.w m7_sin
    stz.w m7_cos
    rep #$20
    .ACCU 16
    stz.w m7_scale_x
    stz.w m7_scale_y
    ldx.w #6
@scroll:
    stz.w bg_scroll_x,x
    stz.w bg_scroll_y,x
    dex
    dex
    bpl @scroll

    plb
    plp
    ; Every sprite hidden; the NMI uploads the buffer at the next
    ; WaitForVBlank(), as for any other OAM change. A tail call: oamClear
    ; returns to our caller.
    jml oamClear

.ENDS
