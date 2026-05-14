# Changelog

All notable changes to OpenSNES are documented in this file.

OpenSNES is forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib). This changelog
covers changes made since the fork.

## [Unreleased]

## [0.18.0] — 2026-05-14

Function inlining + lib retrofit release. The compiler gains end-to-end
`inline` keyword support (chantier "function inlining"), plus deferred
asm emission with consumption-aware standalone suppression that makes
C99 cross-TU inline patterns work end-to-end. Sixteen lib helpers are
retrofitted to use the new inline path — 55 examples now inline
`setScreenOn()`, 22 inline `setScreenOff()`, plus scattered wins on
`fixCos`, `textSetPos`, `colorMathInit/Enable`, etc.

Total cycle reduction vs PVSnesLib+816-opt improves from **−29.1 %**
(v0.17.0) to **−32.2 %**. The previously-regressing `call_chain` bench
recovers from +31.9 % to −59.6 % vs PVSnesLib+opt. The catalogue's A4
(oamSet perf cliff) is acknowledged RESOLVED — fix actually shipped
2026-03-03 via tactical ASM rewrite (commit `39dbff8`) but the doc
hadn't been updated. B4/B6/D1 also receive partial-shipped status
updates after a full catalogue audit. No public API breakage.

### Compiler

- **Function inlining end-to-end (chantier "function inlining").** The
  C `inline` keyword now drives a real qbe inline pass. cproc forwards
  the keyword as a QBE IR linkage hint
  (`Lnk.inline_hint`); qbe's new `inline.c` records each eligible
  callee's body during the per-function pipeline and splices it into
  caller IR at every direct call site. Eligibility heuristic:
  linear-flow (no phi, no conditional jumps), no nested `Ocall`, no
  `Oalloc*`, ≤ 8 IR instructions (overridable via
  `CC_INLINE_MAX_INSTR=N` env var). The bench's `helper(helper(x))`
  pattern was the motivated target — its previous +31.9 % regression
  becomes a −59.6 % win once `helper` is marked `static inline`.
  Reference: `compiler/qbe/inline.c`, qbe SHAs `13d9b14`, `29b0941`,
  `77d07a0`. 410/410 quick suite green throughout.

- **Deferred asm emission + consumption-aware standalone suppression
  (cross-TU enabler).** The earlier inline ship only worked within a
  single TU (user `static inline` helpers in their own .c file). For
  lib helpers defined in a header to inline at example call sites, the
  body needs to reach the inline pass without the linker seeing
  duplicate symbols. Implementation: cproc/decl.c always emits inline
  bodies into IR (drops the C99-inlinedefn-based skip);
  qbe/main.c buffers each function's asm in an `open_memstream` buffer
  during the per-function pipeline; qbe/inline.c tracks
  per-fn `n_direct` / `n_inlined` / `n_declined` / `n_indirect`
  consumption counters; `flush_pending()` skips the standalone in two
  cases — (a) every direct caller in this TU inlined and no indirect
  refs, or (b) the function isn't used in this TU at all
  (header-only inclusion). A canonical TU forces the standalone via a
  data-section indirect reference (`void(*const force_emit)(...) = fn;`).
  Reference: `compiler/qbe/main.c`, `compiler/qbe/inline.c`,
  qbe SHA `988073d`, cproc SHA `42a5c46`.

- **`CC_INLINE_MAX_INSTR` default bumped 8 → 16 (qbe `2eb9f53`).**
  Opens wave 4 candidates (`textSetPos` 10, `fixCos` 11,
  `colorMathEnable` 14). Dormant for the 12 helpers shipped with the
  ≤ 8 cap. No measurable bank-$00 ROM regression.

- **A6.8: large-frame indirect addressing + Kl slot widening
  (qbe `4aa4989`).** Standalone prerequisite for the in-flight A6+A7
  atomic patch (full pointer ABI, parked on
  `wip/a6-a7-atomic-v3`). Adds an indirect addressing fallback for
  functions whose stack offsets exceed the 8-bit `<N>,s` cap, and
  widens Kl temp slots to 4 bytes. Not user-visible in v0.18.0; lays
  the groundwork.

### Library (perf retrofits)

Sixteen lib helpers now ship with the C99 `inline` pattern. Every TU
that includes the right header inlines the body at direct call sites;
the canonical `.c` file emits one standalone fallback for fn-pointer
callers. Per-call overhead drops from ~28 cycles (JSL + RTL + prologue
+ epilogue) to zero.

- **Wave 2 (`ebcf428`):** `getBrightness`, `mosaicInit`,
  `hdmaWaveSetSpeed`, `scopeCalibrate`, `colorMathInit`,
  `colorMathDisable` — 6 helpers, ~28 cycles each per call, 2 with
  active example callers (scopeCalibrate × 1, colorMathInit × 1).
- **Wave 3 (`98387fd`):** `setScreenOn` (the big win — **55** example
  callers), `getFrameCount`, `gsuInit`, `scopeSetHoldDelay`, plus the
  static-inline of `mosaic_update_register` collapsing 5 intra-TU
  callers. Saves ~1736 cycles across the SDK on the dominant
  setScreenOn path.
- **Wave 4 (`85416eb`):** `textSetPos`, `fixCos` (+ paired `fixSin`),
  `colorMathEnable` — the 9-14 IR-instr range opened by the threshold
  bump. ~112 cycles saved across 3 examples (random, aim_target,
  transparency).
- **Wave 1 was `setScreenOff`** (already in the deferred-emit ship,
  `2b4e2f8`) — 22 example callers, the first symbol exercised end-to-end.

Total: ~2700+ cycles saved across the SDK at typical example call
sites. The pattern (`extern u8 state;` in header + `inline void
fn(...)` body + `void(*const force_emit)(...) = fn;` in canonical .c)
is reusable; the audit in
`.claude/notes/chantiers/lib_inline_retrofit_assessment.md` documents
when to retrofit and when to skip.

### Performance

- **Cycle benchmark TOTAL: 1341 → 1282** (-4.4 % vs v0.17.0).
- **vs PVSnesLib+816-opt total: -29.1 % → -32.2 %** (improvement of
  +2.3 percentage points).
