;------------------------------------------------------------------------------
; streams.asm — hand-built LZ77 streams for lzssDecodeRam's edge cases
;
; Format (lzss.h): tag $10, 3-byte little-endian output length, then a flag
; byte before every eight items, read MSB first (0 = one literal byte,
; 1 = a reference: b0 = (length - 3) << 4 | (distance - 1) >> 8,
; b1 = (distance - 1) & $FF).
;
; The tile streams come from opensnes-sprite (res/*.toml). Its compressor
; never emits a distance of 1, and rarely a distance shorter than the length:
; the overlapping cases are written here by hand, so that they are tested at
; all.
;
; Literal bytes follow one formula, P(k) = (k * 37 + 11) & $FF, which main.c
; recomputes to check the output without a second copy in ROM.
;------------------------------------------------------------------------------

ASSET_SECTION "lzr_streams"

;--- 4096 literals: 512 flag bytes of 0, each followed by 8 bytes P(k) -------
lit_lz:
    .db $10, $00, $10, $00              ; 4096 bytes
.REPT 512 INDEX g
    .db $00
    .db ((g * 8 + 0) * 37 + 11) & $FF, ((g * 8 + 1) * 37 + 11) & $FF
    .db ((g * 8 + 2) * 37 + 11) & $FF, ((g * 8 + 3) * 37 + 11) & $FF
    .db ((g * 8 + 4) * 37 + 11) & $FF, ((g * 8 + 5) * 37 + 11) & $FF
    .db ((g * 8 + 6) * 37 + 11) & $FF, ((g * 8 + 7) * 37 + 11) & $FF
.ENDR

;--- long references: 32 literals P(0..31), then 224 references of 18 bytes
;    at distance 32 (no overlap). Output: P(k mod 32), 32 + 224 * 18 = 4064.
ref_lz:
    .db $10, $E0, $0F, $00              ; 4064 bytes
.REPT 4 INDEX g
    .db $00
    .db ((g * 8 + 0) * 37 + 11) & $FF, ((g * 8 + 1) * 37 + 11) & $FF
    .db ((g * 8 + 2) * 37 + 11) & $FF, ((g * 8 + 3) * 37 + 11) & $FF
    .db ((g * 8 + 4) * 37 + 11) & $FF, ((g * 8 + 5) * 37 + 11) & $FF
    .db ((g * 8 + 6) * 37 + 11) & $FF, ((g * 8 + 7) * 37 + 11) & $FF
.ENDR
.REPT 28
    .db $FF                             ; eight references
    .db $F0, $1F, $F0, $1F, $F0, $1F, $F0, $1F
    .db $F0, $1F, $F0, $1F, $F0, $1F, $F0, $1F
.ENDR

;--- the same, 194 bytes: 32 literals and 9 references. Longer than 144 bytes,
;    so the first references go through the fast loop: into a bank other than
;    $7E/$7F, its byte-loop copy.
ref194_lz:
    .db $10, 194, $00, $00
.REPT 4 INDEX g
    .db $00
    .db ((g * 8 + 0) * 37 + 11) & $FF, ((g * 8 + 1) * 37 + 11) & $FF
    .db ((g * 8 + 2) * 37 + 11) & $FF, ((g * 8 + 3) * 37 + 11) & $FF
    .db ((g * 8 + 4) * 37 + 11) & $FF, ((g * 8 + 5) * 37 + 11) & $FF
    .db ((g * 8 + 6) * 37 + 11) & $FF, ((g * 8 + 7) * 37 + 11) & $FF
.ENDR
    .db $FF
    .db $F0, $1F, $F0, $1F, $F0, $1F, $F0, $1F
    .db $F0, $1F, $F0, $1F, $F0, $1F, $F0, $1F
    .db $80
    .db $F0, $1F

;--- a reference that runs past the end: length 5 = 'A' + 18 x 'A' cut to 4 ---
clip_lz:
    .db $10, $05, $00, $00
    .db %01000000                       ; L R
    .db $41
    .db $F0, $00                        ; length 18, distance 1

;--- overlapping references (distance < length), 44 bytes ---------------------
; items: L R L L R L L L | R R
ovl_lz:
    .db $10, 44, $00, $00
    .db %01001000                       ; L R L L R L L L
    .db $41                             ; 'A'
    .db $F0, $00                        ; length 18, distance 1: 18 x 'A'
    .db $42, $43                        ; 'B' 'C'
    .db $70, $01                        ; length 10, distance 2: BCBCBCBCBC
    .db $44, $45, $46                   ; 'D' 'E' 'F'
    .db %11000000                       ; R R (the last six flags unused)
    .db $40, $02                        ; length 7, distance 3: DEFDEFD
    .db $00, $00                        ; length 3, distance 1: DDD
ovl_expected:
    .db $41, $41, $41, $41, $41, $41, $41, $41, $41, $41
    .db $41, $41, $41, $41, $41, $41, $41, $41, $41
    .db $42, $43, $42, $43, $42, $43, $42, $43, $42, $43, $42, $43
    .db $44, $45, $46, $44, $45, $46, $44, $45, $46, $44
    .db $44, $44, $44

;--- long overlapping references, 330 bytes: longer than 144, so the first two
;    flag groups go through the fast loop and its MVN
; 'A' + 7 x (18 at distance 1)            = 127 x 'A'
; 'B' 'C' + 6 x (18 at distance 2)        = 55 x "BC"
; 'D' 'E' 'F' + 5 x (18 at distance 3)    = 31 x "DEF"
ovlong_lz:
    .db $10, $4A, $01, $00              ; 330 bytes
    .db %01111111
    .db $41
    .db $F0, $00, $F0, $00, $F0, $00, $F0, $00, $F0, $00, $F0, $00, $F0, $00
    .db %00111111
    .db $42, $43
    .db $F0, $01, $F0, $01, $F0, $01, $F0, $01, $F0, $01, $F0, $01
    .db %00011111
    .db $44, $45, $46
    .db $F0, $02, $F0, $02, $F0, $02, $F0, $02, $F0, $02

;--- refused streams ----------------------------------------------------------
zero_lz:                                ; length 0: returns 0, writes nothing
    .db $10, $00, $00, $00
big_lz:                                 ; length 65536: refused before any write
    .db $10, $00, $00, $01
    .db $00, $01, $02, $03, $04, $05, $06, $07, $08
tag_lz:                                 ; tag $11 (LZ11, another format): refused
    .db $11, $04, $00, $00
    .db $00, $01, $02, $03, $04

.ENDS
