;----------------------------------------------------------------------
; assets.inc — where bulk read-only data belongs
;
; Included automatically in every assembled file (make/common.mk,
; wrap_asm), so ASSET_SECTION is available with no .INCLUDE of your own.
;
; ## The problem it names
;
; `.SECTION "x" SUPERFREE` lets the linker choose a bank, and it chooses
; the first one that fits — bank $00. That is the same 32 KB your code
; lives in. A map, a tile sheet or a collision grid dropped in there
; costs you code space for nothing, and the cost is invisible: the build
; succeeds and the game runs, right up until one more const array pushes
; past the boundary. The RPG template sat at 12 free bytes in bank $00
; with 12 KB of map data parked there, for no reason at all.
;
; It is "for no reason" because that data never needed to be in bank $00:
;
;   - anything you hand to a lib DMA function (tiles, tilemaps,
;     palettes, fonts) travels as a FAR pointer — dmaCopyVram and
;     friends read the bank byte, so the source can live anywhere;
;   - anything you read from C through a `const` pointer compiles to a
;     bank-honouring far read since #121.
;
; Bank $00 is for code and for const data that C dereferences through a
; NON-const pointer. Everything else should be somewhere else.
;
; ## Use
;
;     ASSET_SECTION "townmap"
;     town_map:    .incbin "res/town_map.bin"
;     .ENDS
;
; The linker packs them itself, highest bank first, falling back down
; toward 1. Bank $00 is not in the list, which is the whole point.
;
; If you get it wrong, the post-link check tells you: every build prints
; how much asset data ended up in bank $00, and the existing ratchet
; hard-fails once free space runs out.
;
; ## When NOT to use it
;
; Data that C dereferences through a plain (non-const) pointer MUST stay
; in bank $00 — the compiler emits `lda.l $0000,x` for it and will read
; garbage from anywhere else. The build's bank-blind check catches that
; case, so if you are unsure: move it out, and let the build tell you.
;
; ## HiROM
;
; Verified on LoROM, SA-1 and HiROM (#127.3, 2026-09-07). HiROM units carry
; `.BASE $C0` (memmap_hirom.inc) so payload at offset $0000 of a high bank
; is addressed in the full 64 KB view ($Cn:0000); the wlalink fork keeps
; that base off RAMSECTION labels. Since #127.3 the compiler places its own
; const data (`.rodata.N`) with this very directive, so ASSET_SECTION is
; only needed for hand-written payload (.incbin, .db tables).
;----------------------------------------------------------------------

; SEMISUPERFREE with an explicit bank order: the linker tries 7 first and
; walks down, so payload packs away from the code bank and bank $00 is
; never a candidate. You do not pick a bank — an earlier version of this
; macro made you, which just moved the guesswork to the caller.
;
; The list is literal because it has to be: a `.DEFINE` is not expanded
; inside a BANKS clause (wlalink reports "malformed BANKS list"), and a
; bank outside the memory map is a hard link error ("out of range").
; 1-7 exist in every ROM memory map this SDK ships (LoROM, HiROM and
; SA-1 all declare 8 banks); if you write your own with fewer, narrow
; this list to match.
;
; The section name is prefixed with "asset." so the post-link report can
; identify declared payload exactly, instead of guessing from names (an
; earlier heuristic flagged `.text.bgSetMapPtr` because it contains "map").
.MACRO ASSET_SECTION ARGS name
.SECTION "asset.\1" SEMISUPERFREE BANKS ASSET_BANKS
.ENDM
