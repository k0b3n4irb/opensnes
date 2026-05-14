---
name: WLA-DX accumulator size tracking bug
description: WLA-DX loses track of 8/16-bit accumulator size after branch merges in complex 65816 ASM functions, causing instruction misalignment
type: feedback
---

WLA-DX accumulator/index size tracking fails in complex ASM functions with branches.

**Why:** WLA-DX tracks `rep #$20`/`sep #$20` to determine immediate operand size (1 vs 2 bytes). But after branch merge points (e.g., `bcs label` with fallthrough + `jmp` reconvergence), WLA-DX can lose track of the current accumulator width. If it thinks A is 16-bit when it's 8-bit, `lda #$00` (2 bytes) is encoded as `lda #$0000` (3 bytes), **shifting all subsequent instructions by 1 byte**. The CPU then executes misaligned garbage.

**Upstream issue filed**: https://github.com/vhelin/wla-dx/issues/704
Includes minimal reproduction case, expected vs actual binary output, and fix suggestions.

**How to apply:**
- This is the SECOND time this class of bug has occurred (first was `.INDEX 16` missing for `cpx` in shift loops)
- When writing complex 65816 ASM with multiple branches and `rep`/`sep` switches:
  1. Add explicit `.ACCU 8` after every `sep #$20`
  2. Add explicit `.ACCU 16` after every `rep #$20`
  3. Especially critical **after branch merge points** (labels where two code paths rejoin)
  4. Add `.INDEX 8`/`.INDEX 16` after `sep #$10`/`rep #$10` similarly
- The C implementation of `oamMetaDrawDyn*` works because the compiler manages its own `.ACCU`/`.INDEX` directives
- When converting these functions back to ASM for performance, add size directives throughout
- Simple ASM functions (few branches, no `rep`/`sep` switches mid-function) are generally safe
