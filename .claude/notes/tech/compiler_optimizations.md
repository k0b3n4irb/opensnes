# Compiler Optimizations (emit.c) — COMMITTED

## Phase 1: Dead jump elimination + A-register cache (-10.4% cycles, 2404→2153)
- Dead jump: `jmp @label` skipped when target is `b->link` (next block)
- A-cache: tracks Ref in A, skips redundant `lda N,s`. Invalidate at block/store/call
- **Critical fix**: `cmp.w #0` in emitjmp after emitload to ensure Z flag correct

## Phase 2: 8/16-bit mode tracking + byte immediate (-36% on byte-heavy functions)
- `amode_8bit` state var + `emit_sep20()`/`emit_rep20()` helpers (skip if already in mode)
- Consecutive `Ostoreb` stays in 8-bit — single sep at start, single rep at end
- `emitins()` calls `emit_rep20()` before non-byte ops; byte ops manage their own mode
- Ostoreb with constant: sep first, then `lda #N` (2 bytes) instead of `lda.w #N` (3 bytes)
- `emit_sep20()` invalidates A-cache (8-bit A semantics differ from 16-bit)
- Block boundaries + function epilogue: `emit_rep20()` ensures 16-bit at entry

## Phase 3: Leaf function optimization (-37.6% overall, 2153→1343 on benchmark)
- 4 sub-optimizations, all gated behind `leaf_opt = (fn->leaf && !fn->dynalloc)`
- **Void epilogue**: skip `tax`/`txa` pair for `Jret0` (saves 4 cycles per void func)
- **Param alias propagation**: cproc generates `alloc4 → storew param → loadw alloc → extuh`
  chains. Track which alloc slots are "param shadows" (`alloc_param[]` table), alias result
  temps to original caller-frame param slots. Eliminates all param-to-local copies.
  Must also propagate through Ocopy + Oextuh/Oextsh/Oextsw/Oextuw (both pre-scan + emission)
- **Dead return store elimination**: pre-pass `count_temp_uses()` + `temp_is_retval[]`;
  if last instruction's result temp used only once (as retval), skip `sta N,s` (A-cache keeps it)
- **Frame elimination**: `can_be_frameless()` checks all allocs are param-shadows + all temps
  aliased or dead-retval → sets `framesize=0`, skips `tsa;sec;sbc;tas` prologue + epilogue
- **Key insight**: cproc params arrive via `Oloadsw SLOT(-N)` (ABI lowering), but immediately
  get stored into alloc slots. Must track alloc_param[] separately from temp_alias[].
- **Critical fix: emitphimoves()** must check `temp_alias[]` for phi arguments. When a phi arg
  is aliased (its Oloadsw was skipped), `fn->tmp[...].slot` points to an uninitialized slot.
  Fix: load from `framesize + 3 + (-neg_slot)` (caller frame) instead.
- add_u16: 95→28 cycles (-70%), bitwise_and: 93→26 (-72%), mul_variable: 117→55 (-53%)

## Phase 4: Compiler parity optimizations (PVSnesLib output matching)
- **String literals to ROM** (cproc/decl.c): `QUALNONE` → `QUALCONST` in `stringdecl()`
- **Comparison+branch fusion**: skip boolean materialization, emit direct compare+branch
  - **CRITICAL FIX**: Guard `temp_use_count[cidx] == 0` (not `== 1`)
- **`pea.w` for constant args**: `pea.w N` instead of `lda #N; pha` (2 cycles + 1 byte saved)
- **`.l` → `.w` address shortening**: bank $00 symbols safe with DB=$00
- **Redundant `cmp.w #0` elimination**: `last_load_emitted` flag
- **`stz` for zero stores**: `stz.w symbol` for storing 0 to global symbols

## Phase 5a: Remove php/plp prologue, keep rep #$20 (-7 cycles per function)
- `PARAM_OFFSET` constant: `framesize + 2 + (-slot)` (was +3 with php byte on stack)
- **CRITICAL BUG**: NMI handler calls C after `sep #$20`. C prologue must have `rep #$20`.
- Leaf functions: add_u16 28→18 (-36%), bitwise_and 26→16 (-38%), empty_func 19→9 (-53%)

## Phase 5b: Extend optimizations to non-leaf functions (total -8.1% vs PVS)
- Removed `leaf_opt` gate. Guard only on `fn->dynalloc`.
- Non-leaf wrappers now frameless: call_add 92→45 (-51%), pea_constant_args 64→37 (-42%)

## Phase 7a: Dead store elimination + aggressive frame elimination (total -22.0%, 1634→1275)
- `temp_is_dead_store[]` array: marks temps whose stack slot stores can be skipped
- A temp is dead store if: not aliased, not phi arg, not r1, used once as r0, same block,
  immediately next real instruction
- Key wins: global_increment 49→18 (-63%), conditional 72→37 (-49%),
  call_add 92→46 (-50%), clamp 111→63 (-43%), compare_and_branch 105→57 (-46%)

## Build Notes
- **Build gotcha (fixed 2026-09-26)**: `make compiler` did not recompile changed QBE / cproc sources (its rules had no prerequisites); they now always hand over to the submodule Makefile. Do NOT build with a bare `make -C compiler/qbe`: it uses QBE's own flags (no `-static`, no `-O2`) and gives a different binary. And `make` re-syncs submodules to the superproject's recorded pointer — `git add compiler/qbe` after committing in the fork, before any build.
- **Binary name**: cc65816 script uses `bin/qbe` NOT `bin/qbe-w65816`!
- **Benchmark tools**: `tools/cyclecount/cyclecount.py`, `tools/benchmark/compare_compilers.sh`, `tests/benchmark/bench_functions.c` (29 functions)
- **Gotcha**: `git stash` in parent repo does NOT affect submodule working tree!

## Kl multiply by a small constant (2026-09-26, qbe 9a17010)

`(u32)x * k` inlines as the 16-bit shift-add sequence when k is not a power
of two, `is_inline_mul_const(k)` accepts it, and the product is proven below
2^16 (mark_high_zero: an 8-bit or 16-bit zero-extension, or a zero-extending
byte / half load, plus bitlen(k)) — or the destination is addr_only.

Measured by recompiling every C file under lib/source and examples with
the previous and the new qbe: **39 → 4** `jsl tcc_mul32`. (The commit
message of e01ffa29 says "28 → 0": that count came from a recursive grep
whose output the shell condensed. The 39 → 4 figure is from `find … -exec`
per file, both compilers, same flags.)

The 4 that remain, both expected:
- `examples/mode7/dsp1_ground` ×3: a byte times 515. The product can reach
  18 bits, so the real 32-bit multiply is required.
- `examples/games/rpg` `main` ×1: a byte times 6, which qualifies, but the
  function has more than `MAX_ALIAS_TEMPS` (256) temporaries and the width
  analysis only tracks the first 256. Raising the limit would help very
  large functions; not done (it sizes several per-temp tables).
