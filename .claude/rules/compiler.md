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
- **A submodule commit is staged in the superproject before any `make`**: the `submodules` target runs `git submodule update`, which puts a committed-but-unstaged submodule back on the old pin (detached HEAD) and the build then uses the old compiler. Uncommitted edits survive; commits do not until `git add compiler/<name>`. **And `git stash` / `git stash pop` undoes that staging** (2026-10-09): `pop` restores the working tree, not the index, unless `--index` is given, so the submodule pointer comes back unstaged and the next `make` checks the old pin out. A whole final validation ran on the previous compiler that way and was caught only by `verify-toolchain`. Do not stash on a branch that has a staged submodule; copy the files you want to compare instead.
- **A function has at most 2048 temporaries** (since 2026-10-08; 256 before, unchecked): `MAX_ALLOC_TEMPS` sizes every per-temp table of the backend, and `w65816_check_temps` stops the build beyond it. A guard of the form `if (idx < MAX)` that silently skips is a miscompilation in waiting: refuse instead.
- **Passes that change the CFG run before `gvn`, on plain SSA** (since 2026-10-08): `threadjnz` (`cfg.c`) lets each predecessor of a block that only chooses (one phi, the `jnz` on it) take the decision itself. After `gvn` one definition stands for every equal value wherever it was first met, so a block cut off there can hold the only definition of a value still used — `gcm` then asserts (`b0 != NOBID`). And `gvn` itself infers from the shape of the CFG: its phi-to-condition rule (`phicopyref`) trusted a phi argument whose edge it had just folded away, and returned `y != 0` for `(y || K) && 1`. A new shape reaching `gvn` gets a differential hunt of ten thousand programs, not two hundred.
- **An indexed addressing mode adds an UNSIGNED 16-bit index** (since 2026-10-08): `sym,x` and `[tcc__r9],y` carry into the bank byte, so they equal the 24-bit sum only for an index in 0..65535. `mark_far_decomp` keeps them for an index proven non-negative (`idx_nonneg`: a zero extension or a Kw temp, through non-negative constants) or straight off a symbol with no offset; `(far_arr + 8)[-1]` read bank $7F for a month. Any new use of X or Y as an index (stage S4 and after) asks the same question first.
- **A near `sym[index]` is a far access to the emitter** (since 2026-10-08): `mark_near_indexed` runs first in `emitfn` and `is_far()` answers yes for a load or store whose address is `$sym + index` (`sym_idx_form`), so the access takes the B2 path and its `lda.l sym,x`. Everything keyed on `is_far()` — the dead-store rule for stores, the address-only analysis, `far_decomp_all` — therefore applies to it unchanged; a field through a pointer (`NEAR_PTR`, `lda.l N,x`) and any other address in a temp (`NEAR_DEREF`, `lda.l $000000,x`) entered the same way the same day, except for an address a 32-bit load goes through (that load reads the full 24-bit pointer, `lda [tcc__r9]`, and keeps to). X is remembered between accesses (`x_load`, `x_keeps`): every instruction that is not itself an X-indexed access drops it, so a new instruction never has to declare that it writes X. `QBE_NO_NEAR_INDEXED=1` turns it off for an A/B.
- **`rep #$20` at function entry is kept wherever assembly could enter** (owner-level decision taken by the chief engineer on 2026-10-09, against item (b) of the plan): moving it to the caller for every function would have gained 3 cycles a call — under 1 % of the measured workloads — and added a silent failure, any assembly calling C in 8-bit mode. Only a function that is not exported and whose address is never used (`addrtaken()`, `main.c`: a data reference, or any code reference that is not a direct call's target) leaves it out. Anything that hands a C function to assembly does so through its address, so no new rule for hand-written assembly, and no lint, is needed.
- **The frame of a leaf function is in the direct page** (since 2026-10-09): `fn->leaf`, no alloc, at most 16 words → temps at `tcc__lf` (`$0080`, `templates/crt0.asm`), `framesize` 0. `SOFF(slot)` is the only way to turn a slot index into an operand and `emit_stack_load/store/op` the only places that print one: a new emit path that writes `(slot + 1) * 2` by hand puts a leaf's temp on the caller's stack. Three things hold it up, and each has a guard: a leaf calls nothing (the runtime helpers use `tcc__r0..r3`, `r9`, `r10`, not this block); the NMI handler's direct page mirrors the block (`tcc__nmi_registers`, 160 bytes — fixture `leaf_frame` runs the same leaf in the main loop and in the NMI callback); an IRQ handler is assembly. `$0040-$007F` is SNESMOD's, which is why the block is at `$80` and `.registers` must stay under `$40`.
- **A narrow 32-bit temp** (since 2026-10-09): a Kl temp whose high half nobody reads (`temp_addr_only`) and whose defining operation never reads back its own high half (`narrow_ok`: not a right shift, a divide, a call) takes one word of frame; `emit_store_high` returns at once for it. Every write of a high half therefore goes through `emit_store_high` — a direct `sta (slot+1)*2+2` on an instruction's result would land in the next temp's slot. Phi results are never narrow (the edge moves write both halves directly). A read of a narrow temp's high half is the high-half invariant's error.
- **A temp without a slot, and a function without a frame** (since 2026-10-08): the alias and dead-store analyses run BEFORE slot assignment and the temps they name get no slot (`temp_noslot`); reading one from the stack is an internal error, where a skipped store used to mean a silent read of whatever the slot held. Two things follow. An emit path that reads an operand's LOW half twice breaks the promise `consumes_r0_via_emitload()` makes (the Omul twin of the 2026-05-22 Oshl defect was still doing it): load once, keep the value in a direct-page scratch. And a function left with no slot has `framesize` 0, not the 2 bytes of alignment — parameters are read at `framesize + PARAM_OFFSET + n`, and a function with parameters, calls and nothing on the stack (`textInit`) read them 2 bytes too high for an afternoon. Parameter aliasing is `alias_opt` (every function); `leaf_opt` only gates going frameless and the skipped store before a return.
- **A function is emitted into a temporary file and read back** (since 2026-10-09, `relax_branches`): the triple `bxx + / jmp @target / +` that `emitjmp` writes becomes `byy @target` when an UPPER BOUND of the bytes in between (`line_bytes`: the width suffix, else the operand's shape, 4 when unknown) is within 124. Two consequences for whoever touches the emitter: a new instruction form must be boundable by `line_bytes` or it counts 4, and a line that carries both an anonymous label and an instruction (`+<tab>lda.w #1`) is an instruction — the first version skipped it and judged a 129-byte branch near. The assembler refuses a branch out of reach, so a wrong bound fails the link; it does not miscompile. `QBE_NO_SHORT_BRANCH=1` turns the pass off.
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
