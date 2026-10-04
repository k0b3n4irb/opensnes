# Doc Consistency Rules (Auto-loaded)

CRITICAL: Three classes of doc/code drift have caused hard-to-trace problems
in this project's history. They are now mechanically blocked by
`devtools/check_doc_drift.py`, which is wired into `make lint-docs` and the
`doc-drift` job in `.github/workflows/lint.yml`. Read this file before
touching any doc that names a version, an examples count, or the framework
opt-in list.

## What the sentinel watches

1. **`lib/include/snes.h` version macros** must match the head version in
   `CHANGELOG.md`. Ship-blocker: a public macro that lies. Caught
   historically as `OPENSNES_VERSION_STRING "0.1.0-dev"` while the project
   was at v0.16.0 — every release shipped wrong macros for months.

2. **`ROADMAP.md` "Current Status: post-vX.Y.Z" line** must match the
   `CHANGELOG.md` head. Caught historically as ROADMAP saying
   "post-v0.13.0 (developing toward v0.14.0)" while the project was at
   v0.16.0 — three minor versions stale.

3. **Examples count claims in active docs** (`ROADMAP.md`, `README.md`,
   `.claude/rules/*.md`, and since 2026-09-26 the `Makefile`, the
   workflows and `tools/luna-test/README.md`) must match
   `find examples -name 'main.c' | wc -l`.
   Caught historically as the pre-v0.16.0 count (one off the current
   total) sticking around in `testing.md` and `nmi_audit.md` after
   v0.16.0 shipped the new `scene_stack` example. `CHANGELOG.md` is
   exempt — historical entries freeze a past state on purpose.

4. **ABI prototypes quoted in `compiler/ABI.md`** must match the canonical
   declaration in `lib/include/snes/*.h`. `ABI.md` is the canonical ABI
   reference; a stale prototype there silently invalidates its whole
   stack-offset table. Caught historically as the `oamSet` worked example
   pinning a 6-arg `(u8 id, u16 x, u16 y, u16 attr, u8 size, u16 tile)`
   signature while the real API is 7-arg `(u16 id, u16 x, u16 y, u16 tile,
   u16 palette, u16 priority, u16 flags)` — every offset in the table was
   wrong. Only prototypes whose function name exists in a header are
   checked; illustrative prototypes and code-block *calls* are ignored.

5. **Example paths quoted in onboarding docs** (`ROADMAP.md`,
   `examples/README.md`, `docs/GETTING_STARTED.md`) must exist under
   `examples/`. Caught historically as GETTING_STARTED sending newcomers
   to `memory/superfx_3d` (real home: `graphics/effects/superfx_3d`) and
   ROADMAP listing a `sa1_speed` example that never existed.

6. **Per-category counts in `examples/README.md`'s table** must match the
   per-directory `main.c` count, and their sum the corpus total. Caught
   historically as the table summing to 53 under a "56 examples" header.

7. **`ROADMAP.md` footer date** (`*Last updated: YYYY-MM-DD`) must not
   predate the head release date in `CHANGELOG.md`. Caught historically
   as a 2026-05-07 footer under a post-v0.26.0 (2026-07-02) status line.

8. **No pre-A6 bank-assumption prose in `lib/source/*.asm`** — comments
   claiming "assumes … bank $00" or "only passes 16-bit pointers"
   describe the pre-A6 ABI; post-A6 the bank is read from the far
   pointer's bank byte. Caught historically as dma.asm's "BANK
   LIMITATION" header contradicting its own per-function stack maps —
   prose the ABI lint (which only reads `lda N,s` annotations) can't see.

9. **Example READMEs name the modules their Makefile links** (since
   2026-09-26): each `examples/*/*/README.md` has a "Modules" section in
   which every `LIB_MODULES` name of the Makefile appears (any format).
   Caught as 33 READMEs without the section and 5 omitting `gameloop`,
   `math` or `fixed32`.

10. **Docs cite only functions that exist, and deprecated ones as
    deprecated** (since 2026-09-26): an SDK-shaped call no header declares,
    or an `OPENSNES_DEPRECATED` name cited without saying so on that line or
    the one before, fails. Caught as `colorMathSetMaskMain/Sub`,
    `objRegisterTypes`, `spcLoad/spcPlay`, `mosaicEnable` in API_INDEX.

11. **No retired tool in `.claude/agents`, `skills`, `hooks`** (since
    2026-09-26): Mesen2, opensnes-emu, `tests/*.sh` — caught as the
    snes-engine-reviewer agent committed with all three.

