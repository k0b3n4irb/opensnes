;----------------------------------------------------------------------
; sample_swap - four BRR samples written by hand, block by block, so that
; their bytes are a known pattern the manifest compares with ARAM.
;
; A BRR block is 9 bytes: a header (range in bits 7-4, filter in bits 3-2,
; loop in bit 1, end in bit 0), then 16 signed 4-bit samples, high nibble
; first. Filter 0 everywhere: each sample is its nibble times 2^range,
; with no prediction from the previous ones. The last block of each sample
; carries end + loop ($x3): the voice goes back to the loop point (0) and
; holds its note until it is keyed off.
;
; Block j's amplitude is 1, 3, 5 or 7 (j & 3), so no two neighbouring
; blocks are equal and a sample read one byte off its address shows.
;----------------------------------------------------------------------

ASSET_SECTION "sample_swap_brr"

; lead: 8 blocks, range 9, a square of 16 samples (8 up, 8 down)
lead_brr:
.rept 7 INDEX j
    .db $90
    .db ((1 + (j & 3) * 2) * 17) & $FF, ((1 + (j & 3) * 2) * 17) & $FF
    .db ((1 + (j & 3) * 2) * 17) & $FF, ((1 + (j & 3) * 2) * 17) & $FF
    .db ((15 - (j & 3) * 2) * 17) & $FF, ((15 - (j & 3) * 2) * 17) & $FF
    .db ((15 - (j & 3) * 2) * 17) & $FF, ((15 - (j & 3) * 2) * 17) & $FF
.endr
    .db $93, $77, $77, $77, $77, $99, $99, $99, $99
lead_brr_end:

; pad: 8 blocks, range 10, a square of 4 samples (up, up, down, down)
pad_brr:
.rept 7 INDEX j
    .db $A0
.rept 4
    .db ((1 + (j & 3) * 2) * 17) & $FF, ((15 - (j & 3) * 2) * 17) & $FF
.endr
.endr
    .db $A3, $77, $99, $77, $99, $77, $99, $77, $99
pad_brr_end:

; bell: 8 blocks, range 11, a square of 8 samples (4 up, 4 down) - the
; sample that replaces pad in pad's place
bell_brr:
.rept 7 INDEX j
    .db $B0
.rept 2
    .db ((1 + (j & 3) * 2) * 17) & $FF, ((1 + (j & 3) * 2) * 17) & $FF
    .db ((15 - (j & 3) * 2) * 17) & $FF, ((15 - (j & 3) * 2) * 17) & $FF
.endr
.endr
    .db $B3, $77, $77, $99, $99, $77, $77, $99, $99
bell_brr_end:

; swap: bell's bytes again, the sample the silence cases write in pad's
; place. A copy of its own so that a control build can give it pad's bytes
; (then nothing the voice reads changes) without moving any code.
swap_brr:
.rept 7 INDEX j
    .db $B0
.rept 2
    .db ((1 + (j & 3) * 2) * 17) & $FF, ((1 + (j & 3) * 2) * 17) & $FF
    .db ((15 - (j & 3) * 2) * 17) & $FF, ((15 - (j & 3) * 2) * 17) & $FF
.endr
.endr
    .db $B3, $77, $77, $99, $99, $77, $77, $99, $99
swap_brr_end:

; bulk: 192 blocks (1728 bytes) for the timing of a load in steps against a
; note played on another voice; never played
bulk_brr:
.rept 191 INDEX j
    .db $80, j & $FF, (j * 3) & $FF, (j * 5) & $FF, (j * 7) & $FF
    .db (j * 11) & $FF, (j * 13) & $FF, (j * 17) & $FF, (j * 19) & $FF
.endr
    .db $83, 0, 0, 0, 0, 0, 0, 0, 0
bulk_brr_end:

.ends
