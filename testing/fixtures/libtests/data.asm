;==============================================================================
; libtest — map-module test data (issue #103 regression pin)
;
; Real opensnes-level output, shared with examples/maps/map_scroll (its
; rooftops, referenced in place — no duplication in the repo). Pinned OUT of
; bank $00 on purpose: mapLoad honours the far pointer's bank byte (B1),
; and the #103 getters must work regardless of where the map data lives —
; the bug was in the WRAM-table reads, not the map reads.
;==============================================================================

.section ".rodata2" semifree bank 2

mapdata:
.incbin "../../../examples/maps/map_scroll/res/BG1.m16"
mapdata_end:

tilesetdef:
.incbin "../../../examples/maps/map_scroll/res/rooftops.t16"
tilesetdef_end:

tilesetatt:
.incbin "../../../examples/maps/map_scroll/res/rooftops.b16"
tilesetatt_end:

.ends

; audio v2 phase-2 vector: a single looping BRR block (header $C3 =
; range $C, filter 0, loop+end; +7/-7 nibbles — the pitch_mod LFO
; square). 9 bytes streamed through audioLoadSample in libtest.
.section ".rodata_beep" superfree

beep_brr:
.db $C3, $77, $99, $77, $99, $77, $99, $77, $99
beep_brr_end:

.ends

; L2c (2026-09-15): raw IRQ handler for the interrupt.h asserts. Contract
; per interrupt.h — save/restore what it touches, acknowledge TIMEUP,
; rti. Pinned in bank 0 so the plain irqSet() (bank-0 assumption) is
; exercised as well as irqSetBank(). Counts every IRQ into irq_count
; (a C u16 in bank-0 WRAM, read back through the WRAM mirror).
.section ".irq_test" semifree bank 0

irqTestHandler:
    rep #$20
    .ACCU 16
    pha                     ; interrupted C code is arbitrary: full 16-bit A
    sep #$20
    .ACCU 8
    lda.l $004211           ; TIMEUP: acknowledge the IRQ line
    rep #$20
    .ACCU 16
    lda.l irq_count
    inc a
    sta.l irq_count
    pla
    rti

.ends

; The same handler again, forced OUT of bank 0 (ASSET banks 7-1), counting
; into irq_count_far: plain irqSet() must reach it through the bank byte of
; the far pointer it is given. It used to pass a literal bank 0 — the IRQ
; then entered bank $00 at this offset (bank-byte chantier, 2026-09-20).
.section ".irq_test_far" semisuperfree banks 7-1

irqTestHandlerFar:
    rep #$20
    .ACCU 16
    pha
    sep #$20
    .ACCU 8
    lda.l $004211           ; TIMEUP: acknowledge the IRQ line
    rep #$20
    .ACCU 16
    lda.l irq_count_far
    inc a
    sta.l irq_count_far
    pla
    rti

.ends

; The map engine's streaming (2026-11-11): a map of 80 x 40 entries and its
; metatile table, made by formula so that test_libtest.py can compute every
; tilemap word the engine must leave in VRAM. Entries carry both flip bits
; and junk in bits 10-13 (which the engine masks off); metatile words carry
; flip bits too (OR-ed with the entry's). The formulas are mapgen_entry()
; and mapgen_tile() of test_libtest.py.
.section ".rodata_mapgen" semisuperfree banks 7-1

mapgen:
.dw 80 * 8, 40 * 8, 80 * 40 * 2
.REPEAT 80 * 40 INDEX MG
.dw (((MG * 37) & $3FF) | (((MG * 5) & 15) << 10) | (((MG >> 3) & 3) << 14))
.ENDR

mapgen_tiles:
.REPEAT 2048 INDEX MT
.dw (((MT * 11) & $3FF) | ((MT & 7) << 10) | (((MT >> 3) & 3) << 14))
.ENDR

mapgen_props:
.dsw 1024, 0

.ends