12. **Every `?=` variable of `make/common.mk` is on `docs/tools/build.md`**
    (since 2026-09-26), backticked. Caught as eight knobs named in no page
    (`USE_FASTROM`, `ROMSIZE`, `SPCSRC`, the three thresholds…) and one,
    `BPP`, that nothing read.

13. **The benchmark table of `docs/BENCHMARK.md`** (since 2026-10-03): each
    row's OpenSNES figure must equal `devtools/cyclecount/bench_baseline.json`,
    the TOTAL row must be the sum of its rows, and the percentage and the
    summary line must follow. Caught as the page saying −32.2 % for four
    months after far pointers had made it −20.4 %. After an intentional
    codegen change: `make bench` to see the new figures, update the baseline
    and the page in the same commit.

14. **No example teaches a fixed or imaginary bug** (since 2026-10-04): a
    short motif list over `examples/**/*.{c,h,md}` — `framesize=158`, "uses
    logical shift", "assumes bank $00", "spill to bank 1", "must be in bank
    $00 WRAM", "due to a compiler quirk". Caught as six comments and README lines
    (shmup_1942, superscope, window, parallax_scroll, two_players) still
    teaching the pre-A6 bank constraint, a logical shift the compiler never
    did, a string spill that #127.3 ended, and a stale cost figure
    (examples audit, F_examples.md PF4). Explain history in other words.

Count claims (anchor 3) are matched on a **soft-wrapped** view of each doc
(single newlines count as spaces), so a claim split across two lines —
ROADMAP's historical `54\nworking examples` — can no longer hide, and the
`### Examples (N)` / `(N / N)` forms are matched too.

## Mandatory workflow

Before committing any change that touches one of those classes:

```sh
make lint-docs
```

It exits 0 on green, 1 on drift with an actionable message per finding.

## Doc-render convention: never put an inline `code` span inside `"..."`

Doxygen's Markdown parser mangles an inline code span placed inside an ASCII
double-quoted phrase — it leaks into the page as a literal `<tt>…</tt>` tag (when
the line wraps) or as raw backticks (single line). So write the citation **or**
the code, not code-inside-a-quote:

- ✅ `the principle: compute the` `` `left` `` `and` `` `right` `` `edge …`
- ✅ `PHILOSOPHY.md calls out "no printf in core lib"`  (quote, no backticks)
- ❌ `"read from bank` `` `$00` `` `"`  ·  ❌ `"no` `` `printf` `` `in core lib"`

A source-side regex can't reliably catch this (a code span *between* two quotes
looks like one *inside* a quote), so the guard scans the **generated HTML**:
`make docs && python3 devtools/check_doc_render.py` (wired into the `doc-render`
CI job). Caught historically as `<tt>WH0</tt>`-style literals across the
window/hdma/math/sram tutorials.

## When to update each anchor

- **Version macros (`lib/include/snes.h:30-40`)**: in the same commit as
  the new `## [X.Y.Z]` heading in `CHANGELOG.md`. The pre-release checklist
  in `.claude/rules/release.md` enforces this.
- **ROADMAP "Current Status"**: at release-PR time, when CHANGELOG gets
  the new heading. One line, one commit.
- **Examples count claims**: prefer not to hard-code numbers in active
  rules. The canonical pattern is "every example" / "the full suite" —
  see `testing.md:14` and `nmi_audit.md:57` for the form. Hard-coded
  numbers belong in changelog snapshots and release notes only.
- **ABI prototypes in `compiler/ABI.md`**: whenever a public function's
  signature changes in `lib/include/snes/*.h`, update the matching worked
  example in `ABI.md` in the same commit — the signature line, the codegen
  push list, and the stack-offset table all move together.

## Adding a new anchor

If a new class of drift causes you pain ("test count moved and three docs
disagreed", "module list rotted", etc.), extend
`devtools/check_doc_drift.py` with a new `check_*` function, add a section
to this file, and add an entry to the workflow under `doc-drift`. Don't
solve a class of drift twice — solve it in the sentinel.

## What NOT to add to the sentinel

- Anything inherently dynamic (test counts that move every chantier; the
  full corpus is covered by `tools/luna-test/luna_runner.py --coverage`).
- Pure prose (commit messages, README narrative). The lint should catch
  drift in *anchored claims*, not in writing.
- Anything CHANGELOG-frozen by design.

## When this rule does NOT apply

- Editing CHANGELOG entries for past releases.
- Files under `docs/` that intentionally describe a past state (release
  notes, migration guides).
- This file itself.
