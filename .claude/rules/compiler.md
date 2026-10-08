---
paths:
  - "compiler/**/*"
  - "bin/cc65816"
---

# Compiler Rules

## Architecture

Three git submodules:
- `cproc/` — C11 frontend, parses C into QBE IR
- `qbe/` — Code generator with custom 65816 backend, emits .asm
- `wla-dx/` — WLA-DX assembler (wla-65816) and linker (wlalink)

The `bin/cc65816` driver (a compiled program since 2026-10-06, `compiler/cc65816/cc65816.c`; a bash script before) orchestrates the pipeline:
```
.c → cc -E → cproc → QBE IR → qbe w65816 → .asm → wla-65816 → .obj
```

The qbe w65816 backend emits WLA-DX syntax directly (`.db`, `.dw`, `.SECTION`)
— the historical sed post-transform is gone.

## Build

```bash
make compiler       # Build all three submodules
```

This is a Class A change — requires `make clean && make` + full test suite (luna) on ALL examples.

## Critical Constraints

- **Bank $00 ROM is code only** (since #127.3, v0.41.0): C const data is placed in the asset banks and every C read of it is a far read; `devtools/check_bank_reads.py` fails the link on a bank-blind read. The bank $00 free-space ratchet still guards the code bank.
- **Plain C RAM lives below $2000; `FAR` is the way above it** (since chantier B2): `sta.l $0000,x` reads bank $00, so a plain global sits in `$00:0000-$1FFF`; `FAR` objects go to `$7E:2000-$FFFF` with bank-honouring codegen.
- **Refused, never miscompiled**: variadic functions, struct parameters / returns / assignment by value, inline assembly, floating-point values computed at run time, 64-bit integers (`long long`) computed at run time (both since 2026-10-08: they compiled to 16- and 32-bit integer code until then; constants the compiler folds, `(int)(1.5 * 256)` or `-2147483648`, are not refused). cc65816 stops with a message naming the feature; `devtools/compiler-tests/cases/negative/` pins the refusal, `KNOWN_LIMITATIONS.md` (Type & ABI gotchas) gives the workaround.
- **LEFT-TO-RIGHT argument push**: cc65816 pushes function args left-to-right, NOT right-to-left like tcc816/PVSnesLib.
- **`volatile` is honoured** (since chantier A2, 2026-05-09): cproc tags volatile loads/stores with a `volat` IR keyword that QBE's loadopt/promote/gcm passes respect. The SDK still favours plain globals for NMI handshake patterns for cycle-cost equivalence, but user code can use `volatile` for MMIO without silent coalescing.
- **`unsigned int` = 2 bytes, `unsigned long` = 4 bytes** on this target (since chantier A1, 2026-05-08).
- **The IR means what upstream QBE says, at this target's widths** (since 2026-10-08): `w` is 16 bits, `l` is 32, and `jnz` tests a word whatever the class of its argument — cproc compares a 4-byte condition with zero first (`cnel`) and the backend folds that compare back into the branch. A fork-only rule the optimizer does not know (the old "jnz tests both halves of an `l` temp") is a miscompilation waiting for a copy propagation; QBE's folder computes at `T.wordsz`, and so does the width inference of `copy.c` (`defwidthle`, since 2026-10-08: it sized shifts at 32 bits and dropped the mask of `(v >> 15) & 1`). Any `32`, `31`, `64` or `63` left in a generic pass is upstream's word, to be read again for this target before the pass is trusted. Six defects of this family were found in a day by `testing/difftest.py`. The same holds inside the backend: a Kw temp standing where a Kl operand is expected is zero-extended and owns ONE slot word — every read of a high half goes through `emit_load_high` / `emitop2_high`, never through `slot + 1` by hand.
- **Phi moves are a parallel copy** (since 2026-10-08): `emitphimoves` orders the moves of an edge and breaks a cycle through `tcc__r10`. Emitted in list order they miscompiled `prev = cur; cur += d;` in any loop. `QBE_DBG_PHI=1` lists them. The two ways out of a conditional branch do not share the A-cache: `branch_fork()` before the branch, `branch_join()` at the `+` label.
- **A submodule commit is staged in the superproject before any `make`**: the `submodules` target runs `git submodule update`, which puts a committed-but-unstaged submodule back on the old pin (detached HEAD) and the build then uses the old compiler. Uncommitted edits survive; commits do not until `git add compiler/<name>`.
- **A function has at most 2048 temporaries** (since 2026-10-08; 256 before, unchecked): `MAX_ALLOC_TEMPS` sizes every per-temp table of the backend, and `w65816_check_temps` stops the build beyond it. A guard of the form `if (idx < MAX)` that silently skips is a miscompilation in waiting: refuse instead.
- **Passes that change the CFG run before `gvn`, on plain SSA** (since 2026-10-08): `threadjnz` (`cfg.c`) lets each predecessor of a block that only chooses (one phi, the `jnz` on it) take the decision itself. After `gvn` one definition stands for every equal value wherever it was first met, so a block cut off there can hold the only definition of a value still used — `gcm` then asserts (`b0 != NOBID`). And `gvn` itself infers from the shape of the CFG: its phi-to-condition rule (`phicopyref`) trusted a phi argument whose edge it had just folded away, and returned `y != 0` for `(y || K) && 1`. A new shape reaching `gvn` gets a differential hunt of ten thousand programs, not two hundred.
- **An indexed addressing mode adds an UNSIGNED 16-bit index** (since 2026-10-08): `sym,x` and `[tcc__r9],y` carry into the bank byte, so they equal the 24-bit sum only for an index in 0..65535. `mark_far_decomp` keeps them for an index proven non-negative (`idx_nonneg`: a zero extension or a Kw temp, through non-negative constants) or straight off a symbol with no offset; `(far_arr + 8)[-1]` read bank $7F for a month. Any new use of X or Y as an index (stage S4 and after) asks the same question first.
- **A near `sym[index]` is a far access to the emitter** (since 2026-10-08): `mark_near_indexed` runs first in `emitfn` and `is_far()` answers yes for a load or store whose address is `$sym + index` (`sym_idx_form`), so the access takes the B2 path and its `lda.l sym,x`. Everything keyed on `is_far()` — the dead-store rule for stores, the address-only analysis, `far_decomp_all` — therefore applies to it unchanged; a field through a pointer (`NEAR_PTR`, `lda.l N,x`) and any other address in a temp (`NEAR_DEREF`, `lda.l $000000,x`) entered the same way the same day, except for an address a 32-bit load goes through (that load reads the full 24-bit pointer, `lda [tcc__r9]`, and keeps to). X is remembered between accesses (`x_load`, `x_keeps`): every instruction that is not itself an X-indexed access drops it, so a new instruction never has to declare that it writes X. `QBE_NO_NEAR_INDEXED=1` turns it off for an A/B.
- **Compare and branch fuse only when adjacent** (since 2026-10-08): `gcm` orders a block by dependencies, so `cmplast` (`cfg.c`, skiprega targets) moves the compare a block branches on to its end. Fused forms: every 16-bit compare, a signed compare against 0 (N of the load), `cnel x, 0` and 32-bit `ceql` / `cnel` (two compares and a skip, label `@lcmp.N`). The other 32-bit compares still materialise 0 or 1.
- **No register allocator means no dead-code sweep for free** (since 2026-10-08): upstream QBE lets spill/rega drop a definition nobody uses, and this target skips both (`T.skiprega`). `gcm.c` therefore sweeps after `sink()` (`sweepdead`). A new middle-end transformation that can leave a definition without a use must be followed by that sweep, or the emitter will emit it — it has no "result unused" rule of its own.

## After Any Compiler Change

1. `make clean && make` — full rebuild (stale objects cause phantom bugs)
2. `make tests` — luna coverage + visual regression + probes
3. Present the triaged representative examples to the user (luna visual
   pass is the reference; interactive spot-check via `luna mcp` / luna GUI
   if needed — see docs/tutorials/debugging.md)
4. Check for regressions in code generation with compiler test patterns
5. `python3 testing/difftest.py --seeds 1-2000` and `python3 testing/difftest_stmt.py --seeds 1-2000` — the two differential tests beyond their gates (about a minute each)

## Width literals in cproc are ratcheted (since 2026-10-05)

`make lint-cproc-widths` (in `make lint`) lists every `'w'` / `'l'` /
`ILOADW` / `ISTOREW` / `ILOADL` / `ISTOREL` outside `qbetype()` in
`compiler/cproc/qbe.c` and fails on one that is not in
`devtools/cproc_width_sites.txt`. Those sites are where upstream's model
(`l` = 8 bytes) survives unless re-read for this target (`int` 2, `long`
4, pointer 4): `funccopy`'s chunk table, `zero()`, the bit-field
extraction and the cast scaling were each found by a consumer, one at a
time (compiler audit 2026-10-03, PF3). A new site is reviewed, then added
with `--update`; `testing/fixtures/compiler/d_quals` is where its
runtime effect gets a cell.
