# OpenSNES — Gaps Review and Prioritised Backlog

**Kind of review:** a *gaps* review. Not a grade of what exists (see
`2026-05-10_sdk_review.md` for that) but an inventory of what is missing —
for the project and for an AI agent working in it — on three axes the owner
named: development information, SNES documentation (the Cartouche corpus),
and tooling (coverage, sanitizers, profiling, static analysis, fuzzing).

**Project under review:** OpenSNES SDK
**HEAD:** `0ffd06bf` (post-v0.41.1, luna pinned to v1.18.0 in
`tools/luna-test/luna.version`)
**Date:** 2026-09-11
**Dominant criterion:** reliability / correctness — catch more bugs earlier.
Cheap documentation fixes that stop an agent acting on a wrong fact count as
reliability. Adoption and velocity are secondary.
**Deliverable:** this document and its backlog (§10). No implementation ships
with it; each backlog item is a separate chantier, and luna capability
requests go through the owner-validation step of `.claude/rules/luna_tooling.md`
before any issue is filed.

---

## Table of contents

1. [Executive summary](#1-executive-summary)
2. [Methodology](#2-methodology)
3. [Process & CI](#3-process--ci)
4. [Host toolchain](#4-host-toolchain)
5. [Lib C](#5-lib-c)
6. [Compiler test surface](#6-compiler-test-surface)
7. [ROM-side (luna)](#7-rom-side-luna)
8. [Docs & agent knowledge](#8-docs--agent-knowledge)
9. [Cartouche corpus](#9-cartouche-corpus)
10. [Backlog](#10-backlog)
11. [Recommended ordering](#11-recommended-ordering)
12. [Verification of this review](#12-verification-of-this-review)

---

## 1. Executive summary

Five findings matter more than the rest.

1. **CI does not run the gate contributors are told to run.** Neither
   `make tests` nor `make lint` is invoked by any workflow; each workflow
   re-lists a hand-picked subset, and the lists have already diverged (three
   linters and two runtime ROM tests are local-only; the "functional probes"
   step is a no-op that prints `Probes: 0/0 passed`). The Makefile itself
   documents this failure mode at `Makefile:158-165` and it has recurred.
2. **The C ABI surface has large untested regions.** Struct pass-by-value
   and struct return have zero tests and zero uses; `switch` jump tables and
   function pointers have one compile-only fixture each, and the latter's
   header admits an unfixed QBE assertion crash. 56 of 76 compiler fixtures
   are compile-only. Two Class A silent-corruption bugs are documented only
   in `.claude/notes/tech/` and appear in neither `KNOWN_LIMITATIONS.md` nor
   `.claude/STRUCTURAL_DEFECTS.md`.
3. **The host toolchain runs with no sanitizer, no static analyser and no
   upstream test suite.** cproc and QBE are built with `CFLAGS="-g"` and no
   warning flags; the only `-fsanitize` in the repository is a Windows-only
   monthly cron. The asset tools that parse untrusted PNG / IT / TMX /
   Aseprite input have no malformed-input tests at all.
4. **luna v1.18.0 already ships most of the ROM-side capabilities the
   project lacks, unused.** `luna profile`, `luna diff`, `--power-on random`,
   `--force-region pal`, `--audio-out`, `--native-res` and `--call-stack` are
   never invoked by the harness. The CI "benchmark" is a static cycle
   estimate whose own docstring calls the luna cross-check "the planned
   upgrade".
5. **Auto-loaded project docs carry stale facts an agent will act on.**
   `CLAUDE.md:104-105` still states the pre-#127 SUPERFREE spill model and
   "all C RAM must be below $2000"; `ROADMAP.md:286` says the same; both are
   contradicted by chantier B2 (`FAR`, v0.39.0) and #127.3 (v0.41.0).
   `.claude/STRUCTURAL_DEFECTS.md` §5–§7 still recommends "Mesen2 mandatory"
   and A6 as the next chantier.

The backlog (§10) has 43 items. Tier 1 (§11) is eleven of them, all
small-to-medium effort with high reliability return:

| id | Tier 1 item |
|---|---|
| P1 | CI runs `make tests` and `make lint` verbatim |
| P3 | `symmap --check-overlap` becomes blocking |
| D1 | Correct the memory-model claims in `CLAUDE.md` / `ROADMAP.md` |
| C3 | Pin the two documented-only silent compiler bugs |
| L1 | Lib C gets the clang `-Wall -Wextra -Werror` pre-pass examples already get |
| H2 | Compiler and tools built with warnings on |
| R1 | Corpus liveness and visual hashes under `--power-on random=<seed>` |
| H3 | ASan/UBSan job on Linux |
| R3 | `luna diff` as the Class A validation protocol |
| H1 | Upstream cproc / QBE / wla-dx suites run on every PIN bump |
| D2 | `.claude/STRUCTURAL_DEFECTS.md` refresh |

What is healthy and should be left alone: 85/85 example READMEs plus a
template; 17 auto-loaded rules and 3 hooks; `docs/tutorials/debugging.md`
covers the luna MCP surface well; the Cartouche corpus holds 195 sources with
arbitration and covers the hardware side thoroughly; zero `TODO`/`FIXME`
markers in `lib/`, `templates/`, `make/`.

---

## 2. Methodology

What was done (2026-09-10/11, read-only):

- Three parallel explorations of the tree at HEAD `0ffd06bf`: (a) every
  test / lint / verification mechanism in `Makefile`, `make/common.mk`,
  `devtools/`, `tools/luna-test/`, the five workflows under
  `.github/workflows/`, and the build flags of `compiler/` and `tools/`;
  (b) `docs/`, `.claude/notes/`, `.claude/STRUCTURAL_DEFECTS.md`,
  `KNOWN_LIMITATIONS.md`, `ROADMAP.md`, `CONTRIBUTING.md`, and a repo-wide
  `TODO|FIXME|XXX|HACK` grep; (c) the public API of `lib/include/snes/*.h`
  (300 declarations, 36 headers) cross-referenced against every
  `examples/**/*.c` and harness ROM, the `LIB_MODULES` link counts over 85
  example Makefiles, the 76 compiler fixtures and 46 luna manifests.
- The luna v1.18.0 flag surface was read from
  `tools/luna-test/bin/luna <cmd> --help` for `run`, `state`, `diff`,
  `profile`, `test`, `frames`, `bench`. Every flag named in this review was
  checked against that output.
- The Cartouche corpus was probed with `snes_sources` and eight
  `snes_search` queries, each with
  `exclude_sources=["opensnes-docs", "opensnes-notes-tech"]` so the SDK's
  own docs could not answer for the corpus.

What was **not** done: no CI job was executed; no sanitizer, fuzzer or
static analyser was trialled on the tree (the proposals in §4 are designs,
not measurements); no luna feature request was filed.

This review makes **no hardware claim**. Every statement is about the
repository, its tooling, or the corpus index; the `hardware_claims.md` gate
is therefore satisfied vacuously.

Evidence convention: each gap carries an id (G = tooling, K = knowledge,
S = untested surface) and a path or a grep result. Every id appears exactly
once in the backlog of §10, except three intentional facet splits noted there.

---

## 3. Process & CI

**G1 — `make tests` and `make lint` are never run as units by CI.**
`grep -n 'make lint\|make tests' .github/workflows/*.yml` returns only
comments (`lint.yml:152`, `release.yml:176-178`, `opensnes_build.yml:417`).
`opensnes_build.yml`'s `functional-tests` job and `lint.yml`'s eight jobs
re-list steps by hand. `Makefile:158-165` documents the exact failure mode
("`make tests` could be green on a codegen change that CI then rejected")
from the last time the lists diverged; they have diverged again in the other
direction (G2–G5).

**G2 — the "functional probes" step is a no-op.** `tools/luna-test/probes/`
contains only `lib.py` and `run_all.py`, both in `run_all.py`'s `SKIP` set;
running it prints `Probes: 0/0 passed` and exits 0. The probes were migrated
to `tools/luna-test/manifests/*.toml` (native `luna test`; the first line of
`manifests/apu_switch.toml` says so). `Makefile:157`,
`.claude/rules/testing.md:14` and the workflow still describe the old state
as "functional probes (scripted input → WRAM asserts)".

**G3 — three linters in `make lint` never run in CI:** `check_asm_abi.py`
(the ABI-mismatch class that bit chantier A6+A7 `hdmaSetupBank`),
`check_lib_rodata.py`, `check_corpus_fresh.py`.
`grep -rn 'check_asm_abi\|check_lib_rodata\|check_corpus_fresh' .github/` is empty.

**G4 — tool golden tests: six defined, two in CI.** `Makefile:198-207`
(`test-tools`) lists gfx4snes, tmx2snes, smconv, wav2brr, palplan,
aseprite2snes; `lint.yml:179-180` runs gfx4snes and smconv only. font2snes
and img2snes have no `tests/run_golden.py` at all.

**G5 — two of four runtime compiler ROM tests are local-only.**
`b2_far_ram` and `debug_channel` (the only test of `--nocash-out` and the
`SNES_ASSERT` WDM channel) are in `make tests` but absent from every workflow.

**G6 — the example-validation step cannot fail.** "Validate examples" in
`opensnes_build.yml:233-256` pipes `symmap.py --check-overlap` through `tee`
and ends with a bare `true`; `release.yml` duplicates the pattern. A detected
bank $00 ↔ `$7E` WRAM-mirror overlap is printed and ignored.

**G7 — orphans.** `devtools/test_asset_budget.py` is referenced by no
Makefile target and no workflow. `devtools/benchrom/` is referenced only by
itself and carries committed build artefacts (`benchrom.sfc`, `.o`, `.asm`,
`linkfile`).

**G8 — harness README drifted from the harness.** `tools/luna-test/README.md:21`
says the pin is "v1.14.0 at time of writing" (it is v1.18.0); `:95` says
`wram_regress.py` is "Local, same-arch tool — not a CI gate" while
`opensnes_build.yml` runs it as a blocking step on both arches;
`release.yml:202` says "boots all 56 examples" (85).

**G25 — Doxygen warnings are not gated.** `docs/Doxyfile` has `WARNINGS = YES`
and `WARN_IF_UNDOCUMENTED = YES` but no `WARN_AS_ERROR`; neither `make docs`
nor `deploy_docs.yml` inspects the warning stream.

**G26 — no dependency pinning.** No `.github/dependabot.yml`; actions are
floating major tags (`actions/checkout@v5`, `msys2/setup-msys2@v2`,
`peaceiris/actions-gh-pages@v4`) in workflows holding `contents: write`.

**G27 — no pre-commit configuration and no `.editorconfig`.** The per-PR
lint story depends on contributors remembering `make lint`, which per G1 is
not what CI runs.

---

## 4. Host toolchain

**G9 — no submodule test suite is run.** `compiler/cproc/Makefile` defines
`check` (→ `./runtests`), `compiler/qbe/Makefile` defines `check` (→
`tools/test.sh all`), `compiler/wla-dx` ships `byte_tester/`.
`grep -rn 'runtests\|tools/test.sh\|make check' Makefile .github/ make/` is
empty. A PIN bump (`compiler/PINS.md`) is validated only by SHA equality
(`verify_toolchain.py`) plus downstream ROM behaviour.

**G10 — the compiler is built unoptimised and warning-blind.**
`compiler/Makefile:70` builds QBE with nothing but `CC=clang`;
`compiler/Makefile:83` builds cproc-qbe with `CFLAGS="-g"`. No `-O2`, no
`-Wall`, no `-Werror`. `tools/tmx2snes/Makefile:5` carries
`-Wno-implicit-function-declaration`. The compiler that produces every
shipped ROM is the least warning-hygienic C in the tree.

**G11 — no sanitizer on Linux or macOS, ever.** The single `-fsanitize` in
the repository is `msys2_cproc_diagnostic.yml:122-123` (UBSan on cproc-qbe),
Windows-only, on a monthly cron (`:21`). Nothing instruments `tools/*`.

**G12 — no static analyser.** Repo-wide grep for
`cppcheck|clang-tidy|scan-build|codeql|infer` (submodules and CHANGELOG
excluded) is empty; `.github/` has no CodeQL workflow.

**G13 — no fuzzing and no malformed-input tests for the asset tools.**
`grep -rniE 'malformed|corrupt|invalid|truncat|fuzz' tools/*/tests/*.py` is
empty; all golden tests feed known-good fixtures. `tools/gfx4snes/src/lodepng.c`
(6 300+ lines of third-party PNG decoding, vendored a second time in
`tools/img2snes/src/`), smconv's IT loader, tmx2snes's TMX reader and
aseprite2snes's JSON reader have no adversarial coverage.
`compiler/qbe/tools/abifuzz.sh` exists and is unused.

**G14 — `tools/valgrind-static.supp` is an orphan** (referenced only by
`tools/README.md`; flagged in the 2026-05-10 review, still there).

### Tool-class proposals

These are the concrete designs for the tools the owner named. Each says what
job, what command, what it gates.

**Sanitizers (H3).** One Linux job `sanitizers` in `lint.yml`. Build QBE,
cproc-qbe, wla-65816 / wla-spc700 / wlalink and every `tools/*` with
`CC=clang CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"`
and matching `LDFLAGS` — this must override the `-static` in
`compiler/Makefile` and the tool Makefiles, since ASan does not link static.
Run `python3 devtools/compiler-tests/run.py` (76 fixtures through
cproc → QBE → wla), `make lib`, `make test-tools`. Gate with
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`:
any report fails the job. Once green, retire the Windows-only monthly UBSan
in `msys2_cproc_diagnostic.yml`.

**Valgrind (H6).** ASan cannot instrument the shipped `-static` binaries,
and a static build is what `make release` produces; that is the one place
valgrind still adds value. One job in `release.yml` after `make release`:
the six `run_golden.py` scripts and the compiler fixtures under
`valgrind --error-exitcode=1 --suppressions=tools/valgrind-static.supp`.
Not per-PR (slow, redundant with ASan). If the owner does not want that job,
delete the suppression file.

**Static analysis (H4).** Pick **cppcheck**: it needs no compile database
(clang-tidy would need `bear` on a recursive-make tree), its findings gate a
PR immediately (CodeQL reports to the Security tab and does not block
without extra wiring), and its false-positive rate on C99 is low. Gating:
`cppcheck --std=c99 --enable=warning,performance,portability
--error-exitcode=1 --suppress=*:tools/gfx4snes/src/lodepng.c` over
`tools/*/src`. Advisory (`continue-on-error: true`) on `compiler/cproc` and
`compiler/qbe`, which carry upstream noise. `lib/source` joins the gating
set after L1. CodeQL can be added later as non-gating security scanning; it
is not the reliability tool here.

**Fuzzing (H5).** libFuzzer, since clang is already the compiler
everywhere. One 20-line `LLVMFuzzerTestOneInput` per parser, each calling
the existing entry point: `lodepng_decode32` (one target once H8 dedupes
lodepng), the IT loader in smconv, the TMX reader in tmx2snes, the JSON
reader in aseprite2snes, `stb_image` in font2snes. Seed corpus:
`tools/*/tests/fixtures/`. A nightly cron runs each target
`-max_total_time=600 -rss_limit_mb=2048` under ASan and uploads crash inputs
as artifacts. Gate: crash inputs found are committed under
`tools/<tool>/tests/crashes/` and replayed by the tool's `run_golden.py`
inside the H3 sanitizer job on every PR (seconds). `qbe/tools/abifuzz.sh`
does not apply as-is — it compares QBE's ABI against the *host* C compiler
through an OCaml call generator; for the 65816 target the executor would be
luna. Note it as a possible follow-up of C1, not a tier item.

**Coverage (H7 host).** Build QBE and cproc-qbe with
`-fprofile-instr-generate -fcoverage-mapping`, run the 76 fixtures and
`make lib`, then `llvm-profdata merge` and `llvm-cov report` restricted to
the patched files (the 65816 target files in QBE; `targ.c` / `qbe.c` in
cproc). Publish the report as an artifact first; ratchet once a baseline
exists. Answers "which patched code paths do the fixtures never reach".

**Lib C static pass (L1).** `make/common.mk:330` gives every example a
`clang -fsyntax-only -Wall -Wextra -Werror` pre-pass; `lib/Makefile:46` is
`CFLAGS = -I$(INCDIR)` and gets none. Copy the rule; then add `lib/source`
to the cppcheck gating set.

---

## 5. Lib C

**G15 — `lib/source/*.c` gets no static analysis at all**, not even the
clang pre-pass every example gets (`lib/Makefile:46`; `grep -c clang
lib/Makefile` → 0). The 24 library `.c` files are compiled only by cproc,
which `make/common.mk` itself describes as having no `-W` flags.

**G16 — library unit tests cover five areas.** `devtools/libtests/` holds
≈ 30 WRAM assertions: div16/mod16 (including /0 and 65535/1), mul16, sqrt16,
signed-division cast, NMI-context mul/div/mod, the anim state machine, map
tile/prop, text cursor wrap, audio v2. The rest of 24 `.c` / 21 `.asm`
modules have no direct assertion.

**96 of 300 public declarations are called by zero examples and zero
harness ROMs.** By header:

| header | never-called functions |
|---|---|
| `collision.h` | `collidePoint`, `collideRectEx`, `collideRectTile`, `collideTileEx`, `rectContains`, `rectGetCenter`, `rectInit`, `rectSetPos` — 8 of 10; the two examples linking `collision` use none of them |
| `window.h` | `windowInit`, `windowCentered`, `windowSplit`, `windowSetPos`, `windowSetSubMask`, `windowDisable`, `windowDisableAll` |
| `interrupt.h` | `irqSetVTimer`, `irqDisable`, `irqClear` — the entire IRQ path |
| `input.h` | `mouseIsConnected`, `padIsConnected`, `padRaw`, `scopeButtonsHeld`, `scopeGetRawX/Y`, `scopeSetRepeatDelay`, `scopeSinceShot` |
| `dsp1.h` | `dsp1Multiply`, `dsp1Rotate`, `dsp1Target`, `dsp1Distance`, `dsp1Range` — 5 of 13 |
| `colormath.h` | `colorMathSetBrightness`, `colorMathSetChannel`, `colorMathSetCondition`, `colorMathSetFixedColor`, `colorMathTransparency50` |
| `console.h` | `consoleInitEx`, `getRegion`, `isPAL`, `isInVBlank`, `resetFrameCount` |
| `background.h` | `bgInit`, `bgInitTileSetData`, `bgGetScrollX/Y`, `bgSetScrollY` |
| `math.h` | `fixAbs`, `fixClamp`, `fixDiv`, `fixLerp`, `fixSqrt` |
| `mode7.h` | `mode7Rotate`, `mode7SetMatrix`, `mode7SetPivot`, `mode7Transform` |
| `dma.h` | `dmaClearVRAM`, `dmaCopyCGramBank`, `dmaCopyOam`, `dmaTransfer` |
| `audio.h` | `audioStopAll`, `audioStopVoice`, `audioUnloadSample`, `audioUpdate` |
| `snesmod.h` | `snesmodAllocateSoundRegion`, `snesmodFlush`, `snesmodGetPosition`, `snesmodSetSoundTable` |
| `sprite.h` | `oamDrawMetaFlip`, `oamDynamicSetSize`, `oamSetTile`, `oamSetX`, `oamSetY` |
| `sram.h` | `sramSave`, `sramLoad`, `sramChecksum` — `save_game` uses only the `*Offset` variants |
| `hdma.h` | `hdmaGetEnabled`, `hdmaGradient`, `hdmaWaveInit` |
| `video.h` | `videoSetObjInterlace`, `videoSetOverscan`, `videoSetPseudoHires` |
| `profile.h` | all 8 (`profileInit`, `profileColorStart/End`, `profileScanlineStart/End`, `profileGetScanline`, `profileGetFrameCount`, `profileGetLagFrames`) |
| others | `gfxLoad` (asset), `consoleMesenBreakpoint` (debug), `fix32Div` (fixed32), `mapSetMapOptions` (map), `mosaicGetSize/SetSize` (mosaic), `textGetX` (text) |

Two modules are **linked by zero builds**: `profile` and `math_ease`
(`ease_quad_table`, `ease_in_quad`, `ease_out_quad`). Nothing in the
corpus has assembled them into a ROM; whether they still link is unknown.
Five more are linked by exactly one example (`lzss`, `mosaic`, `scene`,
`sprite_dynamic_meta`, `sram`).

Nineteen functions are covered only by `devtools/libtests/main.c` or the
runtime ROMs, with no example (`animRestart`, `audioGetFreeMemory`,
`audioGetSampleInfo`, `audioGetVoiceState`, `audioGetVolume`, `audioIsReady`,
`audioSetGain`, `audioSetVoicePitch`, `audioSetVoiceVolume`, `bgSetScrollX`,
`consoleNocashMessage`, `dmaCopyVramBank`, `nmiClear`, `div16`, `mod16`,
`mul16`, `textFlush`, `textGetY`, `textPutChar`).

**S6 — arithmetic helpers with no runtime oracle.** `tcc_smod16`
(`lib/source/runtime.asm`) is the only 16-bit runtime helper without a
libtest assertion; `fixMul` and `fixLerp` (`lib/source/math.asm`) have none
and are the functions `KNOWN_LIMITATIONS.md:217` flags as unsafe in
`nmiSet()` callbacks with an "observed-not-explained" mechanism;
`fix32Div` (`lib/source/fixed32.asm`) is called by nothing.

**S7 — no `memcpy` / `memset` / `strlen` in the library.** `games/breakout`
and `games/tetris` each roll their own loops.

crt0: `ScanMPlay5` (`templates/crt0.asm`, multitap ports 3–5, ≈ 90 lines
running in NMI every frame for MP5 users) has zero examples and zero probes;
`ReadMouse`'s sensitivity-cycling path is untested.

---

## 6. Compiler test surface

`devtools/compiler-tests/`: 76 fixtures in `cases/`, 16 with a `.checks`
file (asserted codegen), 56 compile-only — a crash or empty output fails,
codegen is unchecked — ratcheted by `MAX_UNCHECKED = 56` at `run.py:48`.
Four runtime ROMs under `runtime/` (`a6_farptr`, `a7_32bit`, `b2_far_ram`,
`debug_channel`) assert through luna. All four `KNOWN_FAIL` sets are empty.

C features with **no test at all** (grep over every fixture and runtime main):

| id | feature | status |
|---|---|---|
| S1 | struct passed by value, struct returned by value | 0 tests, 0 uses anywhere in `lib/` or `examples/` — on a left-to-right-push, non-standard calling convention, whatever the compiler emits today is unvalidated |
| S2 | `switch` / jump table | one compile-only fixture (`test_switch.c`) whose own header says sparse values "may use if-else chain instead"; no `.checks`, no runtime assert |
| S3 | function pointers / callback tables | one compile-only fixture; `cases/test_function_ptr.c:6-8` says "Complex function pointer arrays trigger a QBE bug (assertion failure) … TODO: Fix QBE bug and expand this test" — an acknowledged unfixed crash with no regression test |
| S4 | bitfields, `enum`, `goto`, recursion, varargs, inline asm | 0 tests each |
| S5 | 32-bit arithmetic | `test_u32_arithmetic.c` contains no `*`, `/`, `%`; 32-bit mul/div/mod are covered only by the `a7_32bit` ROM; variable-count 32-bit shifts and static 32-bit compare checks are uncovered |

> **Erratum (2026-09-11, item C3):** both bugs below were already fixed when
> this review was written — the ternary bank drop on 2026-08-13 (pinned by
> `cases/ternary_addr_const.checks` and the `ph2` cell of `a6_farptr`), the
> `Kl` shift by chantier A7 (v0.21.2). The notes' status lines were stale,
> which is itself the K9 finding: an agent reading `.claude/notes/tech/`
> saw "OPEN" for closed bugs. The function-pointer-array crash also no
> longer reproduces. C3 therefore closed the notes and added regression
> coverage instead of KNOWN_LIMITATIONS entries.

**K9 — two live silent-corruption bugs are documented only in
`.claude/notes/tech/`.** `ternary_addr_const_bank_drop.md` ("Status: OPEN
(unfixed)", Class A: a ternary yielding an address constant materialises
only the low 16 bits of the far pointer — found 2026-08-12 in
`sprites/aseprite_pipeline`) and `cc65816_kl_shift_high_half.md` (a `Kl`
shift-by-constant reads an unstored stack slot; `fix32Sin` was rewritten in
asm to route around it). Neither is in `KNOWN_LIMITATIONS.md`; neither is in
`.claude/STRUCTURAL_DEFECTS.md`; `KNOWN_LIMITATIONS.md:404` says of the
related `long`-return gap that it is "not yet tracked in
`STRUCTURAL_DEFECTS.md` because no shipping code triggers it". The public
limitations list under-reports the compiler.

`allow-known-bugs` survives only in prose (`ROADMAP.md:108-110`,
`KNOWN_LIMITATIONS.md:297-302`), correctly described as removed; no flag
implements it.

---

## 7. ROM-side (luna)

The harness invokes `run`, `state`, `wram-trace` and (in one prototype)
`mcp`. Per `.claude/rules/luna_tooling.md`, each gap below is classified
**wiring** (luna v1.18.0 already has the capability; the harness does not
use it) or **request** (missing; needs owner validation, then a luna issue
specified by a transitory prototype).

| id | gap | luna v1.18.0 | class |
|---|---|---|---|
| G17 | `luna profile` (real master cycles per `.sym` symbol, `--from-frame` / `--until-frame`, `--out` JSON) never invoked. The CI "bench" is `devtools/cyclecount/bench.py`, a *static* estimate whose docstring calls the luna cross-check "the planned upgrade" | present | wiring |
| G18 | `luna diff` (two-ROM frame-by-frame MATCH/DIFF, `--frames`, `--tolerance` for boot-length shift, `--input`) never used. A codegen change is validated by re-baselining fbhash and WRAM streams, not by A/B-diffing old vs new ROM at equal frame | present | wiring |
| G19 | `--power-on random[=seed]` (on `run`, `state`, `frames`, `diff`, `profile`) never used. Every run boots from zero-filled RAM. This is the bug class of the v0.40.0 and v0.41.1 boot fixes, which luna could not see | present on the CLI **and** as the manifest keys `power_on` / `seed` (erratum 2026-09-12; `luna test --help` does not list manifest keys) | wiring |
| G20 | `--force-region pal` never used; `isPAL` / `getRegion` never executed; 312-line timing never seen. All 85 baselines are NTSC | present | wiring |
| G21 | `--audio-out <wav>` never used. Audio is asserted as APU liveness and DSP voice flags only; a wrong-sounding but running driver passes | present | wiring (hash the WAV only — the earlier `audio_analyze.py` prototype was dropped as unreliable) |
| G22 | No ROM code coverage: nothing maps executed PCs to lib symbols or C lines. The 96 never-called declarations of §5 are a static estimate of the same thing | symbol level derivable from `profile --out` (symbols with cycles > 0); PC-set / C-line level absent | wiring (symbol) + request (PC set) |
| G23 | No VBlank *time* budget. Manifests assert `max_vblank_bytes = 4096` (`manifests/dma_*.toml`) but no cycle or scanline headroom; `lib/source/profile.asm`'s scanline latch is read by no test | per-frame max cycles of a symbol absent | request (prototype: peek the `profile.asm` latch each frame) |
| G24 | `luna frames` (consecutive-frame capture), `luna bench` (corpus anomaly sweep), `--native-res` (modes 5/6 are hashed at 256×224 only), `state --call-stack` unused | present | wiring |

> **First run of R1 (2026-09-12, luna v1.18.0, `--power-on random=1`):**
> liveness holds for all 85 examples, but the visual pass finds **nine
> examples whose rendering depends on the power-on RAM state**. Six audio
> examples (`audio/apu_switch`, `audio/echo`, `audio/pitch_mod`,
> `audio/play_noise`, `audio/soundboard`, `audio/speech_synth`) never write
> the tilemap/tiles they display — from zero-filled VRAM the screen is the
> backdrop colour, from random VRAM it is garbage tiles, which is what real
> hardware shows. Three examples (`backgrounds/mode0`, `color/hicolor_blend`,
> `games/tetris`) differ on the last scanline only (y = 223, 191–256 px),
> identically across three unrelated ROMs — systematic, to investigate
> (SDK-side init of the last row, or luna-side) before blaming either.
> Tracked as backlog item R10 below; the random pass gates liveness only
> until these are resolved.

Open luna observations parked in notes, to consolidate with the owner:
luna #207, #210, #211, #212 OPEN in `status/luna_stress_campaign.md`; the
observation there that `--input` is ignored when a run ends with
`--until-frame` (pending owner validation — it touches the in-flight
PPU-frame capture chantier on `wip/luna-frame-capture`);
`status/luna_input_replay_bug.md` still describes luna #126 as open (closed
in v1.13.0).

---

## 8. Docs & agent knowledge

Ranked by how often an agent or contributor acts on a wrong fact.

**K5 — stale memory-model claims in auto-loaded docs.** `CLAUDE.md:104`
describes the pre-#127.3 model ("`static const` arrays get SUPERFREE
sections … data silently spills to bank $01+ but C code reads from bank
$00"); `CLAUDE.md:105` says "all C RAM must be below $2000";
`ROADMAP.md:286` says "All C variables must be in bank $00, < $2000" under a
footer dated after chantier B2 shipped. `KNOWN_LIMITATIONS.md` and
`.claude/rules/bank0_budget.md` already carry the correct model (`FAR` to
`$7E:2000-$FFFF`; C const data in the asset banks by default). An agent that
loads `CLAUDE.md` refuses or rewrites valid `FAR` code.

**K9** — see §6.

**K6 — `.claude/STRUCTURAL_DEFECTS.md` §5–§7 are pre-A6 and pre-luna.**
§6's three paths and the "Recommended path (default, revised 2026-05-08)"
lead with "D1 path a (Mesen2 mandatory)" (Mesen2 retired 2026-07-05) and
A6 (resolved v0.19.0); §5's tier tables and §7's acceptance table are the
same vintage. D2 is 🟡 "unsourced assumption" while `KNOWN_LIMITATIONS.md`
marks the SIWP polarity 🟢 resolved with four sources; B4's text cites
`examples/graphics/effects/hdma_helpers`, deleted in the 2026-07-31 reorg.
Still genuinely open in the ledger: A5 (fork divergence, ongoing), A8
(MSYS2 segfaults, under telemetry), B3 (`mode7LoadGraphics` for non-bank-$00
data), B4 (partial), C2 (sprite/text C+ASM duplication).

**K1 — no luna CLI/MCP reference in the repository, and cited versions
disagree.** `luna.version` is v1.18.0; `tools/luna-test/README.md:21` says
v1.14.0; `docs/tutorials/debugging.md` anchors on "since luna v1.6.0";
`status/luna_stress_campaign.md` works against v1.17.0. Every session
rediscovers flag semantics from `--help`.

**K7 — the Cartouche corpus is undocumented in-repo.**
`.claude/rules/hardware_claims.md` mandates `snes_search` with
`exclude_sources=["opensnes-docs","opensnes-notes-tech"]` for every hardware
claim, but nothing lists the corpus contents, and the audit's "golden
queries" live in an external artifact
(`.claude/notes/chantiers/hardware_docs_audit.md:120`).

**K8 — five chantiers are open on `wip/*` branches with no landing and no
mention in `ROADMAP.md`:** `hicolor`, `hicolor-hires`, `interlace`,
`mode7-perspective` (validation TODO at `chantiers/mode7_perspective_port.md:96`),
`spc700`. A contributor can duplicate weeks of work.

**K4 — no "how to profile" document.** `lib/include/snes/profile.h`
(`profileInit`, `profileColorStart/End`, frame/lag counters) is documented
only in its Doxygen block; `devtools/cyclecount/` and `devtools/benchrom/`
have no user-facing guide; `docs/craft/frame-budget.md` teaches the concept
with no bridge to the tooling; `docs/BENCHMARK.md` is a results report.

**K2 — `docs/README.md` is a stale index.** It lists 9 of 23 tutorials and
omits `docs/craft/`, `docs/tools/`, `API_INDEX.md`, `LEARNING_PATH.md`,
`EXAMPLES_BY_CATEGORY.md`.

**K3 — no header → tutorial map; 12+ headers have no tutorial.** Most
painfully `text.h` (in nearly every first program), then `profile.h`,
`asset.h`, `object.h`, `gameloop.h`, `apu.h` (the raw-APU path), `lzss.h`,
`console.h`, `system.h`, `debug.h`, `interrupt.h`, `fixed32.h`.
`API_INDEX.md` is task-indexed and never names a header.

**K10 — two v1.0 must-haves do not exist:** the PVSnesLib → OpenSNES
migration guide and the FAQ (`ROADMAP.md`, v1.0 must-have list). The only
substitute is `.claude/rules/porting.md` plus
`.claude/notes/patterns/pvsneslib_porting.md`, both maintainer-internal.

Open items scattered across notes, to close or advance:
`status/clock_skew_incremental_builds.md` (root cause of future mtimes never
chased; a clean rebuild on 2026-09-10 still printed 12 clock-skew warnings);
`chantiers/function_inlining_audit.md` Phase 2 pending;
`chantiers/a1_followup_long_is_kw.md` not bisected;
`chantiers/b2_far_ram.md` §10b/§10f follow-ups;
`chantiers/hardware_docs_audit.md` F1 doc rewrite pending;
`tech/wla_span_upstream_report.md` never sent;
`tech/816_opt_analysis.md:45` TODO on a wla-dx branch-optimisation pass;
`devtools/compiler-tests/README.md:38` — the ~50 fixtures still without
`.checks`.

Structural: `lodepng.c/h` and `cmdparser.h` are vendored twice
(`tools/gfx4snes/src/`, `tools/img2snes/src/`); there is no `tools/common/`.
And a meta-gap: debt is invisible to `grep` — 0 `TODO`/`FIXME` in `lib/`,
`templates/`, `make/`; 42 of the 45 hits in `tools/` are upstream lodepng /
stb_image — so an agent grepping for markers concludes the project has no
open debt, when it lives entirely in `.claude/` prose.

---

## 9. Cartouche corpus

State on 2026-09-10 (`snes_sources`): 195 sources captured of 218 listed;
7 general arbiters (snesdev-wiki, fullsnes, anomie-regs, anomie-timing,
anomie-sdsp, anomie-spc700, undisbeliever-snesdev), 9 domain arbiters. The
hardware side is thorough and arbitrated; toolchain-side coverage includes
wla-dx docs and repo, snesdev-abi-v1, tcc-65816, pvsneslib, ittech-txt,
snesmod, and the test-ROM suites (blargg, gilyon, undisbeliever, sour,
absindx, higan).

Gaps by query (each with `exclude_sources=["opensnes-docs","opensnes-notes-tech"]`):

| query | result | missing source |
|---|---|---|
| QBE IL reference (data, phi, calls, aggregates) | no relevant hit | `compiler/qbe/doc/il.txt`, `abi.txt` |
| cproc internals (type sizes, decl parsing, struct emission) | no hit | `compiler/cproc/README.md`, `doc/` |
| luna CLI/MCP (`--until-frame`, mem-trace, profile, manifests) | reachable only via `opensnes-docs` (the debugging tutorial) | `k0b3n4irb/luna` README, docs, CHANGELOG |
| Tiled TMX / Aseprite JSON formats | only pvsneslib's tmx2snes copy | Tiled TMX format doc; Aseprite file spec |
| 65816 C compiler ABIs (Calypsi, WDC816CC, vbcc, ORCA/C, cc65) | named by snesdev-wiki, no content | the vendor manuals |
| profiling / CPU budget measurement | anomie-timing, fullsnes | nothing to add |
| test-ROM suites | snesdev-wiki, tasvideos | nothing to add |

### Missing sources (RAG1)

| source | why | suggested authority |
|---|---|---|
| QBE IL reference (`compiler/qbe/doc/il.txt`, `abi.txt`) | zero hits; every codegen fix starts from IL semantics | domain arbiter (compiler) |
| cproc docs (`compiler/cproc/README.md`, `doc/`) | zero hits; `type.c` / `decl.c` conventions govern every front-end patch | domain arbiter (compiler) |
| luna README + docs + CHANGELOG (`k0b3n4irb/luna`) | today reachable only through the OpenSNES tutorial; flag and manifest semantics are re-learned from `--help` each session | domain arbiter (tooling) |
| Tiled TMX format (doc.mapeditor.org) | only pvsneslib's copy; gid flip bits and layer encodings are the tmx2snes correctness spec | reference |
| Aseprite file spec (`aseprite/docs/ase-file-specs.md`) | no source; the only authority on cel / palette semantics behind the JSON export | reference |
| Calypsi 65816 manual, WDC816CC manual, vbcc 65816 backend doc | named, no content; the C1 (struct by value) and 32-bit-return decisions need what peer compilers do | secondary (comparative) |
| `snes-sdk-hecht` (already "to-capture") | the other cc-based SNES SDK; the closest crt0 / runtime prior art | secondary |
| `llvm-mos` (already "watch") | 6502-family LLVM backend; relevant to a future "replace QBE" discussion only | watch |

### Golden queries after ingestion

Each must return the new source with the exclusion set applied:

1. In QBE IL, how are aggregate types declared and passed to a call?
2. Which QBE ABI rules apply to a target with no register-passed arguments?
3. How does cproc lower a struct assignment; where is struct layout computed?
4. What does `luna --power-on random=<seed>` fill, and how is the seed reported?
5. What is the `luna test` manifest schema; which assert keys exist?
6. How does `luna diff --tolerance` match frame F of ROM A to ROM B?
7. In TMX, which gid bits encode horizontal / vertical / diagonal flip?
8. How does Calypsi return a 32-bit value and pass a struct by value?

---

## 10. Backlog

Layer: PROC / HOST / LIB / CT (compiler tests) / ROM / DOCS / RAG.
Effort: S ≤ 1 day, M ≤ 1 week, L > 1 week. Rel = reliability impact.
Every G/K/S id appears once, with three declared facet splits: G2 across P1
(the CI step) and P2 (the dead runner); S3 across C2 (tests) and C3 (pinning
the crash); K7 across D6 (the in-repo corpus note) and RAG1 (the corpus side).

### Process / CI

| id | title | gaps | eff | rel | first step |
|---|---|---|---|---|---|
| P1 | CI runs `make tests` and `make lint` verbatim on one Linux job | G1 G2 G3 G5 | M | **H** | **done 2026-09-11** (`8af66f53`: `functional-tests` runs `make tests`, `lint.yml` runs `make lint`, verbatim) — replace the hand lists in `opensnes_build.yml` (`functional-tests` job) and `lint.yml` with the two targets; the platform matrix keeps `make release` only |
| P2 | Delete the dead Python probe runner, refresh harness docs | G2 G8 | S | M | **done 2026-09-14** — `probes/run_all.py` deleted (it globbed an empty directory since the manifests migration); `lib.py` stays, it is the assert helper of every runtime ROM checker. `make tests`, CLAUDE.md and testing.md name `luna test` manifests instead; the harness README no longer calls the WRAM oracle "not a CI gate" (it gates `make tests` and CI since 2026-09-11) |

| P3 | `symmap --check-overlap` becomes blocking | G6 | S | **H** | **done 2026-09-11** (`a5280ef3`) — drop the trailing `true` in `opensnes_build.yml:243,256` and the `release.yml` copy; confirm the corpus is clean first |
| P4 | `make test-tools` in CI + goldens for font2snes / img2snes | G4 | M | M | **done 2026-09-14** — `tools/font2snes/tests` (3 cases: 2bpp, 4bpp, C header; synthetic 96-glyph sheet) and `tools/img2snes/tests` (2 cases: 16 colours, 4 colours rounded to BGR555; synthetic RGB gradient); `make test-tools` covers all eight tools and the CI job `tools-golden` runs that target instead of a hand-listed pair (six tools were CI-blind) |

| P5 | Wire or delete orphans | G7 | S | L | **done 2026-09-15 (verified, both halves already closed)** — `devtools/test_asset_budget.py` runs in the `devtools-tests` CI job since 2026-09-11; `devtools/benchrom/` keeps only sources under version control (`git ls-files` lists Makefile, main.c, data.asm, bench.py and the `b2_deref` variant — the `.sfc`/`.o` artefacts are gitignored) and `bench.py` is the C1-audit driver, kept on purpose |
| P6 | Supply-chain hygiene | G26 G27 | S | L | **done 2026-09-15** — every `uses:` in the eight workflows pinned to a 40-char commit SHA with the tag as a trailing comment (33 occurrences, 7 actions); `.github/dependabot.yml` (github-actions, weekly) keeps the pins moving; `.editorconfig` matches the CLAUDE.md style (4-space C, tabs for ASM and Makefiles). Submodules deliberately excluded from Dependabot: the three compiler forks move through `PINS.md` and the upstream suites |
| P7 | Doxygen warnings gate | G25 | S | L | **done 2026-09-15** — 56 warnings counted, then driven to 0 and the gate turned on (`WARN_AS_ERROR = FAIL_ON_WARNINGS`). Fixes at the source: `MARKDOWN_ID_STYLE = GITHUB` + `TOC_INCLUDE_HEADINGS = 6` so the `#heading` links written for GitHub resolve (39 of the 56); `@defgroup framework` and `@defgroup ui` for the two groups only `@ingroup` named; the seven `demo.gif` files renamed `<example>_demo.gif` (Doxygen matches images by basename); `KNOWN_LIMITATIONS.md` and `API_INDEX.md` added to the inputs, links to files outside them (CLAUDE.md, `.claude/rules/*`, ATTRIBUTION, ROADMAP, CHANGELOG, CODE_OF_CONDUCT, compiler/ABI.md) rewritten as GitHub URLs; twelve headings whose GitHub id collided with a page label renamed. the `doc-render` job (`make docs-strict`) now fails on a new warning. The gate is deliberately NOT in the Doxyfile: the first push proved why — `release` depends on `docs`, so a doc warning failed the Linux release build (the lib never finished under `-j`) and the Windows one (Doxygen resolves directory links differently there). Plain `make docs` stays non-fatal; the strict variant runs on Linux with the pinned Doxygen and the CSS submodule, which that job was missing |

### Host toolchain

| id | title | gaps | eff | rel | first step |
|---|---|---|---|---|---|
| H1 | Upstream suites run on every PIN bump | G9 | M | **H** | **done 2026-09-13** — `make test-toolchain-suites` (`devtools/toolchain_suites.py`, known-fail ratchets in `devtools/toolchain-suites/`), run at the end of the `sanitizers` CI job. First run: **cproc** 63/170 (the 107 are the 16-bit `int` and the rodata section line, 3 of them front-end errors on `sizeof(enum)` / bit-field width — listed, none a bug); **wla-dx** 31/32 (`base_test_1` asserts the `.BASE`-on-RAMSECTION semantics our one patch reverses on purpose — upstream's own test documents our divergence); **QBE** could not run at all: the fork had rewritten the shared `emitlnk`/`emitdat` for WLA-DX, so amd64/arm64/rv64 output was unassemblable since the fork's first commit. With upstream's ELF emitters restored under a target dispatch, 5 of 56 failed and all five bisected to the 2-pass architecture (eaf6116): `parse()` freed the type table at EOF while abi1/isel/emit of the collected functions ran afterwards — a use-after-free on every target but w65816, which never reads `typ` at pass 2. Fixed (`freetyps()` after `emit_collected`), 56/56, corpus byte-identical. The fork's true upstream base is QBE `120f316` (2025-05-30), found by blob matching; at that base the suite is 56/56 on the same host, which is what made the five failures attributable |
| H2 | Compiler and tools built with warnings on | G10 | S | **H** | **done 2026-09-11** (`83461a74`: `-O2 -Wall -Wextra`, `-Werror` on Linux CI via `TOOLCHAIN_WERROR=1`) — dry run `-O2 -Wall -Wextra` on cproc / QBE (`compiler/Makefile:70,83`); drop `-Wno-implicit-function-declaration` from `tools/tmx2snes/Makefile:5`; `-Werror` on Linux CI only |
| H3 | ASan/UBSan job on Linux | G11 | M | **H** | **done 2026-09-12** — `make test-sanitizers` (`SANITIZE=1` knob in `compiler/Makefile` and the nine `tools/*/Makefile`), CI job `sanitizers` in `lint.yml`; the first run found **seven bugs in five programs**: wla-65816 read one byte before its token buffer on every one-character label inside a macro (`-:` in snesmod.asm), wlalink `READ_T` shifted a byte >= 128 into the sign bit, QBE `memset` on a NULL temporary table, smconv stored `int` through `(int *)` casts of two `u16` fields (little-endian-only by accident) and shifted negative samples in the BRR encoder, wav2brr shifted a negative 8-bit sample, tmx2snes computed `offsetof` through a null pointer. All behaviour-neutral (byte-identical corpus). The CI runs on ubuntu-24.04's clang 18 then added two more that clang 22 and gcc 16 no longer report: cproc formed NULL + 0 over empty growable arrays (`arrayforeach`, `emitfunc`, `arrayadd`) and tmx2snes shifted `0xFF << 24` as an int — nine findings in six programs; the CI compiler is reproduced locally with `podman run ubuntu:24.04` when a report does not show on the host. The Windows-only UBSan step is retired. §4 Sanitizers was the design |
| H4 | Static analysis with cppcheck | G12 | S | M | **done 2026-09-14** — `make lint-cppcheck` (in `make lint`, cppcheck installed in the CI lint job): tools' sources and lib C gate, lodepng / stb_image suppressed, w65816 advisory. First run: a dangling context pointer in cmdparser (`ctx = &default_ctx` from a block-local, both vendored copies), an uninitialised `ticks[0]` for a frameless tag in aseprite2snes, gfx4snes's free-then-`fatal` paths read as double frees until `fatal` was declared noreturn (also gives it printf format checking) |

| H5 | Fuzzing the asset parsers | G13 | M (lodepng + IT) / L (all) | M | **complete 2026-09-15** — the remaining three parsers joined `tools/fuzz`: cute_tiled (tmx2snes's `.tmj` loader), the aseprite2snes JSON parser and font2snes's stb_image. Two real bugs on the first session: two **heap-buffer-overflows in cute_tiled** (the whitespace skip and the `expect` error message both read past the end of the caller's buffer, which the parser neither copies nor NUL-terminates while peeking characters unguarded and letting `STRTOULL`/`STRTOD` scan past the end) — fixed structurally by parsing an internal NUL-terminated copy plus bounding the one loop a terminator cannot stop and **undefined behaviour in stb_image** (an empty IDAT chunk skips the buffer allocation, so the accumulator computes `NULL + 0` — undefined behaviour clang 18 reports and clang 22 does not, which is why it took a container run to reproduce). Both fixed in the vendored copies with the crashing input committed under `tools/fuzz/crashes/`; `tmx2snes` now prints a diagnostic instead of trapping on a malformed map (cute_tiled's `CUTE_TILED_CRASH` made overridable). A third finding was the harness's own: jumping out of that abort path skips the library's cleanup, so malformed inputs leaked until libFuzzer called it an out-of-memory — the tiled harness now tracks the parser's allocations and releases them after a jump. lodepng and the IT loader shipped 2026-09-14 |

| H6 | Valgrind only on static release binaries, else delete the supp | G14 | S | L | §4 Valgrind |
| H7 | Host coverage report (llvm-cov) | owner request | M | M | **done 2026-09-15** — `COVERAGE=1` in `compiler/Makefile` builds QBE and cproc-qbe with clang source-based coverage; `make coverage-host` runs the compiler fixtures and `make lib` through them and emits three artefacts in `/tmp/opensnes_coverage`: the whole-tree report, a fork-only report (the w65816 backend and cproc's target side, upstream files filtered out) and an lcov export. New `host-coverage` job in `lint.yml` uploads them. A report, not a gate — the ratchet comes once the number has a history. First measurement (verified end-to-end in an ubuntu-24.04 container, clang 18 as in CI): **72.8 % of lines over the fork-owned files**, 55.8 % over the whole tree. `qbe/w65816/emit.c` 84.1 % and `inline.c` 82.3 % (the fixtures do exercise the backend); the thin spots are `cproc/attr.c` 14.8 %, `cproc/utf.c` 18.9 % and `qbe/emit.c` 44.7 % — attributes, UTF-8 decoding and the generic ELF emitters, none of which the SNES pipeline uses much |
| H8 | `tools/common/` for lodepng + cmdparser | open note | S | M | **done 2026-09-15** — the four files were byte-identical in both tools; now `tools/common/{lodepng,cmdparser}.{c,h}` with a README, compiled into each tool's `build/` via a `common_%.o` rule (`-I../common`); `tools/fuzz` and the cppcheck suppression point at the shared copy; goldens and fuzz replay unchanged |

### Lib C

| id | title | gaps | eff | rel | first step |
|---|---|---|---|---|---|
| L1 | Lib gets the clang `-Wall -Wextra -Werror` pre-pass examples already get | G15 | S | **H** | **done 2026-09-11** (`75296a1c`) — copy the `make/common.mk:330` rule into `lib/Makefile`; fix what it finds |
| L2a | "Every module links" smoke build | `profile`, `math_ease` linked by zero build | S | M | **done 2026-09-13** — `devtools/link_modules.py` (`make test-link-modules`, in `make tests`): every module alone with its declared deps, plus two all-together groups (hdma's ~2.8 KB and the dynamic-sprite family's ~2.9 KB of plain RAM cannot share one 8 KB band, so `all_but_hdma` and `all_but_dynamic`). **First run: 7 of 41 modules did not link alone** — background→dma, text→console, fixed32→math, object→sprite+sprite_dynamic, sprite_dynamic→sprite+sprite_lut, and the dispatch/meta halves→sprite_dynamic — every one an undeclared `_DEP_` that only worked because every example also lists console/sprite/dma; all declared now. The group builds then found `fixed32.asm`'s macro operands (`lda \1 + 1`) sized as direct page by WLA: fine while the f32 RAMSECTION landed below $0100 (every example), a link error at $19B7 — now `.w`. Also explains the "71 bytes free" seen on 14 ROMs: the linker packs bank $00 first and spills SUPERFREE code to bank 1, so a full bank $00 is the normal end state post-#127.3 and the "nearly full" warning no longer signals imminent failure (candidate follow-up: retire or re-aim it) |

| L2b | Runtime asserts for `tcc_smod16`, `fixMul`, `fixLerp`, `fix32Div` | S6 | S | **H** | **done 2026-09-14** — 19 vectors in `devtools/libtests` for fixMul / fixDiv / fixLerp (8.8) and fix32Mul / fix32Div (16.16): signs, fractions, t = 0/128/255, zero divisor, 1/3 truncation; tcc_smod16 is covered by the c_features ROM. First run found the public `FIX(x)` macro undefined for a negative argument (`(x) << 8`, rejected by the clang pre-pass — the math tutorial itself writes `FIX(-3)`): now an unsigned shift, same bits |

| L2c | Asserts for collision / window / sram / interrupt / dsp1 / console | G16, the 96 uncalled | M | M | **done 2026-09-15 (dsp1 left out — `dsp1b.rom` is not in CI)** — 45 new vectors in `devtools/libtests` (99/99): every `collision.h` function, the `sram.h` round trip in bank $70 (`USE_SRAM=1`), the raw IRQ path (V-timer 10/10 frames, disabled 0, H+V 2, `irqClear` default handler), `isInVBlank`/`isPAL`/`getRegion`, and the window module asserted on luna's PPU register view (`windows`, `w12sel`…, `tmw`, `tsw`). First run found `sramClear` storing a byte ramp instead of zeros — fixed in `sram.asm` (KNOWN_LIMITATIONS 🟢) |
| L3 | `memcpy` / `memset` / `strlen` in lib | S7 | M | M | decide ASM vs C; cover in L2b; `check_asm_abi.py` covers the ASM variant |

### Compiler test surface

| id | title | gaps | eff | rel | first step |
|---|---|---|---|---|---|
| C1 | Struct by value: support-and-test or diagnose-and-reject | S1 | M | **H** | **done 2026-09-13** — diagnose-and-reject was already the behaviour (the w65816 emitter refuses `parc`/`argc`/`blit` rather than emit wrong code); the refusal now names the feature and the workaround, and `devtools/compiler-tests/cases/negative/` pins struct parameter, struct return, struct assignment, variadic functions and inline asm as clean refusals (`run.py` fails if one starts compiling). `KNOWN_LIMITATIONS.md` has the three 🟡 entries |
| C2 | Runtime feature ROM: switch, function pointers, bitfields, enum, goto, recursion, varargs, inline asm, 32-bit `* / %`, variable 32-bit shift | S2 S3 S4 S5 | M | **H** | **done 2026-09-13** — `devtools/compiler-tests/runtime/c_features/` (64 asserts, in `make tests`). **First run: 9 of 58 failed, five silent miscompilations**: bit-field reads (cproc padded the extraction to a 32-bit word), variable-count 32-bit shifts (emitter fell through to the 16-bit loop), signed `long` compares (cproc classed them `w`; the branch fusion did too), signed compares ignoring overflow (`cmp`/`bmi`, 16- and 32-bit), `if (long)` and s16→s32 sign extension testing the low word / a clobbered N flag. All fixed (cproc `funcbits` + `cmpnarrow`; emitter `emit_scmp_w`/`emit_scmp_high`, `@vsh` shift loops, `ora` of the high half on `jnz`, `cmp #$8000` sign test); `KNOWN_LIMITATIONS.md` 🟢 entry. varargs / inline asm are refusals (C1) |
| C3 | Pin the two documented-only silent bugs | K9 (S3 facet) | S | **H** | **done 2026-09-11** — erratum: both bugs were already fixed (see the box above); notes and the fixture marker updated — originally: `KNOWN_LIMITATIONS.md` 🔴 entries for the ternary bank drop and the `Kl` shift high half; `KNOWN_FAIL` fixtures for both plus the function-pointer-array crash, replacing the `test_function_ptr.c:8` TODO |
| C4 | Ratchet `MAX_UNCHECKED` down | 56 unchecked fixtures | S per batch | M | **batch 1 done 2026-09-15** — 14 fixtures got their `.checks` (acache_pha, cmp_dead_store, commutative_swap, dead_store_elimination, mul_dead_store, inline_mul, inline_boundaries, xba_shift, signed_division, stack_adjust, volatiles, switch, static_mutable, string_init); `MAX_UNCHECKED` 55 → 41. Next batches: the phi/ssa fixtures, the struct ones |

### ROM-side (luna) — wiring vs request

| id | title | gaps | eff | rel | luna status | first step |
|---|---|---|---|---|---|---|
| R1 | Corpus liveness + fbhash under `--power-on random=<seed>` | G19 | S | **H** | wiring only — the manifest key `power_on = "random"` + `seed` already exists in v1.18.0 (erratum 2026-09-12: the "request" half was wrong; found through the corpus, verified on the pinned binary) | **done 2026-09-12** (`7eece535`, then `62910c24` gates the random visual compare; it found the six audio examples and the VOFS quirk, see R10) — a runner option that passes `--power-on random=1` (fixed seed) through to luna for coverage and compare; the WRAM oracle stays excluded (it hashes the stack by design) |
| R2 | PAL pass | G20 | S | M | wiring (`--force-region pal`) | **done 2026-09-15** — `luna_runner.py --region pal` (`--force-region` threaded through run/state, report file untouched), `test_libtest.py --region pal` asserting `getRegion()==1` / `isPAL()==TRUE`; `make test-pal` runs both; weekly `.github/workflows/pal.yml` (Monday 04:00 UTC). First pass: 83 OK / 2 INPUT-DEP of 85 |
| R3 | `luna diff` as the Class A validation protocol | G18 | S | **H** | wiring (`diff --frames --tolerance --input`) | **done 2026-09-12** (`7eece535`: `tools/luna-test/diff_corpus.py`, Class A row in `testing.md`) — thin orchestrator `tools/luna-test/diff_corpus.py` (drives luna, computes nothing); the Class A row of `testing.md` cites it |
| R4 | `luna profile` replaces the static cyclecount; VBlank *time* budget | G17 G23 | M | M | **luna shipped it in v1.22.0 (2026-09-14, pinned)**: `per_frame {max, max_frame, mean, frames}` per row and `--budget SYMBOL=MCLK` (exit 1 over, exit 2 unknown symbol). Verified on the corpus. One step before the gate can gate: rows are per label and the NMI handler is five (`NmiHandler`, `NmiHandler@vblank_work`, …), so a budget on `NmiHandler` judges the entry block only; the per-frame max of a sum cannot be folded from the JSON. Asked luna to fold `@child` rows into their parent (`~/opensnes_reports/opensnes_report_luna_2026-09-14_v1.22.0_ack.md`). Wiring (orchestrator + manifest `nmi_budget`) follows that | cross-check `devtools/cyclecount/bench.py` against `luna profile devtools/benchrom/benchrom.sfc --from-frame 2 --out -` |
| R5 | ROM coverage: executed-symbol set, ratchet on never-executed lib symbols | G22, the 96 uncalled | M | **H** | **done 2026-09-12** — `tools/luna-test/rom_coverage.py` over `luna profile --pc-set` (v1.21.0), in `make tests`. Measured on the input-free boot/idle path: **133 of 301 public functions executed, 168 never** (`baselines/never_executed.txt`, ratchet may shrink, never grow; `ROM_COVERAGE.md` per header). Stricter than the static 96 because input-driven paths are not exercised — the manifests' scripted legs are the next increment | commit the never-executed list as the ratchet |
| R6 | Audio regression by WAV hash | G21 | S | M | wiring (`--audio-out`); hash only | **done 2026-09-15** — `tools/luna-test/audio_regress.py` hashes 300 frames of APU output for apu_switch, snesmod_music, pitch_mod, play_noise (`baselines/audio.json`; a silent capture fails too); in `make tests`. Capture is bit-identical run to run on aarch64; cross-arch stability is what CI's x86_64 leg tells us. sfx_from_wav is silent without input, left out |
| R7 | Manifests for the interactive examples; multitap and mouse probes | `ScanMPlay5`, `ReadMouse`, examples with no manifest | L | M | **first slice done 2026-09-15 (8 of the ~50 uncovered)** — the 7 `games/` plus `input/move_sprite` now have input-driven manifests: 51 → 59 manifests, all green. Each scripts real input and asserts a measured global (never an invented one), and most carry a negative too — the ball that must not move before START, the Tetris piece that must stay at column 0 against the wall, the stopped Mode 7 car whose steering must not move the centre, the plane that must not climb below minimum speed. `move_sprite` has no game global at all (its coordinates are `main()` locals), so it asserts the OAM shadow it writes through. Remaining: the ~42 non-interactive examples (backgrounds, colour, HDMA, text, windows), the multitap path (`ScanMPlay5`) and mouse sensitivity |
| R8 | `luna bench` nightly, `--native-res` for modes 5/6, `--call-stack` on failure | G24 | S | L | wiring | **done 2026-09-15** — `make luna-bench` collects the corpus into a flat directory and runs `luna bench` (nightly `luna-bench.yml`, report uploaded); only a `bug` verdict fails, `suspect` is luna's word for a screen that never changes while the CPU runs, which most examples do on purpose (first run: 28 ok, 0 bug, 57 suspect of 85). `native_res = true` in `manifest.toml` puts `--native-res` on the capture of `backgrounds/mode5_hires` (its fbhash changed, baseline re-captured; luna's `run --screenshot` still writes the averaged PNG — observation filed in `status/luna_stress_campaign.md`). A DEAD or FAIL coverage verdict now prints `luna state --call-stack` |
| R10 | Power-on-state dependencies found by R1 | R1 first run | S (audio ×6) / M (scanline 223) — **both done 2026-09-12** | **H** | wiring done; findings were ROM/lib bugs | six audio examples now `dmaClearVRAM()` after `consoleInit()`. The scanline-223 case was the hardware vertical-scroll quirk (the PPU never outputs scanline 0, anomie-regs BG Scrolling): the lib now writes `y - 1` to `BGnVOFS`/`M7VOFS` (crt0 sync + reset default, map.asm, `mode7SetScroll`), so `bgSetScroll(bg, x, 0)` puts tilemap row 0 on the first line; PVSnesLib writes the raw value (documented in KNOWN_LIMITATIONS, CLAUDE.md, porting.md, scrolling.md). All 85 examples now render identically from zero and random RAM; the random visual pass gates `make tests`. Open lib question kept: should `consoleInit()` clear VRAM by default? |
| R9 | Consolidate open luna observations | luna #207 #210 #211 #212; "`--input` ignored after `--until-frame`" | S | M | **done 2026-09-12** — #207–#212 had closed in v1.16.0, the `--input` bug is fixed in v1.20.0 (luna report of 2026-09-12); notes updated | — |

### Docs / agent knowledge

| id | title | gaps | eff | rel | first step |
|---|---|---|---|---|---|
| D1 | Correct the memory-model claims | K5 | S | **H** | **done 2026-09-11** (`014a58f4`) — `CLAUDE.md:104-105` and `ROADMAP.md:286` → the B2 / `FAR` / #127.3 model; cite `.claude/notes/chantiers/b2_far_ram.md` |
| D2 | `.claude/STRUCTURAL_DEFECTS.md` refresh | K6 | S | M | **done 2026-09-13** — §5–§7 rewritten around the live ledger; the re-read found that B3, B4 and C2 had been closed by A6 / A4 / audio v2 without the catalogue noticing (B3: `dmaCopyVramMode7` is the loader and is bank-aware; the last two example asm loaders migrated to it; B4: `hdmaSetup` reads the pointer bank; C2: no function has two implementations), D2 closed by the corpus arbitration; A8's telemetry window has elapsed (retirement step due); A5 restated with the H1 numbers and the three queued upstream reports. Three stale doc claims fixed on the way: `hdma.h` "hardcodes bank $00", `mode7.h` and the Mode 7 tutorial naming a `mode7LoadGraphics()` that never existed. Tier 1 is complete |
| D3 | In-repo luna reference, one version | K1 | S | M | **done 2026-09-14** — `docs/tools/luna.md` is generated from the pinned binary's own `--help` by `devtools/gen_luna_doc.py` (11 subcommands, stamped with `luna.version`); `make tests` runs `--check`, which fails when the page differs from the installed pin and skips where no luna is installed (the lint job). Linked from the toolbox index |

| D4 | `docs/README.md` index + header → tutorial map | K2 K3 | M | L | **done 2026-09-15** — the index now lists all 24 tutorials (grouped by what you are doing), the 6 craft guides, the 9 tool pages, the hardware reference and the contributor docs, with every link checked to resolve. The header → tutorial map is measured, not asserted: each header's declared functions were matched against the prose, which is how the gaps got their honest state — `object.h` (15 functions) and `text.h` (14) have no tutorial at all, `input.h` documents the button masks but none of the pad/mouse/Super Scope API, `console.h` and `interrupt.h` are partial, `fixed32.h`, `gameloop.h`, `asset.h`, `lzss.h`, `debug.h` are uncovered. That list is the documentation backlog |
| D5 | "How to profile" guide | K4 | S | L | **done 2026-09-15** — `docs/tutorials/profiling.md`: the master-cycle unit (≈357 400 per NTSC frame, ≈51 800 in VBlank), how to read every column of a `luna profile` report (why max/frame decides and the average does not, what idle% means), `--from-frame` and `--input` to profile the game rather than the boot, `--budget SYMBOL=MCLK` with its exit codes (0/1/2) as the CI gate, the `profile.h` colour-bar and scanline instruments, and a decision table between them. Worked numbers are measured on `sprite_swarm`. Carries the R4 caveat explicitly: budgeting `NmiHandler` today measures the 652-mclk entry stub |
| D6 | Corpus source list + golden queries in repo | K7 | S | M | **done 2026-09-12** (`5ea11467`: `.claude/notes/tech/cartouche_corpus.md`) — `.claude/notes/tech/cartouche_corpus.md` from §9 |
| D7 | ROADMAP "in flight" table for the five `wip/*` chantiers | K8 | S | M | **done 2026-09-14** — the premise had expired: no `wip/hicolor*`, `interlace`, `mode7-perspective` or `spc700` branch exists; the four `wip/*` on origin (three CI hygiene branches of 09-07, one harness branch of 06-22) were all superseded by develop and are deleted. ROADMAP has a "Work in flight" section that says so and points at this backlog |

| D8 | PVSnesLib migration guide + FAQ | K10 | M | L | **done 2026-09-15** — `docs/MIGRATING_FROM_PVSNESLIB.md` (what is the same, the difference table, the Makefile and `LIB_MODULES` habit, the five traps that fail silently — argument push order, `setScreenOn` last, CGRAM 128, the vertical-scroll -1, const-in-asset-banks — the refused constructs with their workarounds, a PVSnesLib → OpenSNES API map built from the real headers, and the asset pipeline) and `docs/FAQ.md` (getting started, the language, the silent failures, building and shipping, contributing). Both wired into the Doxygen inputs and the main page |
| D9 | Close or advance stale notes | clock-skew root cause, `wla_span_upstream_report.md`, `816_opt_analysis.md:45`, inlining Phase 2, `a1_followup_long_is_kw.md` | S each | M (clock skew) | **done 2026-09-15** — clock skew: 1802 submodule sources dated 2029–2030 (the retired CI `touch -d 2030` cache stamp), reset with `touch`, note CLOSED; SPAN report was filed as vhelin/wla-dx#729 on 2026-07-26, note renamed without `_DRAFT`; 816-opt TODO answered (WLA-DX has no branch relaxation); inlining note marked SHIPPED (`inline.c`); `b2_far_ram.md` §10b credited to A9, §10f re-checked; `hardware_docs_audit.md` F1 rewrite marked done; `a1_followup_long_is_kw.md` no longer exists |

### RAG

| id | title | gaps | eff | rel | first step |
|---|---|---|---|---|---|
| RAG1 | Missing-sources list handed to the owner | K7 (corpus side) | S (the list) | M | **done 2026-09-11/12** — handed over as `opensnes_report_snes-rag_2026-09-11.md` and validated in `opensnes_report_snes-rag_2026-09-12_validation.md` (owner-side, `~/opensnes_reports/`); the corpus note in the repo is D6 |


---

## 11. Recommended ordering

| Tier | Items |
|---|---|
| **1 — do next** | P1, P3, D1, C3, L1, H2, R1, H3, R3, H1, D2 |
| **2** | C2, C1, L2a, L2b, R5, H4, H5 (lodepng + IT loader), P4, D3, D7, P2, R4, RAG1, D6, R9 |
| **3 — later** | L2c, L3, C4, H5 (remaining targets), H6, H7, H8, R2, R6, R7, R8, P5, P6, P7, D4, D5, D8, D9 |

Tier 1 is every item that is small-to-medium effort *and* high reliability:
CI running what the docs say it runs (P1, P3); three cheap gates that find
bugs in code already written (L1, H2, H3); two luna flags that cost one
argument (R1, R3); three doc fixes that stop an agent acting on a wrong
model (D1, C3, D2); and H1, which closes the "PIN bump validated by SHA only"
hole. Tier 2 builds oracles for surfaces that have none today (C1, C2, L2,
R5) and the harness work that needs a few days. Tier 3 is adoption, hygiene
and the long tail of manifests — worth doing, but no single item there
catches a bug class the earlier tiers do not.

### 11b. Tier 3 in four lots (agreed 2026-09-15)

Tier 1 and Tier 2 shipped between 2026-09-12 and 2026-09-15 (rows marked
**done** in §10; R4 waits on luna folding the `NmiHandler@<child>` profile
rows into the parent). What remains is grouped so each lot is one theme,
one validation shape and a few commits:

| Lot | Items | Theme | Validation |
|---|---|---|---|
| **1** | ROADMAP stale lines, H8, D9 | docs and notes hygiene, one small refactor | `make tools`, `make test-tools`, `make fuzz-replay`, `make lint`, `make tests` |
| **2** | L2c (collision, sram, window, interrupt, console asserts), C4, R6, R2 | oracles for the still-untested lib surface; the compiler-test ratchet; the PAL and audio-hash passes | libtests ROM + `make tests`; ratchet numbers move in the same commit |
| **3** | H5 (remaining parsers), H7, P6, P7, P5, R4 (when luna folds child rows), R8 | host hygiene and coverage; the per-frame NMI budget; nightly luna bench | **done 2026-09-15 except R4** — the three remaining parsers fuzz (two real bugs fixed), `make coverage-host` + its CI job, actions SHA-pinned with Dependabot and an `.editorconfig`, Doxygen warnings driven to zero and gated, orphans verified wired, `make luna-bench` nightly and the hi-res native capture. R4 re-checked on luna v1.23.0: the NMI handler is still five profile rows, so the VBlank time budget stays blocked on the fold |
| **4** | D8, D4, D5, R7 | the long-tail docs (migration guide, docs index, profiling guide) and the interactive-example manifests | **done 2026-09-15** — D4, D5 and D8 complete; R7 delivered its first slice (the 7 games plus `input/move_sprite`, 59 manifests green) and stays open for the remaining ~42 examples, the multitap and mouse paths |

Deliberately parked: **H6** (valgrind on static release binaries — ASan
covers the same code paths since H3 and the release job already runs the
corpus on both Linux arches) and **L3** (`memcpy`/`memset`/`strlen` in
the lib — no lib or example caller needs them yet; bulk moves go through
the DMA helpers, and a C loop covers the rest until a real user appears).

---

## 12. Verification of this review

Run before commit; results recorded in the commit body.

1. Every gap id once in §10: `G1`–`G27`, `K1`–`K10`, `S1`–`S7` each appear
   exactly once in the backlog tables, except the two facet splits stated at
   the top of §10.
2. Every backticked path exists at HEAD `0ffd06bf`; every `path:line`
   reference was read (`sed -n`) and says what this review says it says.
3. Every luna flag named exists in v1.18.0 (`tools/luna-test/bin/luna <cmd> --help`).
4. Every Makefile target named exists (`tests`, `test-tools`,
   `test-manifests`, `lint`, `lint-docs`, `test-compiler`, `docs`, `release`).
5. No hardware claim (§2).
6. `make lint-docs` green: this review cites no examples count and no
   version macro.
7. Committed alone (`docs(review): …`), on `wip/gaps-review` from `develop`,
   not on `wip/luna-frame-capture`.
