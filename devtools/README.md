# devtools/ — contributor-only scripts (never shipped)

For people changing the SDK. Python 3.10+, standard library only, run as
`python3 devtools/<script>.py` — except `vram_layout/`, whose generator
needs `ortools` (its README says how; the lint that gates the committed
output is stdlib). The game developer never sees this directory
(`.claude/rules/two_audiences.md`).

No script of this directory is run by a user build or copied into the zip
(since 2026-10-06; `check_doc_drift.py` anchor 17 keeps it so). The five
post-link checks `make/common.mk` used to run from here are `opensnes-rom
check` since 2026-10-05 (same verdicts on the 99 built ROMs) — the Python
originals stay for the contributor gates that use them — and the 0.x-name
hint is `opensnes upgrade` (shell, `scripts/opensnes`).

Every file here is named by a `make` target, a workflow, or an example
README; a script that none of them names is an orphan and is deleted
(review of 2026-10-05, lot 2).

## Sentinels and lints (`make lint` and the Lint workflow)

| Script | What it refuses | Target |
|--------|-----------------|--------|
| `check_doc_drift.py` | the sixteen anchored doc/code claims (`.claude/rules/doc_consistency.md`) | `lint-docs` |
| `check_doc_render.py` | un-rendered Markdown in the generated Doxygen HTML | `doc-render` job |
| `check_asm_abi.py` | a `lda N,s` whose comment names a parameter at the wrong stack slot (`abi_lint.md`) | `lint-asm-abi` |
| `lint_asm.py` | a `rep`/`sep` without its `.ACCU`/`.INDEX` marker | `lint` |
| `lint_commits.py` | a commit subject outside Conventional Commits, or an attribution trailer | `lint-commits`, the git hooks |
| `check_cproc_widths.py` | a new hard-coded width class in cproc's emitter not listed in `cproc_width_sites.txt` | `lint-cproc-widths` |
| `check_vram_layout.py` | a misaligned VRAM base in an example, or a `vram_map.h` that drifted from its `vram.spec` | `lint-vram` |
| `check_corpus_fresh.py` | testing a corpus older than the lib outputs | `tests`, `lint` |
| `check_bank_reads.py` | a bank-blind C read of bank $01+ data (silent failure) — also on every user link | `lint` (selftest), `common.mk` |
| `check_nmi_wram_race.py` | the WRAM port (`$2180`) reached from an NMI callback (silent failure) — also on every user link | `common.mk` |
| `symmap/symmap.py` | WRAM overlaps, bank $00 overflow, RAM-band budget, the data-init terminator — on every user link and on the release ROMs | `common.mk`, the build workflows |
| `verify_toolchain.py` | a submodule HEAD that is not the one `compiler/PINS.md` pins | `verify-toolchain`, `compiler` |
| `link_modules.py` | a lib module that needs a symbol from a module it does not declare | `test-link-modules` |
| `toolchain_suites.py` | a regression or an XPASS in the upstream cproc / QBE / wla-dx suites (`toolchain-suites/*.txt` ratchets) | `test-toolchain-suites` |
| `gen_luna_doc.py` | `docs/tools/luna.md` that is not the pinned luna's own `--help` (`--check`) | `tests` |
| `check_debug_info.sh` | debug metadata leaking into a normal build, or `-g` emission broken | `opensnes_build.yml` |
| `release_smoke.py` | a release zip from which the starter or a scaffolded project does not build | `release-smoke`, the build and release workflows |

Their unit tests: `test_check_doc_drift.py`, `test_check_nmi_wram_race.py`,
`test_check_vram_layout.py`, `test_asset_budget.py`, `symmap/test_symmap.py`
(`devtools-tests` job of `lint.yml`; `make test-devtools`).

## Fixtures: moved to `testing/fixtures/`

The twenty ROM projects that assert on the library, the compiler and luna
(`libtests*`, the compiler runtime ROMs, the stress ROMs, `benchrom`) live
under [`testing/fixtures/`](../testing/fixtures/README.md) since 2026-10-05,
with one list in the root `Makefile`. `compiler-tests/` keeps the
compile-time pattern checks (`cases/`, `run.py`).

## Benches and reports

| Script | Measures | Target |
|--------|----------|--------|
| `cyclecount/cyclecount.py`, `cyclecount/bench.py` | static 65816 cycle counts of the compiler's output for 33 functions against `bench_baseline.json`; `docs/BENCHMARK.md` is anchored to it | `bench`, `functional-tests` job |
| `asset_budget.py` | static VRAM / CGRAM weight of an example's converted assets | `asset-budget`, `common.mk` (one line per link) |
| `vram_layout/` | `vram.spec` → `vram_map.h` by CP-SAT (ortools, opt-in; six examples use it) | by hand, gated by `lint-vram` |

## Data files

`cproc_width_sites.txt`, `toolchain-suites/*_known_fail.txt`,
`cyclecount/bench_baseline.json`. The names 1.0 removed live in
`make/removed_api.txt` (shipped: `opensnes upgrade` reads it, and so does the
sentinel's anchor 16).

## See also

- [`tools/README.md`](../tools/README.md) — the shipped tools and the
  harness.
- `.claude/rules/two_audiences.md`, `.claude/rules/luna_tooling.md`,
  `.claude/rules/doc_consistency.md`.
