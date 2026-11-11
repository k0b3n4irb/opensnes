; The string_nmi fixture's ROM source: a table in an asset bank (never bank
; $00), so that the callback's block move has banks of its own.

ASSET_SECTION "sn_rom"
sn_rom:
.rept 64 INDEX i
    .db (i * 11 + 5) & $FF
.endr
.ENDS
