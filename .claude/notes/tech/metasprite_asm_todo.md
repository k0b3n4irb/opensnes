---
name: oamMetaDrawDyn* ASM optimization TODO
description: Dynamic metasprite functions are implemented in C — ASM version planned for performance. Documents the WLA-DX bug and fix strategy.
type: project
---

## Current state (2026-03-21)

`oamMetaDrawDyn32/16/8` are implemented in **C** (`lib/source/sprite_dynamic_meta.c`).
They iterate MetaspriteItem arrays and call `oamDynamic32/16/8Draw` per sub-sprite.
Functionally correct, validated in Mesen2, all 3 OBJSEL configs work.

PVSnesLib implements these in **assembly** (~200 lines each in `sprites.asm`).

## Why the ASM version failed

The ASM implementation was ~470 lines in `sprite_dynamic.asm` (removed).
It produced misaligned instructions due to **WLA-DX `.ACCU` tracking loss**
after branch merge points (e.g. `bcs _xhigh` / `jmp _attr` reconvergence).

WLA-DX tracks `rep #$20`/`sep #$20` to determine immediate operand size
(1 vs 2 bytes). After branches where both paths have different `sep`/`rep`
history, WLA-DX loses track. A `lda #$00` (2 bytes) encoded as
`lda #$0000` (3 bytes) shifts ALL subsequent instructions by 1 byte.

This is the same class of bug as the `.INDEX 16` shift loop issue (fixed
in emit.c by emitting `.ACCU 16` + `.INDEX 16` before every function).

## Fix strategy for future ASM optimization

1. Add **explicit `.ACCU 8`/`.ACCU 16`** after EVERY `sep #$20`/`rep #$20`
2. Add them especially **after branch merge labels** (where two code paths rejoin)
3. Add **`.INDEX 8`/`.INDEX 16`** after any `sep #$10`/`rep #$10`
4. Verify by dumping the binary and checking opcode bytes at branch merges
5. Test each function individually in Mesen2 before combining

## Other issues found during implementation

- **Ternary + leaf_opt compiler bug**: `drawText()` with ternary expressions
  was leaf-optimized (framesize=0) despite having function calls. The compiler
  stored SSA temporaries at `sta 2,s` which overwrote the JSL return address.
  Workaround: use if/else blocks instead of ternary in functions with calls.

- **Bank $7E variables inaccessible from C**: `spr16addrgfx` at $7E:2B16 is
  above the WRAM mirror ($0000-$1FFF). C code generates `lda.l $2B16` in
  bank $00 which reads unmapped expansion area = garbage. Fix: added
  `sprsize` parameter to `oamMetaDrawDyn16` instead of auto-detecting.

## Files

- `lib/source/sprite_dynamic_meta.c` — current C implementation
- `lib/include/snes/sprite.h` — declarations (lines 634-711)
- `templates/crt0.asm` — `sprit_mxsvg`/`sprit_mysvg` in RAMSECTION (for future ASM)
- `examples/graphics/sprites/dynamic_metasprite/` — port of PVSnesLib example
