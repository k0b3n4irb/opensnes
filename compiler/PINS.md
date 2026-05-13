# Compiler Toolchain Pins

This file is the **source of truth** for which commit of each compiler submodule
the OpenSNES SDK is built against. Advancing a submodule pointer requires
updating this file in the same commit. `make verify-toolchain` enforces it,
and CI runs the check before every build.

## Why this exists

The audit (`~/opensnes_audit_2026-04-26.md` §2.1) flagged that the three
compiler submodules float on upstream without rationale:

> Si un commit upstream casse silencieusement la codegen 65816 (déjà arrivé :
> `mktype()` UB, struct init, signed promotion — corrigés *après* avoir cassé
> des ROMs), il n'y a aucun buffer.

A `git submodule update --remote` would silently advance any of them past the
point where the test suite is known to pass. The pin table below names
exactly which commit each submodule must be at for the SDK to be considered
"green". Anyone running `make verify-toolchain` gets a hard failure if drift
is detected.

## Pinned commits

The block between the BEGIN/END markers is parsed by
`devtools/verify_toolchain.py`. Format: `| path | sha | source |`. Do not
reformat without updating the script.

<!-- BEGIN PINS -->
| path | sha | source |
|------|-----|--------|
| compiler/cproc | 42a5c4641f1d60bbc33c4b7794f3413973591399 | github.com/k0b3n4irb/cproc:master |
| compiler/qbe | 444edea5dcb930ebe1dffea4bed9db9683141c86 | github.com/k0b3n4irb/qbe:main |
| compiler/wla-dx | ffe59ca1db32a4e7b40e16674acb844a5a0160ef | github.com/k0b3n4irb/wla-dx:master |
<!-- END PINS -->

## Local patches carried on top of upstream

These commits exist only on the OpenSNES forks and must survive any sync
with upstream. Listed newest-first.

### compiler/cproc — 10 patches (upstream merge-base: 7051114)

```
cceac4b  fix(65816): preserve volatile through QBE IR  (chantier A2)
7f26c16  fix(65816): align int/long type sizes with the w65816 target  (chantier A1)
3618c72  fix: eliminate all Clang warnings
ea95cac  fix: initialize all struct type fields in mktype() to prevent UB
801c3e6  fix: add cleanup functions to free maps, arrays, and paramtemps at exit
d15362a  fix(65816): string constification + unsigned promotion detection
03842ce  fix(qbe): pass const-qualified data sections to QBE backend
d929b94  fix(w65816): fix pointer and type handling for QBE IL generation
```

`mktype()` UB (ea95cac) was discovered after a build silently produced a struct
in ROM instead of WRAM — that's the kind of regression a careless submodule
bump would re-introduce.

The chantier-A1 patch (7f26c16) reduces `sizeof(int)` from 4 to 2 and
`sizeof(long)` from 8 to 4. The pointer size deliberately stays at 8 (its
own structural defect is tracked as A6 in the structural-defects catalogue;
reducing pointer storage cascades through QBE w65816's indirect-call emit
pass). Empirically validated against the full quick test suite.

### compiler/qbe — 36 patches (the bulk of the SDK's compiler magic)

Selected highlights (full list via `git -C compiler/qbe log HEAD --not upstream/master --oneline`):

```
2d3af4d  feat(w65816): chantier A6.8 — large-frame indirect addressing + Kl slot widening
9878b9f  fix(build): apply chantier A2 hygiene fixes to clean compile
90b81e1  fix(w65816): respect volatile loads/stores via `volat` IR keyword  (chantier A2)
d9483ee  fix(w65816): restrict leaf optimization to actual leaf functions
b064fbd  fix: eliminate all Clang warnings in QBE w65816 backend
ed0c7ee  fix(w65816): alloc computes absolute stack address via TSA
64eabff  feat(w65816): emit __sdiv16/__smod16 for signed division and modulo
fd1bebb  fix(w65816): emit .ACCU 16 and .INDEX 16 for WLA-DX register size tracking
b45ebdc  feat(w65816): composite constant multiply and inline-mul dead store elimination
ded72c5  fix(w65816): fix variable shift stack offset after pha
bab0164  feat(w65816): lazy rep #$20 for pure tail call functions
ed840fb  feat(w65816): tail call optimization for frameless functions
b56fa3d  feat(w65816): direct page .b for tcc__ registers and div/mod return in A
ea06b2f  feat(w65816): INC/DEC optimization, A-cache survives pha, dead store args
```

These commits implement the cycle reductions documented in
`~/.claude/.../memory/compiler_optimizations.md` (Phases 1 through 7a, total
−22% vs PVSnesLib baseline). Lose them and benchmarks regress.

### compiler/wla-dx — 0 patches ahead of upstream master

The submodule HEAD is at `ffe59ca`, currently 12 commits behind
`upstream/master`. Patches relevant to OpenSNES (notably the .ACCU/.INDEX
warning at `ffe59ca`) were merged upstream but the assembler doesn't have a
post-PR tagged release yet — we pin to the merge commit until Vhelin tags
the next release. See `~/.claude/.../memory/project_wladx_release.md`.

## Updating a pin

When a real reason exists to bump a submodule (security fix, feature merge,
upstream resync), do this in **one** commit:

```sh
# 1. Move the submodule to the new SHA
cd compiler/qbe
git fetch
git checkout <new-sha>
cd ../..

# 2. Run the FULL test suite (not --quick) — this is the gate
cd tools/opensnes-emu
node test/run-all-tests.mjs --allow-known-bugs
# Must report 393/393. Investigate any new failure before continuing.

# 3. Update PINS.md: replace the SHA in the table above, document any new
#    local patches in the per-submodule list. Keep the rationale terse.

# 4. Stage both changes and commit together
cd /path/to/opensnes
git add compiler/qbe compiler/PINS.md
git commit -m "chore(submodule): bump qbe to <short-sha> (<reason>)"

# 5. Push and verify CI green. The verify-toolchain step runs before every
#    build — drift between submodule pointer and PINS.md fails the build.
```

## Drift check

```sh
make verify-toolchain
```

Reads the BEGIN/END block above, compares each entry against
`git submodule status`, and exits non-zero if any submodule's HEAD doesn't
match its pinned SHA. CI calls this before `make release` so a stale or
unauthorised submodule pointer can never produce a release artifact.

## Out of scope (intentionally)

- `docs/doxygen-awesome-css` — cosmetic, third-party, low-risk; not pinned.
- `tools/opensnes-emu` — pinned by submodule pointer alone; the emu repo's
  own test suite is the gate, not this file.