- **`call_chain` recovered**: 62 → 19 cycles, going from
  +31.9 % vs PVSnesLib+opt to −59.6 % (a 91-point swing on the
  benchmark's previously-worst case).
- **`helper` standalone eliminated post-deferred-emit**: counted as
  0 cycles in the table with a (†) footnote — its body lives
  exclusively inside `call_chain` after the inline splice. ROM-size
  win (~6 bytes per fully-inlined helper).

The wins concentrate on:
- direct-call elimination at user call sites (every `setScreenOn()`
  collapses to two register writes)
- dead-standalone removal (fully-inlined helpers don't bloat ROM)
- `call_chain`-shaped patterns (small wrapper around a small wrapper)

### Docs / Catalogue

- **A4 oamSet perf cliff RESOLVED 🟢** (commit `5fdad3b`). The fix
  actually shipped 2026-03-03 (commit `39dbff8`) via ASM rewrite in
  `lib/source/sprite_oamset.asm` — framesize dropped from 158 to 0.
  Catalogue + `KNOWN_LIMITATIONS.md` had 2.5 months of drift; both
  updated to reflect reality.
- **B4 (`hdmaSetup` hardcodes bank $00) → 🟡 partial** (commit
  `6db13fc`): `hdmaSetupBank()` shipped, 1/5 HDMA examples migrated.
- **B6 (no atan2/sqrt/pow) → 🟡 partial**: `sqrt16`, `atan2_8`,
  `fixSqrt` shipped (algorithmic, not LUTs as originally proposed,
  but functional). `pow_lut` is the remaining gap.
- **D1 (snes9x doesn't detect GSU) → 🟡 partial**: Mesen2 visual
  regression phase shipped and CI installs Mesen2 unconditionally
  (ubuntu-22.04 ABI match + xvfb for headless). The "fail-build if
  Mesen2 unavailable" criterion is met for CI; local dev still skips.
- **Function inlining audit + assessment docs**: full Phase 0/1
  audit before implementation, post-mortem on the C99 strict pattern
  attempt, then the unblock when the cproc/QBE deferred-emit chantier
  shipped. References:
  `.claude/notes/chantiers/function_inlining_audit.md`,
  `.claude/notes/chantiers/lib_inline_retrofit_assessment.md`.
- **Wip/* branch policy** (`060dd93`): squash-merge then delete.
  Documented in `.claude/rules/release.md` after a cleanup pass found
  5 stale `worktree-agent-*` branches + 1 `backup/develop-pre-reword-*`
  living forever.
- **Bench re-baselined twice** (`ccaf9c7` post-v0.17.0, `022572d`
  post-deferred-emit). `docs/BENCHMARK.md` reflects the
  −32.2 % current state with full per-function breakdown.
- **A6+A7 chantier audit phase complete** (multiple docs commits):
  the in-flight full-pointer-ABI chantier has its 9-site atomic patch
  plan, the empirical findings of two implementation attempts, and a
  replay script. Parked on `wip/a6-a7-atomic-v3` for the focused
  session that needs Mesen2-debugger access to close the residual 46
  failures.

### Tooling / Infrastructure

- `compiler/PINS.md`: bumped to qbe `5c23467` (40 patches, was 31)
  and cproc `42a5c46` (10 patches, was 8). Patch count grew across
  the function-inlining + deferred-emit chantier; trend opposite of
  catalogue A5's "shrink toward upstream" goal but the patches are
  load-bearing for the perf wins.

### Build & CI (cross-platform unblock)

Three pre-existing CI failures resolved end-to-end in the same session
that closed the v0.18.0 release. The Linux + Functional Tests jobs
were already green; macOS arm64 and Windows MinGW had been red for
many commits. All four jobs now green on the release tip.

- **`open_memstream` dependency removed (qbe `eaf6116`).** The earlier
  deferred-emit chantier bufferised each function's asm output through
  `open_memstream` (POSIX 2008). MSYS2's UCRT does not ship the call,
  which broke the Windows qbe build before any lib compile ran.
  Redesigned as a 2-pass parse: pass 1 (in the parse callback) runs
  SSA cleanup + `inline_record` and collects every Fn; pass 1.b runs
  `inline_check` module-wide so consumption counters see every TU
  caller; pass 2 finalises and emits, skipping fully-consumed inline
  bodies. The w65816 backend writes `w65816_alloc_size[]` /
  `w65816_alloc_slots` globals in abi0 and reads them at emit time;
  the new abi0 snapshots the per-fn state into a side list and emit
  restores it, since the 2-pass model separates abi0 from emit by
  many other functions' processing. Output is byte-identical to the
  previous design across the lib and every example.

- **macOS arm64 SIGBUS fixed (qbe `444edea`).** `parsedat` declared
  `Dat d` on the stack and fired the `DStart` callback before
  `d.isref` / `d.u.ref.name` were written, so those bytes carried
  stack garbage. The OpenSNES `data()` callback's `inline_record_dat_ref`
  lookup then dereferenced a junk pointer at `r->n_indirect` (offset
  108 in `InlRec`). Linux x86_64 / aarch64 typically saw zero bytes
  there so the surrounding `if (d->isref && d->u.ref.name)` test
  failed harmlessly; macOS arm64's stricter stack hygiene produced
  reliable SIGBUS when compiling `background.c`. Fix gates the call
  on `d->type != DStart && d->type != DEnd` so the `isref` byte is
  only trusted once parsedat has written it. Diagnosed with a
  SIGBUS/SIGSEGV crash handler added in qbe `4de6a97` that dumps
  `backtrace_symbols_fd()` output — pinpointed the crash site without
  arm64-Apple hardware.

- **Windows MinGW build fix (qbe `5c23467`).** The diagnostic handler
  used `<execinfo.h>`, which is glibc + Apple libsystem only. MinGW
  does not ship it, so the Windows qbe build aborted before any lib
  compile reached the cproc segfault retry. The handler is now guarded
  behind `__has_include(<execinfo.h>)` so it compiles out on Windows;
  Windows crashes continue to be diagnosed via
  `msys2_cproc_diagnostic.yml` + the retry loop in
  `opensnes_build.yml`.

## [0.17.0] - 2026-05-09

Compiler & runtime hardening release. Five structural defects from
the post-audit catalogue (`.claude/STRUCTURAL_DEFECTS.md`) close in
this version (A1, A2, A3, E1, E2), one new lib API ships (B6 — sqrt
and atan2), and the build infrastructure gains two permanent guard
rails (NMI/WRAM port race lint, hard cycle-count CI gate). Three
🔴 silent failures from the catalogue downgrade to 🟢 (caught at
build time). Public API is unchanged across the upgrade.

### Compiler

- **`volatile` is preserved through the cproc → QBE pipeline
  (chantier A2).** Pre-A2 cproc literally discarded the
  `QUALVOLATILE` qualifier with `(void)(tq & QUALVOLATILE);` and a
  `TODO` comment. As a result every C `volatile` load/store entered
  QBE IR as a plain operation, and the load-forwarding pass coalesced
  redundant volatile reads through value-numbering — a silent
  miscompile that violated the C standard's "each access is a side
  effect" volatile semantics. Concrete pre-fix symptom:

      volatile unsigned short timer;
      unsigned short read_twice(void) { return timer + timer; }
                                              /* MUST emit 2 loads */

  Pre-A2 emitted `lda timer; sta cache; clc; adc cache` (1 load +
  1 cached read). Post-A2 emits two distinct `lda timer` loads as
  the standard requires.

  Fix touches three submodules: cproc threads `QUALVOLATILE` through
  `funcload`/`funcstore` and emits a `volat` IR keyword via a new
  `funcinst_volat` helper; QBE's `Ins` struct gains a `volat` field,
  the parser recognises a `Tvolat` token, and `loadopt` /
  `promote` / `gcm` skip volatile-tagged instructions; the
  opensnes-emu test fixture's `volatiles` phase now asserts load /
  store counts on `test_volatiles.c` rather than just compilation
  success. PINS.md lists 8 cproc patches and 29 qbe patches now.
  KNOWN_LIMITATIONS, CLAUDE.md, the compiler/new_example rules, and
  compiler/ABI.md updated to remove the historical "use globals
  instead of `volatile`" warnings — the lib still favours globals
  for NMI handshakes for cycle-cost equivalence, not because
  `volatile` is broken.

- **`int` is now 16 bits, `long` is now 32 bits on cc65816 (chantier A1).**
  cproc's IR was inherited as a host-target compiler with
  `sizeof(int) = 4` and `sizeof(long) = 8` hardcoded — wrong for a
  16-bit CPU. The QBE w65816 backend already treated these classes
  as 16/32-bit at emit time (per the `d929b94` patch's class `'l'`
  reinterpretation), so cproc's IR sizes were inconsistent with
  reality. Aligned in `compiler/cproc/type.c` (4 lines, 7 cproc patches
  total now). Bare `int counter` no longer silently produces 32-bit
  arithmetic — a documented `KNOWN_LIMITATIONS` 🟡 closes to 🟢. The
  benchmark suite shows `loop_sum` improved by 5 cycles per iteration
  (the only function in the suite using bare `int`); other functions
  are unchanged because the SDK convention is `u16` everywhere.
  Pointer size deliberately stays at 8 — its companion fix is the
  separate chantier A6 (pointer ABI + bank-byte preservation) tracked
  in the structural-defects catalogue. `compiler/PINS.md` now lists 7
  cproc patches; submodule bumped to `7f26c16`.

### Tests

- **Visual baselines for `basics/random` and `superfx_3d` regenerated** —
  both already documented as timing-fragile in `BASELINES.md`. The
  cproc compiler change shifts emitted instructions slightly, which
  cascades to a different RNG seed at frame 120 (random) and a
  slightly different rotation angle (superfx_3d). Pixel comparison
  catches any rendering-pipeline regression as before; the new
  baselines pass byte-by-byte under the rebuilt parent. Submodule
  `tools/opensnes-emu` bumped to `c440825`.

### Performance

- **`WaitForVBlank` no longer sets `oam_update_flag` unconditionally** —
  the runtime's NMI handler already conditionally skips the OAM DMA
  when the flag is clear, but `WaitForVBlank` was setting the flag on
  every call, defeating the optimisation. Sprite-idle ROMs (text-only
  games, paused screens, splash frames) now save ~4.3 K cycles per
  frame (the cost of the unnecessary 544-byte OAM DMA). The contract
  is now: **callers that mutate `oamMemory[]` directly must set
  `oam_update_flag = 1` themselves.** Every OAM-mutating function in
  `lib/source/sprite.c` and the `oamSetFast` / `oamSetXYFast` macros
  in `lib/include/snes/sprite.h` already set it; the documented
  user pattern (per `KNOWN_LIMITATIONS.md` "Performance traps") is
  unchanged. See `templates/crt0.asm:1184+` for the updated function
  comment naming the new contract explicitly.

### Library

- **`<snes/math.h>` ships `sqrt16`, `fixSqrt`, and `atan2_8`
  (chantier B6).** The "where is the target relative to me, and how
  far?" surface every 2D action game leans on now lives in one
  module:
    - `sqrt16(n)` — bit-by-bit integer square root, ~80 cycles, no
      LUT, result bounded to 255 (fits in `u8`).
    - `fixSqrt(x)` — 8.8 fixed-point sqrt; delegates to `sqrt16`,
      4 fractional bits of precision (capped intentionally —
      raising it requires 32-bit shift currently broken under the
      QBE 32-bit codegen gap, catalogue chantier A7).
    - `atan2_8(dy, dx)` — 8-bit angle of the `(dx, dy)` vector in
      the same convention as `fixSin` / `fixCos` (0=+X, 64=+Y, …).
      65-byte LUT covers the first octant, symmetry handles the
      other seven, scale-invariant reduction prevents the internal
      16-bit divide from overflowing on any 16-bit input.

  The `atan2_8 → fixCos / fixSin` chain is the canonical pattern
  for projectile aiming, pursuit AI, and "rotate sprite to face X".

  `lib/source/hdma.c` was using a private static `isqrt()` for the
  iris-wipe radius — promoted to `sqrt16` (same algorithm,
  identical results). `make/common.mk` adds the transitive dep
  `_DEP_hdma := dma math_sqrt`, which means hdma-using examples
  pull only the small sqrt module rather than the full math runtime.

- **`math.c` split into `math.c` + `math_sqrt.c`** (post-B6
  hygiene). The hdma → math transitive dep would otherwise have
  dragged the full sine LUT (512 B), atan LUT (65 B), and
  arithmetic helpers into every hdma-using example, even when the
  caller only needed `sqrt16` through hdma. Six examples reclaimed
  ≈ 1721 bytes of bank-$00 free space each (e.g. `parallax_scrolling`
  5628 → 7349, `gradient_colors` 12324 → 14045). Five examples that
  listed `math` in `LIB_MODULES` without using it had the dead
  listing removed (`window`, `parallax_scrolling`, `hdma_gradient`,
  `transparent_window`, `gradient_colors`). Public `<snes/math.h>`
  surface unchanged — the resolver flattens
  `_DEP_math := math_sqrt` so `LIB_MODULES := math` still pulls
  sqrt as a transitive dep.

### Examples

- **`examples/basics/aim_target` — interactive showcase for
  `sqrt16`, `atan2_8`, and trig chaining.** Player diamond at
  screen centre, X target follows the D-pad. Each frame the
  example recomputes `dx`/`dy`, calls `sqrt16(dx² + dy²)` for
  pixel distance, calls `atan2_8(dy, dx)` for the 8-bit angle,
  then calls `fixCos`/`fixSin` on that angle to read back the
  unit direction vector. Live readout panel above the play area
  shows DX, DY, DIST, ANGLE, COS(ANGLE), SIN(ANGLE) — all the
  numbers update as the user moves the target. First example in
  the suite that exercises fixed-point math at all.

### CI / build infrastructure

- **NMI / WRAM data port race lint shipped (chantier E1).** The
  SNES WRAM data port (`$2180`-`$2183`) shares state between the
  main thread and the NMI handler; if main-thread code is
  mid-sequence on those ports and an NMI fires whose callback also
  touches them, the address pointer is silently corrupted and the
  main thread resumes writing to the wrong location with no error.
  KNOWN_LIMITATIONS flagged the trap with a 🔴 severity tag and
  documentation-only mitigation. Now caught at build time:
  `devtools/check_nmi_wram_race.py` parses each example's
  `combined.asm` + `*.c.asm` intermediates, walks the call graph
  from every NMI callback root (NmiHandler + DefaultNmiCallback +
  every symbol passed to `nmiSet`/`nmiSetBank`), takes the
  closure, and fails the build if any reachable function writes
  to `$2180-$2183`. Wired post-link in `make/common.mk` next to
  the BANK0 ratchet. Bypass per-build with `SKIP_NMI_RACE_CHECK=1`.
  Severity downgraded 🔴 → 🟢. Regression suite
  `devtools/test_check_nmi_wram_race.py` (6 cases) wired into
  `.github/workflows/lint.yml` as the new `nmi-race-tests` job.

- **Cycle-count CI gate promoted from soft to hard (chantier E2).**
  The gate shipped 2026-05-08 in soft / comment-only form
  (commit `98d5014`); after 24 h of operation the threshold design
  held up and is now hard. Two-armed thresholds:
    - **Total cycles** > 5 % regression → fail (broad-drift catch)
    - **Per-function** > 25 % AND > 50 cycles absolute → fail
      (pathological catch — the combined percent + absolute floor
      avoids small-routine noise)

  Override path: include `Cycle-Regression-OK: <reason>` as a
  trailer in any commit in the PR range. The workflow scans
  `git log <base>..<head> --format=%B`; if the trailer is present
  the gate exits 0 even on a breached threshold (the comparison
  is still posted as a comment with a "regression overridden"
  header so the trade-off is visible to reviewers). Trailer
  preferred over PR labels (audit trail in git history forever),
  over comment markers (no audit trail), over self-service
  bypasses (anyone with push access can write the trailer).
  `devtools/cyclecount/cyclecount.py` gains `--fail-on-regression`
  (with tunable `--total-pct-limit` / `--fn-pct-limit` /
  `--fn-abs-limit`).

### Tests

- **`[KNOWN_BUG]` markers retired and `--allow-known-bugs` dropped
  from CI (chantier A3).** The compiler-test phase carried 7
  stale `knownBug()` calls escaping assertions for "TCO not
  implemented yet" and "A-cache through pha not implemented yet".
  Investigation compiled each fixture and inspected the generated
  ASM: every optimisation the markers gated had already shipped
  (TCO via C.1 / C.2.1 / C.2.2; A-cache through pha audited
  working in C.6; lazy `rep #$20` emission active). The
  assertions were drifting behind the codegen, not anticipating
  future work.

  The `compute_across_call.*leaf_opt=1` assertion in
  `nonleaf_frameless` was actively wrong — that function MUST
  get `leaf_opt=0` because `sum` is live across the call (the
  `all_calls_are_tail()` gate at `compiler/qbe/w65816/emit.c:2882`
  correctly returns 0 for it). Rewritten to check
  `forward_with_work.*leaf_opt=1` instead.

  7 `knownBug()` calls converted to `fail()`. CI workflow's
  `--allow-known-bugs` flag dropped. Real regressions on TCO /
  A-cache / lazy rep now hard-fail. The MSYS2-segfault paths
  remain — they only fire on Windows / MSYS2 builds and are
  orthogonal to the optimisation gaps.

- **`test_volatiles` phase strengthened from a smoke check to
  semantic assertions** (companion to chantier A2). Three reads
  of `hw_status` must produce ≥3 `lda hw_status` instructions;
  four writes of `hw_data` must produce 4 stores; the
  read-modify-write helper must produce 2 reads + 2 writes
  balanced. Pre-A2 the QBE loadopt pass coalesced the reads
  silently, and the test never noticed because it only checked
  compilation succeeded.

### Fixed

- **`examples/games/tetris` HDMA gradient migrated from channel 7 to
  channel 6** — the example was writing directly to `$4370`-`$4374`,
  reusing the channel reserved by the NMI's OAM DMA path. The conflict
  was masked at runtime by `WaitForVBlank`'s unconditional flag set
  re-programming channel 7 every frame; with the perf change above
  removing that unconditional set, the latent contract violation is
  exposed. Migrated to channel 6, which the lib does not reserve.

### Maintainer / docs

- **Strategic-defects catalogue promoted to
  `.claude/STRUCTURAL_DEFECTS.md`** (was previously scratch in
  `/tmp` between sessions). The file tracks effort estimates, risk
  profiles, an interaction matrix, sequencing paths, and
  investigation logs for every defect that requires a deliberate
  multi-day chantier. Lives under `.claude/` because it's
  maintainer-internal operational planning, not user-facing
  documentation — contributors looking at the project's headline
  state still read `KNOWN_LIMITATIONS.md` and `ROADMAP.md` at the
  repo root for the public severity tags. CLAUDE.md gets a
  "Strategic Planning" section pointing at the catalogue.

- **`A6` (pointer ABI + indirect-call bank-byte preservation)
  partial implementation reverted with full investigation log.**
  Attempted `A6.1` (Ostorel CAddr → memory bank byte) +
  `A6.3` (indirect call read bank byte from slot+2). The fix
  passed 404/404 tests by coincidence: `A6.1` writes the bank
  byte to a memory location, but `A6.3`'s read targets a stack
  slot — the two are disconnected at the QBE Oload boundary
  (Oload is 16-bit only on the w65816 target). Tests passed
  because all framework callbacks land in bank `$00` thanks to
  SUPERFREE placement, so `lda #$00` and `lda slot+2,s` happened
  to emit identical-effect code. Reverted; root cause documented;
  the full A6 chantier requires a coupled A6.4 / A6.5 / A6.6
  bundle (cproc pointer 8→4 + QBE Kl class 24-bit load/store +
  ASM site audit) before any of it can ship safely.

- **New defect `A7` logged in the catalogue.** Surfaced during
  B5 (`fixed32` 16.16) investigation: the QBE w65816 backend
  truncates *every* 32-bit arithmetic operation to 16 bits
  silently. `Oadd`, `Osub`, `Oshl`, `Oshr`, `Oneg`, and the
  `loaduw → __mul16` lowering all emit a single 16-bit
  instruction with no carry / borrow into the high half. A1
  shipped `sizeof(long) == 4` in cproc IR but did not extend
  the QBE backend to actually do 32-bit arithmetic — storage
  is right, operations are wrong. The lib's own `fixDiv`
  (`lib/source/math.c`) is broken for any dividend > 127
  because of this; no shipped example exercises that path so
  the bug is currently latent. Validated runtime with a
  reproduction ROM displaying five `BUG` rows for the four
  synthetic cases plus the lib `fixDiv` direct hit. Catalogue
  entry has the full investigation, the reproduction ROM
  spec, and the proposed A6.4-style fix surface (slot widening,
  `__mul32` runtime, carry-aware Oadd/Osub).

- **Visual baselines for `basics/random`, `games/tetris`, and
  `graphics/effects/superfx_3d` regenerated** — all three are
  documented timing-fragile examples (RNG seeded by `frame_count`,
  gameplay-driven state, continuously-animated rotating cube), and
  the one-cycle shift in `WaitForVBlank` produces small frame-120
  visual deltas that exceed the 50-px tolerance. Pixel comparison
  catches any rendering-pipeline regression as before; the new
  baselines pass the rebuilt suite cleanly. Submodule
  `tools/opensnes-emu` bumped to `0931e1a`.

## [0.16.0] - 2026-05-07

Framework trilogy completed. The two remaining "framework opt-ins"
promised by `PHILOSOPHY.md` (alongside D.1 `gameloop` in v0.15.2)
ship in this release: `<snes/asset.h>` for typed background and
tileset bundles (D.2), `<snes/scene.h>` for a push/pop scene stack
(D.3). An audit-driven cohesion pass aliases the gameloop config
type to the new `Scene` type, defers scene init to the next VBlank
dispatch (closing a silent VRAM-overrun footgun), and drops a
parallel macro API that did not justify its surface.

Two compiler fixes shipped along the way: chantier C.5 padded DL/DW
static init data emissions to match `dtype_size` (which unblocked
typed struct values with pointer fields), and the C.7
JSL-in-conditional codegen bug was confirmed silently fixed by an
earlier QBE chantier — the workaround macro is gone. Chantier P3.2
closed four residuals from the sprite/text duplication audit
(orphan font header, duplicated size-table macros, unused
`oamDrawMetasprite` API, "legacy" → "internal" naming).

### Added

- **`<snes/asset.h>` + `asset` library module (chantier D.2)** —
  typed `BgAsset` / `GfxAsset` value form for full background and
  tileset bundles, plus `bgLoad()` / `gfxLoad()` runtime functions.
  Constructor sugar via `DECLARE_BG_ASSET(name, color_mode, map_size)`
  / `DECLARE_GFX_ASSET(name, color_mode)` that expand to the six
  `extern` declarations and a populated `static const`. Symbol-naming
  convention: `<name>_tiles` / `<name>_pal` / `<name>_map` (each with
  matching `_end` siblings). Replaces the previous "declare six
  externs, compute three sizes, thread eight positional parameters
  through `bgInitTileSet`" boilerplate with a single typed value.

- **`<snes/scene.h>` + `scene` library module (chantier D.3)** —
  push/pop scene stack with `sceneRun(initial)`, `scenePush(next)`,
  `scenePop()`. `Scene` is two function pointers (`init`, `update`).
  Init for the initial scene runs eagerly (matches `gameLoopRun`);
  init for pushed scenes runs at the next VBlank dispatch (full
  budget for DMAs). Static stack of 8 deep, silent no-ops on
  overflow / pop-from-bottom. Closes the framework opt-in trilogy.

- **`examples/basics/scene_stack`** — title → counter → pause overlay
  demo of the scene framework. ~120 LOC, exercises push/pop, eager
  vs deferred init, and shared state via a file-scope global.

### Changed

- **`GameLoopConfig` is now `typedef Scene` (audit H1)** —
  field-for-field identical structs unified. Source-compatible: every
  existing `GameLoopConfig cfg = { .init = ..., .update = ... }`
  keeps working via the typedef; new code is encouraged to use
  `Scene` directly for vocabulary consistency across the trilogy.

- **`scenePush()` no longer calls `init` synchronously (audit H2)** —
  the dispatcher in `sceneRun` now runs init right after
  `WaitForVBlank()` instead of inside the caller's `update` frame.
  This gives init's DMAs (palette, tiles, tilemap) a fresh ~33 K
  cycle VBlank budget. The initial scene's init keeps its eager
  pre-VBlank semantics so `consoleInit` / `setMode` / `setScreenOn`
  ordering still works as in `gameLoopRun`.

- **`examples/graphics/backgrounds/mode1` and
  `examples/graphics/backgrounds/mode1_bg3_priority` migrated to
  the typed `BgAsset` form (audit H3)** — the `BG_LOAD` /
  `GFX_LOAD` statement-form macros are dropped (one flavour, no
  fuzzy choice between two parallel APIs).

### Removed

- **`oamDrawMetasprite` from `<snes/sprite.h>` (chantier P3.2)** —
  was declared as a "legacy simple interface" with zero callers
  in the entire repository. Drops the symbol and the
  `docs/hardware/OAM.md` table mention.

- **`BG_LOAD` / `GFX_LOAD` statement macros from `<snes/asset.h>`
  (audit H3)** — parallel API to the typed `bgLoad` / `gfxLoad`
  functions. Both did the same thing; the macros are gone, the
  typed form is the only form.

- **`lib/source/opensnes_font_2bpp.h` (chantier P3.2)** — orphan
  font header with 1536 bytes of inlined data and unused `FONT_*`
  macros, referenced by nothing. Drops 219 lines of dead code.

- **`examples/maps/dynamic_map` C64 sprite converter dead code** —
  `convertC64Sprite()`, `getPixel()`, `isBitSet()`,
  `sprite_temp[256]`, the `c64_sprite` extern, and the 144-byte
  data block. Unused since the example was ported. Drops 161 LOC.

### Fixed

- **QBE w65816 emit pads DL/DW init data (chantier C.5)** —
  `compiler/qbe/emit.c` `emit_init_data()` now adds `.dsb N, 0`
  after each emitted directive whose written size is smaller than
  `dtype_size[type]`. Without this, `static const Struct { ptr }`
  values had every field after the first pointer at the wrong ROM
  offset (cproc lays out 8-byte stride for `u8 *`, but WLA-DX
  writes 3 bytes for `.dl <symbol>`). The bug had never manifested
  in shipping code because no example carried such a struct; the
  fix unblocks the typed `BgAsset` value shipped in D.2.

- **`scenePush` from `update` no longer overruns the VBlank
  budget (audit H2)** — the pushed scene's init used to share the
  caller's already-consumed window; if init did heavy DMAs it
  could exceed the budget and the PPU would silently drop the
  writes. See "Changed" above for the dispatcher refactor.

### Compiler

- **JSL-in-conditional codegen bug confirmed silently fixed
  (chantier C.7)** — the `MODE_LARGE_SIZE` / `MODE_SMALL_SIZE`
  workaround macros (which existed solely to keep helper calls
  from inlining as JSLs inside an `if (sz == 0) { sz = helper(); }
  if (sz == 8) ...` chain) are converted back to plain functions
  in their own translation unit (`lib/source/sprite_dynamic_helpers.c`).
  `slopemario` (the historical repro) and four other examples
  that link the dynamic sprite engine still render correctly with
  real `jsl` calls in the codepath. Memory note
  `cc65816_conditional_jsl_codegen_bug.md` updated to mark FIXED.

- **`compiler/PINS.md` qbe SHA bumped** to pull in the C.5 fix.

### Documentation

- **Framework trilogy headers cross-reference each other** —
  `<snes/scene.h>` carries an explicit "`Scene` ≡ `GameLoopConfig`"
  paragraph, `<snes/gameloop.h>`'s "When NOT to use this" points
  at `<snes/scene.h>` for state-machine needs, and `asset.h` drops
  the "two flavours" section in favour of "typed value is the
  contract".

- **Stale doc references purged**: `gameloop.h`'s "future C.3 may
  close [the indirect-call TCO] gap" speculative promise dropped,
  the "scene_2d / scene-stack module that's planned next" forward
  reference updated to point at the now-shipped scene module,
  `asset.h`'s "may be added once the patterns settle" sprite-asset
  hint replaced with "out of scope here".

- **`scene.h` documents the contract edge cases** — null `update`
  is documented as undefined (no runtime check by design), the
  `scenePop`-from-`init` pattern pops the scene before its first
  update, push-cascade from `init` runs all chained inits in the
  same VBlank window, scene-swap idiom is `scenePop(); scenePush(next);`,
  inter-scene argument passing uses globals (out of scope by
  design).

- **`docs/mainpage.md` API Reference** lists the three framework
  headers under a new "Framework opt-ins" subsection.

- **README.md and `examples/README.md` example counts refreshed**
  from 53 / 36 to the actual 54.

- **`KNOWN_LIMITATIONS.md` refreshed** — two entries promoted
  🟠 → 🟢 (WLA-DX `.ACCU`/`.INDEX` tracking, now lint-enforced;
  SuperFX/snes9x detection, now covered by the Mesen2-headless
  CI phase). Two stale entries removed (TCO and A-cache through
  `pha` — both fixed in earlier chantiers but still listed as
  open). The "five [KNOWN_BUG] entries" intro line corrected to
  "one" — the only remaining compiler known-bug is the cosmetic
  `leaf_opt=1` comment marker on non-leaf frameless functions.

### Internal cleanup

- **`MODE_LARGE_SIZE` / `MODE_SMALL_SIZE` extracted to a shared
  internal header (chantier P3.2)** — were duplicated identically
  in `sprite_dynamic_dispatch.c` and `sprite_dynamic_meta.c`. Now
  live in `lib/source/sprite_dynamic_internal.h` (and after C.7
  was confirmed fixed, the macros became plain functions in
  `sprite_dynamic_helpers.c`).

- **`sprite_dynamic_dispatch.c` "legacy" comments → "internal"
  (chantier P3.2)** — the ASM entry points behind `oamDynamicInit`
  / `oamDynamicDraw` are not deprecated, they are the
  implementation layer.

### Process / build

- **`.gitignore`: track `lib/source/**/*.h`** — required because
  internal-only headers (`sprite_dynamic_internal.h`) live next to
  their `.c` files, but the previous rule treated all `.h` outside
  `lib/include/` as build artefacts.

## [0.15.2] - 2026-05-01

First of the three "framework opt-ins" promised by `PHILOSOPHY.md`
(alongside the planned scene/state stack and the asset bundle
convention). Ships an opt-in `gameloop` module that owns the
`while (1) WaitForVBlank(); update();` cadence so user code can stay
focused on its own logic.

### Added

- **`<snes/gameloop.h>` + `gameloop` library module (chantier D.1)** —
  opt-in via `LIB_MODULES`, not auto-included by `<snes.h>`. Public
  surface is one struct and one function:
  ```c
  typedef struct {
      void (*init)(void);     // optional, NULL to skip
      void (*update)(void);   // required, MUST NOT be NULL
  } GameLoopConfig;

  void gameLoopRun(const GameLoopConfig *cfg);  // never returns
  ```
  Implementation is three lines of actual logic. The framework owns
  the WaitForVBlank → update cadence and nothing else: no
  `consoleInit()`, no `setMode()`, no screen on/off — all of that
  remains caller-driven so an existing example can opt in by
  extracting init and update into static functions and having
  `main()` hand off to `gameLoopRun`. Documented in the header,
  including the explicit "When NOT to use this" section listing
  state-machine and custom-rhythm patterns that don't fit.

### Changed

- **`examples/basics/timer`, `examples/basics/random`,
  `examples/input/controller`** migrated to the gameloop framework
  as canonical demos of the common pattern (single VBlank sync, an
  `update` that reads input and writes to the tilemap, no custom
  rhythm). ROM bytes change (one indirect call per frame for the
  update dispatch); visual regression at frame 120 is identical for
  controller and timer, shifts ~62 pixels for random's DEC line —
  a one-pixel-column timing offset from the indirect-call overhead
  that's visually indistinguishable. basics/random's baseline was
  refreshed to absorb the shift.

- **`examples/games/breakout` and `examples/games/tetris` deliberately
  NOT migrated.** Both have custom synchronisation rhythms — breakout
  ends each frame with `WaitForVBlank(); oamUpdate();` (work-then-
  sync, with the OAM DMA piggybacking on the same VBlank), tetris
  drives WaitForVBlank from inside per-state functions. The framework
  imposes sync-then-work; migrating either would either drop them to
  30 fps or require restructuring that obscures more than it teaches.
  The "When NOT to use this" section in `gameloop.h` is calibrated
  against exactly these patterns.

### Documentation

- **PHILOSOPHY.md → `gameloop`** — first of the three opt-in framework
  pieces is no longer a placeholder.

## [0.15.1] - 2026-05-01

Audit closure pass. No SDK-consumer-visible changes; the work
finishes off two items from the original 2026-04-26 remediation
plan that the v0.15.0 cycle left partial.

### Added

- **`devtools/lint_asm.py`** — enforces the explicit `.ACCU` /
  `.INDEX` marker policy after every `rep`/`sep` in hand-written
  `.asm` files (lib/source/, templates/). The bug this prevents
  is silent: WLA-DX's per-object-file mode tracking loses
  precision at branch merges and at section boundaries; the
  assembler has historically shipped 2-byte `cpx #0` (8-bit
  immediate) where 3 bytes were needed, cascading misalignment
  through every following instruction. Explicit markers override
  the inferred state — they are pure directives so adding them
  is byte-for-byte identical to the prior ROM. Wired into the
  `Lint` GitHub workflow as the `asm-markers` job.
- **`tools/sa1-patch/`** — dedicated C tool that flips bits 0-1
  of byte $7FD5 in a freshly-linked .sfc to mark it as SA-1
  rather than LoROM. Replaces a python3 one-liner that used to
  live inline in `make/common.mk`, fitting the same source-tree
  pattern as `gfx4snes`, `font2snes`, `smconv`, etc. (audit P2.4
  #3).

### Fixed

- **`devtools/lint_commits.py`** — skip GitHub-style merge commit
  subjects (`Merge pull request #N from owner/branch`,
  `Merge branch 'X' into Y`, `Merge remote-tracking branch ...`).
  Previously every release-PR merge into `main` failed the Lint
  workflow because the auto-generated wrapper subject doesn't
  satisfy Conventional Commits; happened on PR #35 and PR #36
  during the v0.15.0 release flow. The contributor's own commits
  in the same range are still linted individually, and the
  merge commit's body still gets the `Co-Authored-By:` rejection
  check. `.claude/rules/commits.md` updated alongside.

### Build

- **490 `.ACCU` / `.INDEX` markers added** across 21
  hand-written `.asm` files (lib/source/ + templates/),
  generated by `devtools/lint_asm.py --fix`. Top contributors:
  `templates/crt0.asm` (58), `lib/source/sprite_dynamic.asm`
  (69), `lib/source/audio.asm` (55), `lib/source/map.asm` (47),
  `lib/source/snesmod.asm` (46). Verified byte-identical against
  the v0.15.0 ROMs via SHA-256 over all 53 examples.

### Documentation

- **`.claude/rules/testing.md`** gains an "Impacted-Examples
  Triage" section that codifies the workflow used during the
  chantier C TCO validation: identify candidate examples →
  triage to a representative subset → present a small table
  with one "what to look for" per kept entry. Replaces the
  previous "list ALL impacted examples" instruction, which
  produced unreadable walls for Class A changes.
- **`.claude/rules/commits.md`** — new section documenting the
  merge-commit exemption for the lint policy.

## [0.15.0] - 2026-04-30

Audit-driven maintenance cycle (see `~/opensnes_audit_2026-04-26.md` and
`~/opensnes_remediation_plan_2026-04-26.md`). 19 of 23 plan items completed
across P0-P4. The SDK is now end-to-end CI-enforced: build green no longer
just means "it compiles" — it means visual regression, lag detection, runtime,
compiler patterns, bank $00 layout and submodule pins all hold. Adds the
`PHILOSOPHY.md` design-principles document, completes chantiers B (sprite)
and T (text) of P3.2 (public API surface 28 → 18 functions for sprite,
18 → 15 for text), lands the Mesen2-headless visual phase (P3.4) for real
GSU/SA-1 emulation coverage on CI, and ships the chantier C trilogy
(C.1 + C.2.1 + C.2.2) of tail-call optimisations in qbe/w65816 — 18 TCO
sites across the SDK, 4 of the 5 documented known compiler bugs cleared.

### Added

- **`PHILOSOPHY.md`** — design-principles document making the project's
  positioning explicit. OpenSNES is a 2D game engine for C developers
  who don't want to learn 65816 assembly to ship a SNES game; PVSnesLib
  is a thin C wrapper for hardware enthusiasts. The two sit at
  different altitudes of the same stack. Five principles guide every
  API decision (sane defaults with escape hatches, hidden quirks with
  documented escape, opt-in modules, type-safe boundaries, predictable
  performance). Linked from `README.md`, `CLAUDE.md`, `ROADMAP.md`.
- **`KNOWN_LIMITATIONS.md`** — public catalog of silent failures with
  severity tags (🔴 silent corruption / 🟠 silent build / 🟡 toolchain quirk /
  🟢 mitigated). 14 entries covering VBlank, DMA budget, WRAM port, bank $00,
  push order, `volatile` + QBE, `.ACCU` tracking, SA-1 SIWP, SuperFX/Mesen2,
  compiler optimisation gaps, type sizes, sprite-palette CGRAM offset.
- **`compiler/ABI.md`** — empirically verified calling-convention reference:
  LEFT-TO-RIGHT push, frame layout, return values, direct-page layout
  (`tcc__r*`), type sizes, calling C↔ASM templates, port-from-PVSnesLib
  checklist.
- **`compiler/PINS.md`** — pinned SHAs for `compiler/{cproc,qbe,wla-dx}` with
  a per-submodule list of carried-forward local patches.
- **`tools/opensnes-emu/test/BASELINES.md`** — visual-regression baseline
  schema and regen protocol.
- **`lib/contrib/` directory** — non-core engine modules. Currently houses
  `object.asm` (3 124 LOC game-entity engine) relocated from `lib/source/`
  so `lib/source/` only contains hardware-wrapping code.
- **Section "Who is OpenSNES for?"** in README — explicit prerequisites
  (65816 ASM, NMI/VBlank model, hex addresses) and non-targets, plus an
  enhancement-chip maturity table.
- **Branching policy** in `CONTRIBUTING.md` — invariants for `main` vs
  `develop`.
- **2 compiler regression guards** — `test_arg_push_order` (LEFT-TO-RIGHT
  ABI) and `test_section_directives` (`.ACCU 16` / `.INDEX 16` markers).
  Compiler test suite: 60 → 62.

### Changed

- **`tools/opensnes-emu/` is now a public submodule** at
  `github.com/k0b3n4irb/opensnes-emu` (was a local-only directory in the
  user's tree). History was rewritten with `git filter-repo` to strip 275
  generated build artifacts (.sfc, .sym, .o, .obj, etc.) — repo size went
  from 218 MB to 11 MB tracked source.
- **CI runs the full ~390-check test suite** (was build-only). Drop of
  `--quick` adds the 60-test compiler-pattern phase. Added
  `--allow-known-bugs` so the 5 documented compiler-optimisation gaps
  (TCO + A-cache-through-pha + stale `leaf_opt` marker) report as
  `[KNOWN_BUG]` rather than failing the build.
- **`ROADMAP.md` resync** — corrects 52 → 53 examples, 212 → ~390 checks,
  60 → 62 compiler tests, 7 → 8 phases. Drops removed modules
  (animation, entity). Adds the contrib status for `object`. Pivots to
  the "post-v0.13.0 toward v0.14.0" snapshot.
- **`.claude/` is now tracked** (was entirely gitignored except one file).
  Hooks, rules, skills and shared settings ship with the repo so any
  Claude Code user gets the same gates. Personal `settings.local.json`
  remains gitignored.
- **`setMainScreen()` instead of `REG_TM = ...`** in 22 examples (style
  consistency; identical bytes generated). `short` → `s16` in three more.
- **Sprite API simplification (P3.2 chantier B)** — public sprite surface
  collapses from 28 to 18 functions:
  - `oamSet`/`oamSetEx`/`oamInit*` consolidations (B.A);
  - `oamDynamicDraw(id)` size-aware dispatcher replaces
    `oamDynamic{8,16,32}Draw` (B.1+B.2);
  - `OamDynamicConfig` struct + `oamDynamicInit(&cfg)` replace the
    5-arg positional `oamInitDynamicSprite` (B.3);
  - migrated 9 call sites across 5 example games + reverted a
    Y-1 over-compensation in the dynamic engine that was lifting
    Mario 1–2 px off the ground on flat terrain (B.4);
  - the NMI handler auto-flushes the dynamic engine via a new
    `dynamic_flush_hook` indirect call — main loops drop the
    explicit `oamInitDynamicSpriteEndFrame` / `oamVramQueueUpdate`
    pair (B.5);
  - legacy `oamInitDynamicSprite` and `oamDynamic{8,16,32}Draw`
    retire from the public header; their ASM symbols stay as the
    internal mechanism (B.6 modest);
  - `oamMetaDrawDyn(id, x, y, meta, gfx, OBJ_SMALL/OBJ_LARGE)`
    replaces `oamMetaDrawDyn{8,16,32}` — engine resolves pixel size
    from the size pair set at init (B.6 aggressive part 1);
  - simple init flushes (likemario, slopemario, dynamic_sprite)
    migrate to a single `WaitForVBlank()` that fires the auto-flush
    hook under force blank (B.6 aggressive part 2 partial);
  - `oamDynamicDrainQueue()` lets the multi-VBlank init-time drain
    happen via the same NMI auto-flush, with a draining flag that
    inhibits the end-frame hide step during the drain — which lets
    the legacy `oamInitDynamicSpriteEndFrame` / `oamVramQueueUpdate`
    retire from the public header for good (B.6 aggressive part 2
    complete).
- **Text API simplification (P3.2 chantier T)** — public text surface
  collapses from 18 to 15 functions:
  - `textPrintS16`, `textDrawBox` retired; internal `textFillRect`
    made `static` (T.1) — none had callers across the 53 examples
    or the library;
  - text writers (`textPutChar`, `textClear`, `textFillRect`)
    auto-flush via the NMI tilemap_update_flag (T.4) — 17 examples
    drop the manual `textFlush()` call. `textFlush()` stays public
    as the explicit primitive for hand-rolled tilemap writers.
- **`textGetX()` / `textGetY()` restored** — initially dropped in
  T.2 on a "zero callers" sweep, then reintroduced after the unit
  test fixture in opensnes-emu surfaced as a legitimate caller
  reading cursor advancement. Two 1-line accessors are the right
  shape for testable internal state.
- **NMI handler gains a `dynamic_flush_hook` 24-bit function pointer**
  in the bank-$00 register area. Defaults to a single-`rtl` no-op
  stub; `oamInitDynamicSprite` repoints it at `oamDynamicNmiFlush`
  which calls `oamInitDynamicSpriteEndFrame` + `oamVramQueueUpdate`
  every VBlank. Cost for ROMs that don't use the dynamic engine: one
  PEA + JML indirect + RTL ≈ 25 cycles per frame.
- **Mesen2-headless visual regression phase (P3.4)** — second visual
  phase that runs the four chip-using ROMs (SuperFX 3D, SuperFX
  Hello, SA-1 Hello, SA-1 Starfield) through Mesen2 in `--testrunner`
  mode for real GSU/SA-1 emulation. snes9x's libretro core in
  opensnes-emu does not detect the GSU chip in our ROM headers, so
  its visual phase only validates "boots without crashing" for those
  ROMs; Mesen2 fills the gap. Vendored `vendor/Mesen` binary +
  `scripts/install-mesen2.sh` for fresh contributor setup.
- **TCO for trivial wrappers (chantier C.1)** — qbe/w65816 backend
  now emits `jml callee` instead of the full `jsl + frame teardown
  + rtl` for non-leaf functions whose every Ocall is in tail
  position (e.g. `unsigned short call_add(a, b) { return add_u16(a, b); }`).
  The TCO emission path was already in place; the remaining gate
  was the conservative "non-leaf needs a frame" rule, relaxed to
  recognise that a function with only tail Ocalls never executes
  past a call and so cannot have temps that need across-call
  storage. Result: 3 of the 5 known compiler-bug tests cleared
  (plx_cleanup wrapper variants, acache_pha through pha, lazy_rep20
  for pure tail calls); −5 cycles + −2 ROM bytes per tail call site.
  Includes an env-gated diagnostic (`CC_TRACE_TCO=1`) that logs each
  TCO eligibility decision — zero-cost when the env var is unset.
- **TCO for framed void-tail-call wrappers (chantier C.2.1)** —
  extends C.1 to functions that have a real frame because of an
  earlier non-tail call, but whose final tail call takes zero
  arguments. The Ocall handler now emits a `tsa; clc; adc.w
  #framesize; tas` teardown immediately before the `jml` (gated on
  `framesize > 2` to mirror the prologue's elision of phantom
  alignment-only frames). 10 new TCO sites unlocked across the SDK,
  including `oamDynamicDrainQueue`, `textInit`, `run_frame` (breakout
  main loop), `stateGameOver`/`stateTitle` (tetris), `changeObjSize`
  (object_size example) and `koopatroopaupdate` (likemario AI).
- **TCO for chained tail calls (chantier C.2.2)** — closes the
  `return f(g(x))` pattern, where the inner call is a regular jsl
  (its result feeds the outer arg) and the outer is a tail call.
  The Oarg handler stores each outgoing arg to its caller-slot
  position (offset `4 + framesize + ...` while the frame is still
  up); the existing Ocall teardown then lifts SP back to the
  function-entry point so the callee sees the args at canonical
  `4`, `6`, ... offsets. Required two supporting changes:
  count_fn_param_bytes() now falls through to a slot-walking helper
  for non-leaf functions where Opar* has been lowered to slot
  reads, and an alloc-safety guard declines C.2.2 on functions
  containing any local Oalloc* (the frame teardown would invalidate
  any pointer-to-local that escaped via the tail call — caught
  the hard way: textPrintU16's `char buf[6]` + `textPrint(p)`
  pattern produced a 101-pixel diff in basics/random's DEC line
  before the guard landed). Cleared the last TCO known-bug
  (`tail_call`'s call_chain check) and unlocked 3 more sites in
  the lib (mouseGetX/Y, colorMathTransparency50). Test suite
  without `--allow-known-bugs`: 397/399 → 398/399; the only
  remaining known-bug is nonleaf_frameless's stale ASM-comment
  marker, orthogonal to TCO.

### Fixed

- **SNES PPU sprite Y +1 scanline quirk** — caller-passed Y was off
  by one row from the rendered top because OAM_Y=N renders on
  scanlines N+1..N+8. The `oamSet` ASM and `oamSetY` C path applied
  the `-1` compensation in 87a0ae2; the macros `oamSetFast` /
  `oamSetXYFast` were missed and silently rendered one scanline
  lower than the equivalent `oamSet` call. Now uniform across all
  five entry points so callers can mix the fast and slow APIs without
  drift. `oamHide` is exempt (writes 240 directly).
- **`lib/source/hdma.c:89`** — `sine_quarter[63] = 256` truncated to 0 in
  `u8`, leaving a 1-pixel notch at sin(90°) on every HDMA sine effect.
  Cap the table at 255.
- **`examples/games/tetris/render.c:346`** — `u8 tile` couldn't hold
  `TILE_BORDER_H | PAL1 = 0x409`, dropping the palette bits silently and
  rendering the line-clear flash with the wrong palette. Promote to `u16`.
- **`examples/memory/sa1_starfield/`** — `USE_FASTROM := 1` eliminates
  the 50 % VBlank lag (150/300 frames → 0/300). The 128-bird OAM-update
  loop was running into the next NMI on SlowROM.
- **Hooks `pre-commit-check.sh` / `mark-tests-passed.sh`** — outputted
  `{"decision": "allow"}` (invalid for the Claude Code schema) and
  pointed contributors to non-existent test scripts. Now run silently
  on the pass path and reference the real
  `node test/run-all-tests.mjs --quick` runner.
- **`.gitignore` cleanup** — `.claude/` entry was a blanket gitignore that
  hid project-wide policies. Replaced with the precise
  `.claude/settings.local.json` exclusion.
- **3 hardcoded `/home/kobenairb/...` paths** in `.claude/rules/` and
  skills replaced with `$PVSNESLIB_HOME`.
- **Two dead unit fixtures** (`unit/animation`, `unit/entity`) referenced
  modules removed from `lib/`. Build phase passed from 76/78 to 76/76.
- **2 stale visual baselines** (`graphics/sprites/dynamic_metasprite`,
  `maps/slopemario`) regenerated after recent ASM/c-bit fixes; baseline
  for `memory/sa1_starfield` captured for the first time after the
  FastROM bump.
- **12 latent C warnings** surfaced by the new clang lint pipeline —
  unused locals, dead helper functions (`writestring_bg2`, `refresh`),
  unused parameters in callback ABI signatures.

### Build

- **Bank $00 ROM overflow check** runs on every link
  (`devtools/symmap/symmap.py --check-bank0-overflow` invoked from
  `make/common.mk`). String-literal spills now fail the build instead of
  producing garbage at runtime. `SKIP_BANK0_CHECK=1` escapes if needed.
- **`make verify-toolchain`** compares each compiler-submodule HEAD
  against `compiler/PINS.md`. CI calls it before every build — drift
  fails fast with a fix-it message naming the right remediation.
- **clang `-fsyntax-only -Wall -Wextra -Werror`** runs in parallel with
  cc65816 in the `%.c.o` rule (cproc ignores `-W*`). The SDK is
  warning-clean and stays that way. `SKIP_LINT=1` for environments
  without clang.
- **Sed QBE→WLA-DX transform removed.** Empirical audit showed 9 of 10
  patterns matched nothing in current QBE output (QBE emits `.db`/`.dw`/
  `.dl` natively). The 10th pattern only deleted `/* end */` comments
  that WLA-DX accepts. `lib/Makefile` lost 12 lines of fragile sed.
- **Memmap dependency tracking** — `wrap_asm` consumers now list
  `$(MEMMAP_INC)` as a real prerequisite. `touch templates/memmap.inc`
  triggers a rebuild instead of producing stale objects.
- **FastROM Makefile flag** documented; SA-1 Starfield uses it.

### CI

- **Functional-tests job** — opensnes-emu test suite gates PR merges
  on Linux. Caches WASM core build by `bridge.cpp` hash; cache hit
  brings full-suite runtime to ~3 minutes.
- **Tag-on-main guard** — `release.yml` rejects a tag whose commit is
  not reachable from `origin/main`. Tags can only be cut from the
  release-PR merge.
- **Lint workflow** (`lint.yml`) — runs `lint_commits.py` on every PR
  and direct push to `main`/`develop`.
- **Lint workflow handles force-push** — after a force-push,
  `github.event.before` points to a now-dangling commit that
  `actions/checkout` does not fetch, so `git log ${BEFORE}..HEAD`
  used to fail with exit code 2 (indistinguishable from a real lint
  violation in the workflow output). Now `git rev-parse --verify`
  the SHA first and fall back to `origin/main..HEAD` if it does
  not resolve. The lint policy itself is unchanged.
- **Mesen2-headless phase wired up on CI** — runs on `ubuntu-22.04`
  (matches Mesen2's published x64-AOT build environment, avoids the
  libstdc++ ABI mismatch that surfaces on 24.04 as `std::bad_cast`
  in `MesenCore.so::InitDll`). Installs `xvfb` for a virtual display
  and `libsdl2-dev` for the runtime SDL2 the native core dlopens at
  startup. A 128 KB sanitised settings.json fixture in
  `tools/opensnes-emu/test/fixtures/mesen2-config/` is copied to
  `~/.config/Mesen2/` before launch — bypasses the first-run wizard
  and provides the per-system config subtrees `MesenCore.so`
  dynamic_cast<>s during init.
- **`visual-mesen2.mjs` auto-wraps Mesen2 in `xvfb-run -a`** when
  `$DISPLAY` is unset, so the same phase runs on a developer's
  desktop and on a headless CI runner without conditional logic.
  Failure messages now include the trailing 2 KB of Mesen2 stdout
  / stderr, surfacing the actual failure signature (DllNotFound,
  std::bad_cast, etc.) inline in the CI log instead of as an opaque
  exit code.

### Testing

- **Visual-regression baselines** carry provenance metadata
  (`rom_sha256`, `snes9x_commit`, `captured_at`). The runner emits
  `[WARN]` on drift instead of silently producing a misleading diff.
- All 53 baselines re-captured against the current `make`-built ROMs.
- **Compiler test runner** marks 5 unimplemented optimisations as
  `knownBug()` instead of `fail()` — TCO (3 tests), A-cache through
  pha (1), and the stale `leaf_opt=1` marker for non-leaf functions
  (1). Without `--allow-known-bugs` they still fail; with the flag
  they report `[KNOWN_BUG]` so a contributor implementing one sees
  green immediately.
- **Mesen2 visual baselines** added for the four chip-using ROMs
  (`graphics/effects/superfx_3d`, `memory/superfx_hello`,
  `memory/sa1_hello`, `memory/sa1_starfield`) under
  `tools/opensnes-emu/test/baselines/*.mesen2.bin`. Per-ROM diff
  overrides for the two animated ones (cube rotation 2000 px,
  starfield scroll 1500 px) absorb 1-frame timing drift between
  machines without losing the "chip ran vs chip-NOT-DETECTED black
  screen" signal that is the actual point of the phase.
- **Unit fixture sync after sprite Y quirk** — 13 Y assertions in
  the opensnes-emu `unit/sprite` fixture updated to expect the
  `(input - 1)` convention now applied uniformly across the OAM
  setters. Header note documents the convention so future test
  authors see it. Phase result: `73/73 passed` (was 14 fail / 59
  pass before the sync).

### Tooling

- **`devtools/verify_toolchain.py`** — parses `compiler/PINS.md` and
  enforces submodule-pin invariants.
- **`devtools/lint_commits.py`** — Conventional-Commits subject
  validation + `Co-Authored-By:` rejection.

### Documentation

- **`compiler/ABI.md`**, **`KNOWN_LIMITATIONS.md`**,
  **`tools/opensnes-emu/test/BASELINES.md`**, **`lib/contrib/README.md`**
  added as canonical references.
- **`README.md`** gains the audience-explicit section, chip maturity
  table, and a prominent link to `KNOWN_LIMITATIONS.md`.
- **`ROADMAP.md`** resynchronised against current state.
- **`CONTRIBUTING.md`** documents the branching policy.
- **`.claude/rules/release.md`** has the full release flow with ASCII
  diagram and `make verify-toolchain` in the pre-release checklist.
- **5 stale "212 checks" / "52 examples"** references corrected across
  `.claude/rules/*.md` and the README.

## [0.13.0] - 2026-03-26

### SuperFX Phase 3-4 — Mandelbrot + Wireframe Cube

- **FMULT validated**: 4.12 fixed-point multiply confirmed (2.0×2.0=4.0, 1.5×3.0=4.5).
  Formula: `result_4_12 = FMULT_output << 4` (4× ADD R0).
- **Mandelbrot fractal**: 256×128 computed by GSU via FMULT+PLOT, 16-color palette,
  4.12 fixed-point iteration (z=z²+c, 15 max iterations). LOOP instruction for
  Y iteration (body exceeds BNE 8-bit range). CACHE for ~3× speedup.
- **Wireframe cube**: 12-edge Bresenham line drawing via PLOT, Y+X axis rotation
  computed in C (sin/cos table), orthographic projection. Chunked 4×4KB VBlank DMA
  (no flicker, ~15 FPS). Function splitting to avoid framesize>255.
- **WRAM stub overflow fix**: stub grew to 90 bytes with parameterized SCBR/R8,
  gsu_wram_area increased from 64 to 96 bytes.
- **Mesen2 backward branch bug**: confirmed via bsnes comparison. BNE/BRA+STW
  works on bsnes (cycle-accurate) but corrupts on Mesen2. bsnes is the reference
  emulator for SuperFX testing.

### Documentation

- **READMEs + screenshots**: all 4 SuperFX examples fully documented with
  emulator compatibility table (bsnes/Mesen2/snes9x).
- **EXAMPLES_BY_CATEGORY.md**: added Enhancement Chips section (SA-1 + SuperFX).
- **LEARNING_PATH.md**: added Level 6 (Enhancement Chips).
- **REGISTERS.md**: added SA-1 ($2200-$230E) and GSU ($3000-$303F) register tables.
- **MEMORY_MAP.md**: added SA-1 I-RAM/BW-RAM and SuperFX SRAM/cache layouts.

### Examples (56 total, +4 new SuperFX)

- **superfx_hello**: boot diagnostic + SRAM byte/word + FMULT 4.12 validation
- **superfx_bitmap**: 16-color gradient via PLOT hardware
- **superfx_mandelbrot**: fractal set via FMULT + PLOT (4.12 fixed-point)
- **superfx_3d**: rotating wireframe cube (Bresenham + PLOT, 2-axis rotation)

## [0.12.0] - 2026-03-23

### SuperFX (GSU) Enhancement Chip Support (NEW)

- **SuperFX coprocessor**: full build system support (`USE_SUPERFX=1`).
  Two-stage GSU assembly pipeline: `.sfx` -> `wla-superfx` -> `.sfx.bin` -> `.incbin`.
- **PLOT bitmap rendering**: GSU renders 16-color gradient via hardware PLOT
  instruction at 21.47 MHz with CACHE optimization. Column-major tile layout,
  pixel cache flush via RPIX.
- **Three critical GSU rules discovered**:
  1. Branch delay slot -- NOP after every BNE/BRA (instruction always executes)
  2. STOP pre-fetch -- NOP padding before STOP (pipeline halts prematurely)
  3. RPIX pixel cache flush -- last 8 pixels per row stay in internal cache
- **superfx_hello**: boot diagnostic with STB/STW SRAM write tests
- **superfx_bitmap**: 16-color rainbow gradient rendered by GSU PLOT hardware
- **superfx.h**: complete GSU register definitions ($3000-$303F)
- **WRAM execution**: mandatory for all GSU launches (ROM bus exclusive)
- **21.47 MHz + CACHE**: ~6x speedup over uncached 10.74 MHz baseline

### SA-1 Enhancement Chip

- **SA-1 murmuration demo**: 128 dots in Lissajous sine patterns at 10.74 MHz.
  Uses `lda.l sine_table,x` (opcode $BF) for DB-independent ROM reads.
  4 brightness palettes for depth illusion on dark blue background.

### Build System

- **Unified memmap files**: deleted 3 duplicate `lib/source/lib_memmap*.inc`,
  single source of truth in `templates/memmap*.inc`.
- **runtime.asm moved to lib/**: compiled once per config instead of per-example.
- **Per-example SA-1/SuperFX boot**: `project_sa1_boot.asm` generated from
  local override or template default.

### Documentation

- **SuperFX tutorial**: `docs/tutorials/superfx.md` covering architecture,
  PLOT rendering, assembly rules, WRAM execution, emulator compatibility.
- **SA-1 tutorial**: `docs/tutorials/sa1.md` with I-RAM patterns and debugging.
- **GETTING_STARTED.md rewritten**: Game Developer vs SDK Developer paths.
- **All examples documented**: 54 READMEs + screenshots (opensnes-emu).
- **Documentation audit**: fixed broken links, updated example count, added
  tutorials index.

## [0.11.0] - 2026-03-21

### SA-1 Enhancement Chip Support (NEW)

- **SA-1 coprocessor**: full build system support for SA-1 cartridges
  (`USE_SA1 := 1`). Includes ROM header (cart type $35), memory maps,
  post-link $FFD5 patching, and per-example boot stub override.
- **SA-1 boot infrastructure**: crt0.asm initializes SA-1 (reset vector,
  I-RAM write protection SIWP=$FF, release from reset, magic byte handshake).
- **Per-example SA-1 boot**: each SA-1 example provides its own `sa1_boot.asm`.
  crt0.asm includes `project_sa1_boot.asm` (local override or template default).
- **sa1.h / sa1.c**: register definitions ($2200-$230E), `sa1Init()` function.
- **Key discovery**: SIWP/CIWP bit=1 means WRITABLE (not protected despite
  the register name). Documented in `.claude/SA-1.md`.

### Build System

- **Unified memmap files**: deleted 3 duplicate `lib/source/lib_memmap*.inc`,
  all 18 library ASM files now reference `templates/memmap*.inc` via
  `-I ../templates`. Single source of truth for memory maps.
- **runtime.asm moved to lib/**: compiled once per config (lorom/hirom/sa1)
  instead of 46 times per example. `RUNTIME_OBJ` always linked from library.

### Compiler

- **WLA-DX .ACCU/.INDEX override warning**: detects when `rep`/`sep` tracking
  diverges from explicit `.ACCU`/`.INDEX` directives after branch merges.
  Flags reset at `.ENDS` boundaries to avoid false positives.
- **Leaf optimization fix**: `leaf_opt = fn->leaf && !fn->dynalloc` (was
  `!fn->dynalloc`). Prevents non-leaf functions from corrupting JSL return
  addresses with SSA temporaries.

### Runtime

- **Dynamic sprite state moved to bank $00**: RAMSECTION relocated from
  bank $7E slot 2 to bank 0 slot 1 — C code can now access variables
  via `lda.l $xxxx` (WRAM mirror, below $2000).

### Examples (52 total, +3 new)

- **sa1_hello**: SA-1 boot diagnostic — displays coprocessor status codes.
- **sa1_speed**: SA-1 32-bit counter at 10.74 MHz — shows 69K increments/frame.
- **sa1_starfield**: 128-dot murmuration with Lissajous sine patterns.
  4 sine lookups per bird using `lda.l sine_table,x` (DB-independent ROM reads),
  4 brightness palettes for depth illusion on dark blue background.

### Documentation

- **SA-1 tutorial**: `docs/tutorials/sa1.md` — architecture, setup, I-RAM
  communication patterns, assembly tips, Mesen2 debugging guide.
- **GETTING_STARTED.md rewritten**: two clear paths — "Game Developer" (download
  release, just needs `make`) vs "SDK Developer" (clone, build from source).
- **6 missing READMEs added**: all 52 examples now have README.md + screenshots
  (generated via opensnes-emu headless API).
- **Documentation audit**: fixed 4 broken links, updated example count 41→52,
  added SA-1 to all doc indexes, added tutorials table to docs/README.md.

## [0.10.0] - 2026-03-21

### Library

- **Dynamic metasprite engine**: `oamMetaDrawDyn32()`, `oamMetaDrawDyn16()`,
  `oamMetaDrawDyn8()` — multi-tile sprite characters with per-frame VRAM streaming.
  Iterates MetaspriteItem arrays and calls the proven oamDynamic*Draw functions
  per sub-sprite. oamMetaDrawDyn16 includes `sprsize` parameter for correct
  name table bit handling in SMALL mode.

### Build System

- **Eliminated `combined.asm`**: each ASM source (crt0, runtime, data_init_start,
  user ASMSRC) is now compiled as a separate object file. Fixes HiROM ROMBANKMAP
  linker errors and removes all `cat >>` file concatenation.
- **common.mk refactoring**: 529 → 303 lines (-43%), 32 → 15 conditionals (-53%).
  New `wrap_asm` macro factors the duplicated memmap-include-and-assemble pattern.
  `_HAS_SOUNDBANK` computed once replaces 6 nested conditional blocks. `$(if)`
  one-liners replace multi-line ifeq/else/endif blocks.
- **smconv patched**: generates FORCE sections with `.ORG 0` and `SOUNDBANK_BANK`
  in header directly — eliminates 3 sed post-processing calls.
- **Soundbank multi-bank support**: soundbanks >32KB correctly split across
  multiple ROM banks with FORCE placement. Tested with 56KB (LoROM, 2 banks)
  and 59KB (HiROM, 2 banks) soundbanks.
- **HiROM soundbank support**: soundbank assembled as separate object to avoid
  ROMBANKMAP conflicts. HiROM `.ORG $8000` override for bank mirror access.
- **HiROM text module fix**: `text.asm` and `text4bpp.asm` now include the correct
  HiROM memmap conditionally (was hardcoded to LoROM, blocking all HiROM+text builds).
- **HiROM ROMBANKS**: increased from 4 to 8 (512KB) to accommodate large soundbanks.
- **macOS compatibility**: `sed -i.bak` instead of `sed -i` for BSD sed portability.
- **Header template**: ROM_NAME via single sed, numeric values (CARTRIDGETYPE,
  ROMSIZE, SRAMSIZE) via `.DEFINE` in project_config.inc.
- **Fixed `make clean`**: removed stale `tests/` directory reference.

### Compiler

- **Zero warnings**: 72 Clang warnings fixed across cproc (67) and QBE (5).
  Strict prototypes, switch default cases, operator precedence parentheses,
  assignment-as-condition, sign comparison, unused variables.

### Tools

- **Zero warnings**: gfx4snes, smconv, img2snes, tmx2snes — strict prototypes,
  uninitialized variables, const qualifiers, missing newlines, duplicate
  instrument name handling in smconv (appends `_N` suffix).

### Examples (49 total, +3 new)

- **dynamic_metasprite**: port of PVSnesLib DynamicEngineMetaSprite — 3 OBJSEL
  configurations (8/16, 8/32, 16/32) selectable via D-PAD, glitch-free transitions.
- **snesmod_music_large**: "What Is Love" 108KB IT module — validates multi-bank
  LoROM soundbank (56KB across 2 banks).
- **snesmod_music_hirom**: "What Is Love" 210KB IT module — validates HiROM
  soundbank support with 64KB banks.
- **likemario**: renamed ACT_STAND/WALK/JUMP/FALL to MARIO_ACT_* to avoid
  conflict with map.h bitmask definitions.

## [0.8.0] - 2026-03-15

### opensnes-emu — Debug Emulator (NEW)

- **SNES debug emulator** powered by snes9x (WASM): load ROMs, run frames, capture
  screenshots, inspect VRAM/CGRAM/OAM/CPU/PPU state programmatically.
- **MCP server** with 14 tools for Claude Code integration (stdio transport).
- **Single source of truth**: `tests/` directory removed. All testing handled by
  `node tools/opensnes-emu/test/run-all-tests.mjs` — 212 checks across 7 phases.
- **Visual regression**: pixel-exact screenshot baselines for all 41 examples.
- **Lag frame detection**: measures steady-state lag over 300 frames per example.
- **Compiler benchmark**: cycle count comparison via `run-benchmark.mjs`.

### Compiler

- **fixMul/fixLerp**: rewritten in assembly using SNES hardware multiplier ($4202/$4203).
  C version overflowed because `(s32)a*(s32)b` compiled to 16-bit `__mul16`.
- **Alloc TSA**: alloc instruction now computes absolute stack address via `TSA+offset`,
  preventing aliasing with tcc compiler registers at $0000-$001F.
- **Benchmark baseline**: 33 functions, 1318 total cycles.

### Library

- **BG_4COLORS0 palette banking**: `bgInitTileSet` now computes CGRAM offset as
  `bgNumber*32 + paletteEntry*4` for Mode 0 (matches PVSnesLib).
- **HDMA channel 7 warning**: NMI handler uses DMA channel 7 for OAM — documented
  in hdma.h, window/transparent_window examples fixed to use channels 4+5.
- **fixMul/fixLerp/fixDiv**: hardware multiplier assembly (lib/source/math.asm).
- **objUpdateXY**: header corrected — parameter is raw index, not handle.
- **MultiPlayer5**: new mp5 module for multitap adapter support.

### Runtime

- **NMI handler**: MP5/mouse/scope extracted to SUPERFREE sections. Auto-joypad wait
  before all input reading. OAM DMA revert (preconfigured channel optimization caused
  Mesen2 regression — snes9x didn't catch it).

### Examples (41 total, +4 new)

- **Mode 0**: 4-layer 2bpp Kirby parallax demo (ported from PVSnesLib).
- **Mode 3**: 256-color 8bpp static display with split DMA loader.
- **Mode 5**: Hi-res 512×256 16-color display.
- **Tetris**: Korobeiniki music (SNESMOD), multi-line clear fix, RNG fix, gravity fix.
- **All 41 examples** now have README.md + screenshot.png.

### Documentation

- Streamlined root README (328→150 lines).
- Removed MATURITY_REVIEW.md (replaced by opensnes-emu real metrics).
- Updated all docs for 41 examples and opensnes-emu workflow.

### Removed

- `tests/` directory (17,701 lines) — fully migrated to opensnes-emu.
- `devtools/benchmark/` and `devtools/check_mvn/` — handled by opensnes-emu.
- `examples/benchmark/` — opensnes-emu handles benchmarking.

## [0.7.1] - 2026-03-10

### Build System

- **WLA-DX submodule updated** to latest upstream (42 commits) — assembler/linker
  improvements and bug fixes.
- **Fixed ASCIITABLE warnings**: moved `.ASCIITABLE` definition from `crt0.asm` into
  all memmap include files so every assembly unit gets the identity ASCII mapping.
- **Added `.ACCU 16` / `.INDEX 16`** to `runtime.asm` for correct WLA-DX register
  width tracking in math functions.
- **Release asset filenames now include version tag**
  (e.g. `opensnes_v0.7.1_linux_x86_64.zip`).

### Tooling

- **VS Code project configuration**: shared `settings.json` (file associations,
  IntelliSense), `tasks.json` (12 build/test tasks), `extensions.json`
  (WLA-DX syntax + C/C++ recommended).

### Dependencies

- **Updated vendored lodepng** from 20230410 to 20260119 in gfx4snes and img2snes.

### Housekeeping

- Added `.gitattributes` to fix GitHub language detection (all `.h` → C).
- Removed accidentally tracked `CLAUDE.md` files from repository.
- Updated `ATTRIBUTION.md` with complete dependency and contributor credits.

## [0.7.0] - 2026-03-10

### CI/CD

- **Pre-built binary releases**: new `release.yml` workflow triggered on version tags.
  Builds SDK on Linux, macOS, and Windows, runs tests, creates GitHub Release with
  platform zips attached automatically.
- Release zip filenames now include architecture (`opensnes_linux_x86_64.zip`,
  `opensnes_darwin_arm64.zip`, `opensnes_windows_x86_64.zip`).
- Build workflow skips tag pushes (handled by release workflow).

### Documentation

- **opensnes-emu debug emulator (single source of truth for testing)
  of OpenSNES vs PVSnesLib across 8 dimensions.
- **Published benchmark** (`docs/BENCHMARK.md`): 34-function comparison showing
  -30.3% total cycles vs PVSnesLib + 816-opt (32/34 function wins).
- Updated ROADMAP from v0.3.0 to v0.6.0 with structured v1.0 milestones.
- Updated README: beta status badge, comparison table, accurate counts (37 examples,
  28 headers, 60 compiler tests, 25 unit modules).
- Getting started guide now offers pre-built SDK download as primary option.

## [0.6.0] - 2026-03-09

### Compiler

- **Signed division and modulo**: emit `__sdiv16` / `__smod16` for signed `/` and `%`
  operators.
- **Fixed cproc `mktype()` uninitialized fields**: garbage in `type->qual` could cause
  mutable structs to be emitted as `.rodata` (ROM). Fixed by initializing all fields.
- Eliminated redundant loads in phi-move A-cache.

### Library

- **HDMA effect helpers**: new high-level library functions for wave, ripple, iris wipe,
  brightness gradient (`hdmaWaveEffect`, `hdmaBrightnessGradient`, etc.).
- Fixed `oamDrawMeta` return value and metasprite mode switching.
- Migrated color gradient HDMA to bank $00 RAM.
- Double-buffer ripple mode fix and edge wrapping.
- Iris tables moved to bank $00 for correct HDMA bank byte.

### Runtime

- Signed 16-bit division and modulo (`sdiv16` / `smod16`) in `runtime.asm`.
- Division-by-zero guard and X register preservation in software division.

### Examples

- HDMA helpers demo example.
- Migrated hdma_wave and hdma_gradient to library helpers.
- Fixed continuous_scroll: replaced fragile nmiSetBank callback with bgSetScroll.

### Build System

- Flattened `templates/common/` to `templates/`.
- Removed dead startup code from crt0.asm and libsnes.asm.

## [0.5.0] - 2026-03-08

### Compiler

- Fixed memory leaks in cproc (cleanup functions added).

## [0.4.0] - 2026-03-07

### Toolchain

- **smconv rewritten in pure C** (was C++). Simpler build, no C++ dependency.

### CI/CD

- cproc segfault retry for Windows MSYS2 stability.
- Replaced cproc|qbe pipe with temp file to catch cproc crashes.
- GitHub Pages deployment workflow for Doxygen docs.

### Documentation

- Restructured documentation with learning path and navigation hub.
- Removed Co-Authored-By requirement from commit messages.

### API

- **Removed 4 deprecated functions**: `padUpdate`, `dmaCopyToVRAM`, `dmaCopyToCGRAM`,
  `dmaCopyToOAM`. Use `padHeld`/`padPressed` and `dmaCopyVram`/`dmaCopyCGram`/`dmaCopyOam`.

## [0.3.0] - 2026-03-05

### Examples

- **Transparency rewrite**: Replaced placeholder color math demo with proper PVSnesLib-style
  transparency example — landscape (BG1, 4bpp) blended with scrolling clouds (BG3, 2bpp) via
  color addition, using assembly DMA loader for correct bank bytes.
- **HDMA gradient fix**: Improved step formula for smoother fade and deferred HDMA activation
  to button press.
- **Asset pipeline modernization**: All 36 examples now use `res/` subdirectories with gfx4snes
  Makefile rules for reproducible asset conversion from source PNGs. Removed tracked generated
  files (`.inc`, `_data.as`, `.pic`, `.pal`, `.map`) — these are now built at compile time.
- Converted all remaining BMP source assets to PNG.
- Added missing `.inc`/`_data.as`/`_meta.inc` entries to clean rules.

### Documentation

- Comprehensive README rewrite for all 36 examples with consistent formatting, hardware
  explanations, and build instructions.
- Added Mesen2 screenshots for key examples (breakout, likemario, collision_demo, etc.).

### Devtools

- Reorganized `devtools/` into one directory per tool, each with its own README.

### Housekeeping

- Removed unused project templates.
- Removed tracked gfx4snes-generated binary files from the repository.
- Fixed sprite32.pal size and removed unused assets.

## [0.2.0] - 2026-03-03

### Compiler

- **Fixed variable shift codegen** — `(1 << variable)` generated broken code because WLA-DX
  defaulted to 8-bit index registers per object file. Fixed by emitting `.ACCU 16` / `.INDEX 16`
  before each function.
- **Fixed variable shift stack offset** — `emitload_adj` now correctly adjusts +2 after `pha`
  in the shift loop.
- **-22% runtime cycles** vs PVSnesLib + 816-opt on benchmark suite (improved from -31% in v0.1.0)
- Added composite constant multiply (*20, *24, *40, *48, *96)
- Added inline multiply for *11 through *15
- Dead store elimination for inline multiply A-cache compatibility

### Library

- New modules: `map`, `object`, `debug`, `lzss`, `video`
- Mouse and Super Scope input drivers
- Assembly `oamSet()` — C version had framesize=158, causing visible slowdown with >2 sprites
- WAI-based `WaitForVBlank()` — saves CPU power
- Conditional BG scroll writes via `bg_scroll_dirty` bitmask in NMI handler
- NMI callback skip optimization (~260 cycles saved when using DefaultNmiCallback)
- `oamSetFast` macro for zero-overhead OAM writes
- Fixed `mode7Rotate` degree overflow
- Fixed SNESMOD FIFO clear
- Fixed `colorMathSetSource` CGWSEL bit 1 polarity
- Fixed write-only register reads in library code

### Examples

- 36 working examples (up from 25), 11 new:
  - **Games**: mapandobjects (object engine platformer), slopemario (slope collision)
  - **Maps**: dynamic_map (tilemap sprite engine)
  - **Graphics**: metasprite, object_size, mixed_scroll, mode1_bg3_priority, mode1_lz77, hdma_gradient
  - **Input**: mouse, superscope
  - **Basics**: collision_demo (AABB + tile collision)
- Removed variable-shift workarounds (collision_demo, background.c)
- Added GFX conversion rules to all example Makefiles (CI clean-build safe)

### Build System

- Multi-file C compilation support (`CSRC` variable)
- Automatic module dependency resolution in `common.mk`
- Bank $00 free space threshold warning in `symmap.py`

### Testing

- 57 compiler regression tests (up from 54)
- 25 unit test modules
- 36 example validations
- POSIX-compatible test scripts (macOS `grep -oE` instead of `grep -oP`)

## [0.1.0] - 2026-02-16

### Compiler (cc65816 — QBE w65816 backend)

- **-31.3% cycle count** vs PVSnesLib + 816-opt on benchmark suite
- 13 optimization phases: dead jump elimination, A-register cache, 8/16-bit mode tracking,
  leaf and non-leaf function optimization, comparison+branch fusion, dead store elimination,
  tail call optimization, and more
- Fixed unsigned integer promotion (u16 comparisons with values >= 32768)
- Fixed signed right shift (`>>` on negative values)
- Fixed function return values being clobbered by epilogue
- Fixed `__mul16` calling convention mismatch
- Added inline multiply for *3, *5, *6, *7, *9, *10
- String literals placed in ROM (saves WRAM)
- `pea.w` for constant arguments
- `.l` to `.w` address shortening for bank $00 symbols
- `stz` for zero stores to global symbols

### Library

- New modules: `mosaic`, `window`, `colormath`, `collision`, `entity`, `animation`, `sram`, `mode7`
- Modernized API with Doxygen documentation on all public headers
- `consoleInit()` sets sensible BG1 defaults (no extra setup needed for simple programs)
- Fixed CopyInitData 16-bit byte overrun (corrupted initialized static variables)
- HDMA support with direct and indirect tables

### Examples

- 25 working examples covering all subsystems:
  - **Games**: Breakout, LikeMario (platformer with scrolling)
  - **Graphics**: sprites (simple, dynamic, animated), backgrounds (Mode 1, Mode 7,
    Mode 7 perspective, continuous scroll), effects (fading, HDMA wave, gradient colors,
    transparency, window, mosaic)
  - **Audio**: SNESMOD music, sound effects
  - **Input**: two-player joypad
  - **Memory**: SRAM save/load, HiROM demo
  - **Text**: hello world, text formatting

### Build System

- Unified `make/common.mk` for all examples
- HiROM support (`USE_HIROM=1`)
- SRAM support (`USE_SRAM=1`)
- Library module selection (`LIB_MODULES=console sprite dma`)
- CI pipeline on Linux, macOS, and Windows (MSYS2)
- `make release` for SDK packaging

### Testing

- 54 compiler regression tests
- Example validation with memory overlap checking
- Multi-platform CI (Linux, macOS, Windows)

### Documentation

- Doxygen-documented public API
- Example READMEs with hardware explanations
- Progressive learning path from hello world to full games
