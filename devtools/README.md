# devtools/ — contributor-only scripts (never shipped)

For people changing the SDK. Python 3.10+, standard library only, run as
`python3 devtools/<script>.py` — except `vram_layout/`, whose generator
needs `ortools` (its README says how; the lint that gates the committed
output is stdlib). The game developer never sees this directory
(`.claude/rules/two_audiences.md`).

Transitional exception: `make/common.mk` still runs five of these scripts
on a user's `make` (`symmap.py`, `check_bank_reads.py`,
`check_nmi_wram_race.py`, `asset_budget.py`, `check_upgrade.py`), so the
`Makefile` copies them into the zip (`RELEASE_DEVTOOLS`). They are to be
replaced by `opensnes-rom check`, in C; until then the list may only
shrink.

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
| `check_upgrade.py` | in a project's sources, the names 1.0 removed and the calls whose meaning changed (`removed_api.txt`) | `check-upgrade`, `common.mk` on a compile error |
| `link_modules.py` | a lib module that needs a symbol from a module it does not declare | `test-link-modules` |
| `toolchain_suites.py` | a regression or an XPASS in the upstream cproc / QBE / wla-dx suites (`toolchain-suites/*.txt` ratchets) | `test-toolchain-suites` |
| `gen_luna_doc.py` | `docs/tools/luna.md` that is not the pinned luna's own `--help` (`--check`) | `tests` |
| `check_debug_info.sh` | debug metadata leaking into a normal build, or `-g` emission broken | `opensnes_build.yml` |
| `release_smoke.py` | a release zip from which the starter or a scaffolded project does not build | `release-smoke`, the build and release workflows |

Their unit tests: `test_check_doc_drift.py`, `test_check_nmi_wram_race.py`,
`test_check_vram_layout.py`, `test_asset_budget.py`, `symmap/test_symmap.py`
(`devtools-tests` job of `lint.yml`; `make test-devtools`).

## Fixtures: ROMs that assert on the library and the compiler

Single-purpose ROM projects (a `main.c`, a `Makefile` over `common.mk`)
rebuilt clean by `make tests`; each asserts result globals by symbol in
luna, through `testing/lib` (`from lib import find_luna, assert_mem`).

| Directory | Covers | Asserted by |
|-----------|--------|-------------|
| `libtests/` | console, sprite, dma, background, text, math, anim, map, audio, fixed32, collision, window, input, object, colormath, mosaic, profile, scene, tile, math_ease | `test_libtest.py` |
| `libtests_fx/` | hdma, mode7, SNESMOD, nmiSet, an IRQ armed before the driver | `test_libtest_fx.py` |
| `libtests_dsp1/` | the DSP-1 commands no example calls, on luna's real firmware | `test_libtest_dsp1.py` |
| `libtests_hirom/` | the HiROM map | `test_libtest_hirom.py` |
| `libtests_sa1_sram/` | SA-1 BW-RAM across a power cycle | `test_libtest_sa1_sram.py` |
| `libtests_gsu/` | the Super FX job path | `testing/manifests/libtest_gsu*.toml` |
| `libtests_snesmod/` | the SNESMOD stop / pause / fade queue | `testing/manifests/libtest_snesmod.toml` |
| `compiler-tests/runtime/*` | a6_farptr, a7_32bit, b2_far_ram, c_features, debug_channel, d_quals — runtime proofs of compiler chantiers | each one's `test_*.py` (`test-lib`) |
| `compiler-tests/cases/` | C → ASM pattern checks, no emulator (`run.py`, `*.checks`) | `test-compiler` |
| `benchrom/`, `benchrom/b2_deref/` | cycles per call of the lib's ASM paths (C1 audit instrument, `lib/ARCHITECTURE.md`) and the far-deref cost (B2) | `bench.py` in each, by hand |

The fixtures are planned to gather under one root with a single list
(review, lot 6); today the `Makefile` lists them in three targets.

## Benches and reports

| Script | Measures | Target |
|--------|----------|--------|
| `cyclecount/cyclecount.py`, `cyclecount/bench.py` | static 65816 cycle counts of the compiler's output for 33 functions against `bench_baseline.json`; `docs/BENCHMARK.md` is anchored to it | `bench`, `functional-tests` job |
| `asset_budget.py` | static VRAM / CGRAM weight of an example's converted assets | `asset-budget`, `common.mk` (one line per link) |
| `vram_layout/` | `vram.spec` → `vram_map.h` by CP-SAT (ortools, opt-in; six examples use it) | by hand, gated by `lint-vram` |

## Asset generators kept for provenance

| Script | Produces | Named by |
|--------|----------|----------|
| `hicolor64.py` | per-tile-row palettes for the HiColor technique (`examples/color/hicolor_1792`) | the example's README |
| `m7ptables.py` | extracts and verifies the Mode 7 perspective tables (`examples/mode7/perspective_rotate`) | the example's README |

Studio needs in disguise: in 1.x they become `opensnes-image`.

## Data files

`removed_api.txt` (the 47 names 1.0 removed, read by `check_upgrade.py` and
the sentinel's anchor 16), `cproc_width_sites.txt`, `toolchain-suites/*_known_fail.txt`,
`cyclecount/bench_baseline.json`.

## See also

- [`tools/README.md`](../tools/README.md) — the shipped tools and the
  harness.
- `.claude/rules/two_audiences.md`, `.claude/rules/luna_tooling.md`,
  `.claude/rules/doc_consistency.md`.
