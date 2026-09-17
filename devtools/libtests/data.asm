;==============================================================================
; libtest — map-module test data (issue #103 regression pin)
;
; Real tmx2snes output, shared with examples/maps/map_scroll (committed
; assets, referenced in place — no duplication in the repo). Pinned OUT of
; bank $00 on purpose: mapLoad honours the far pointer's bank byte (B1),
; and the #103 getters must work regardless of where the map data lives —
; the bug was in the WRAM-table reads, not the map reads.
;==============================================================================

.section ".rodata2" semifree bank 2

mapdata:
.incbin "../../examples/maps/map_scroll/res/BG1.m16"
mapdata_end:

tilesetdef:
.incbin "../../examples/maps/map_scroll/res/tiledMario.t16"
tilesetdef_end:

tilesetatt:
.incbin "../../examples/maps/map_scroll/res/tiledMario.b16"
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
