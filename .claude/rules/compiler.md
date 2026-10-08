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
- **Refused, never miscompiled**: variadic functions, struct parameters / returns / assignment by value, inline assembly. cc65816 stops with a message naming the feature; `devtools/compiler-tests/cases/negative/` pins the refusal, `KNOWN_LIMITATIONS.md` (Type & ABI gotchas) gives the workaround.
- **LEFT-TO-RIGHT argument push**: cc65816 pushes function args left-to-right, NOT right-to-left like tcc816/PVSnesLib.
- **`volatile` is honoured** (since chantier A2, 2026-05-09): cproc tags volatile loads/stores with a `volat` IR keyword that QBE's loadopt/promote/gcm passes respect. The SDK still favours plain globals for NMI handshake patterns for cycle-cost equivalence, but user code can use `volatile` for MMIO without silent coalescing.
- **`unsigned int` = 2 bytes, `unsigned long` = 4 bytes** on this target (since chantier A1, 2026-05-08).
- **The IR means what upstream QBE says, at this target's widths** (since 2026-10-08): `w` is 16 bits, `l` is 32, and `jnz` tests a word whatever the class of its argument — cproc compares a 4-byte condition with zero first (`cnel`) and the backend folds that compare back into the branch. A fork-only rule the optimizer does not know (the old "jnz tests both halves of an `l` temp") is a miscompilation waiting for a copy propagation; QBE's folder computes at `T.wordsz`. Six defects of this family were found in a day by `testing/difftest.py`.
- **A submodule commit is staged in the superproject before any `make`**: the `submodules` target runs `git submodule update`, which puts a committed-but-unstaged submodule back on the old pin (detached HEAD) and the build then uses the old compiler. Uncommitted edits survive; commits do not until `git add compiler/<name>`.

## After Any Compiler Change

1. `make clean && make` — full rebuild (stale objects cause phantom bugs)
2. `make tests` — luna coverage + visual regression + probes
3. Present the triaged representative examples to the user (luna visual
   pass is the reference; interactive spot-check via `luna mcp` / luna GUI
   if needed — see docs/tutorials/debugging.md)
4. Check for regressions in code generation with compiler test patterns
5. `python3 testing/difftest.py --seeds 1-2000` — the differential test beyond its gate (about a minute)

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
