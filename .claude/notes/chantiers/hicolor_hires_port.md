# Chantier: ports #117 (HiColorPseudoHiRes) + #118 (HiColor3840)

Status: IN PROGRESS (2026-07-15). Branch wip/hicolor-hires.

## #117 reference facts (full-ASM read — premise REFINED again)

Mode **1** (not 3): BGMODE=$09 (BG1+BG2 4bpp, priority). SETINI=$08
(pseudo-hires). The 512x224 image is split by COLUMN PARITY:
even cols → BG2 tiles (word $4000) → SUB screen; odd cols → BG1 tiles
(word $0000) → MAIN. SAME tilemap for both (word $3C00, loaded at
$3C80=row 4, VOFS=31 — the hicolor_1792 layout exactly). PLUS colormath:
CGWSEL=$02 (source=subscreen), CGADSUB=$61 (ADD + HALF on BG1+backdrop)
— softens column fringing, and dogfoods colormath (shared with #118).
H-IRQ CGRAM streaming identical to hicolor_1792 (HTIME=190, 16 B/line,
(V&15)==8 reset, VBlank 128 B rewind, 3584 B palette shared by both BGs).
krom's converter: GFX/SNESBGPAL64tilerowHiRes.py — quantize per 128x8
segment of the 512 image, emit odd cols to BG1.pic / even to BG2.pic.

## Port plan #117 → examples/graphics/effects/hicolor_hires

- devtools/hicolor64hires.py (krom contract), 512x224 sunset art
- reuse hicolor_1792's irq_stream.asm + generated tilemap + VBlank cb
- Mode 1 + videoSetPseudoHires(1) + TM/TS + colorMath{Enable(BG1|BACKDROP),
  SetOp(ADD), SetHalf(1), SetSource(SUBSCREEN)}
- validation: side-by-side with krom ROM in luna (content-level 256 view,
  bands/fringing physics), register table, docs 64

## #118 (after): Blend/HiColor3840 — Mode 3 BG1 240c main + BG2 16c sub,
CGWSEL $02 + CGADSUB $21 (ADD on BG1+backdrop, no half? check $21 vs $61
at read time), static. Cousins: 575Myst / 1241DLair photo variants.
