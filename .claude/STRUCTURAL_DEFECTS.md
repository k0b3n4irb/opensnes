# OpenSNES — Structural Defects & Chantiers Plan

**Document type**: working planning document — the inventory of defects in the
SDK that **cannot be fixed by a quick win** and require a deliberate
chantier (multi-day to multi-month engineering effort).

**Audience**: maintainer (k0b3n4irb), future contributors, any reviewer
deploying effort against this list. Lives under `.claude/` because it is
maintainer-internal operational planning, not user-facing documentation —
contributors looking at the project's headline state should read
[`KNOWN_LIMITATIONS.md`](../KNOWN_LIMITATIONS.md) and
[`ROADMAP.md`](../ROADMAP.md) at the repo root, both of which carry the
public severity tags and high-level direction. This document is the layer
*beneath* those: effort estimates, interaction matrices, sequencing paths,
investigation logs.

**First created**: 2026-05-08 (originally drafted in `/tmp/` as scratch
during the post-audit chantier wave; promoted to a permanent
maintainer-internal doc on 2026-05-09 once the catalogue was demonstrably
load-bearing for cross-session continuity).

**Baseline**: external review delivered 2026-05-07; 18 commits shipped on
`develop` between 2026-05-07 and 2026-05-08 closing 9 / 10 of the Top 10
audit items + the tutorials wave (8 / 8) + the cycle-count CI gate (soft
mode). The items in this document are **what remained** after that
response wave — the items where the audit / response cycle established
that no quick win exists.

**Status**: living document. Update as items move (in-progress, partial,
done, scope-changed). Do **not** treat as immutable: as upstream `cproc` /
`qbe` evolves, as SNES homebrew tooling evolves, the cost of each item
changes. Re-cost before deploying real effort.

---

## Table of contents

1. [Executive summary](#1-executive-summary)
2. [Methodology — what's in, what's out](#2-methodology)
3. [Defect catalogue](#3-defect-catalogue)
   - [Category A — Compiler stack (cproc / QBE)](#category-a--compiler-stack-cproc--qbe)
   - [Category B — Library API surface](#category-b--library-api-surface)
   - [Category C — Code organisation & maintenance burden](#category-c--code-organisation--maintenance-burden)
   - [Category D — Toolchain & emulator coverage](#category-d--toolchain--emulator-coverage)
   - [Category E — Runtime / NMI / hardware-level](#category-e--runtime--nmi--hardware-level)
4. [Interactions matrix](#4-interactions-matrix)
5. [Difficulty classification](#5-difficulty-classification)
6. [Strategic sequencing — three deployment paths](#6-strategic-sequencing)
7. [Acceptance criteria summary](#7-acceptance-criteria-summary)
8. [How to use this document](#8-how-to-use-this-document)
9. [References](#9-references)

---

## 1. Executive summary

This document inventories **18 structural defects** in the OpenSNES SDK
that the 2026-05-07 external review and the subsequent 2026-05-08 response
work surfaced as **not solvable by a quick win**. They cluster into five
categories:

- **Category A — Compiler stack** (5 items). The cproc + QBE fork has
  fundamental type-size and codegen issues that cannot be patched with
  surface-level workarounds. Fixing any of them touches the
  `compiler/{cproc,qbe}` submodules and risks the 28-patch QBE divergence
  growing further.

- **Category B — Library API surface** (6 items). The SDK's C-side API
  uses 2-byte pointers (matching the 65816's native short pointer), which
  forces user data and lib helpers into bank `$00`. Several items in this
  category share the same root cause: extending the API to 24-bit pointer
  variants is a 2–3 week lib + audit + migration chantier.

- **Category C — Code organisation** (2 items). 5 modules in `lib/source/`
  are 100 % assembly (≈ 5 478 LOC); two of them (`sprite`, `text`) carry
  parallel C and ASM implementations. Bus factor + benchmark-driven
  consolidation.

- **Category D — Toolchain & emulator** (3 items, one intrinsic). snes9x
  doesn't detect the GSU chip in OpenSNES's SuperFX ROM headers; SA-1's
  SIWP register init is an unsourced assumption never validated on
  hardware; SuperFX has no C compiler (intrinsic to the chip's RISC ISA,
  not fixable).

- **Category E — Runtime / NMI / hardware-level** (2 items). The WRAM
  data port `$2180–$2183` race on NMI is documented but only enforced by
  convention; the cycle-count CI gate just shipped in soft mode and a
  hard-gate design is its own chantier.

**Total estimated effort to address all addressable items** (excluding
intrinsic D3): **6–9 person-months** spread across compiler, lib, ASM
audit, hardware validation, and CI engineering. The items have
**non-trivial interactions** (see §4): fixing B2 in isolation gives
limited benefit; fixing A1 might break invariants the 28 QBE patches rely
on; fixing C2 unblocks understanding of how C1 should be sliced.

**Strategic guidance** (detail in §6):

- The **lib API extension** cluster (B1+B2+B3+B4) gives the largest
  user-facing unlock per unit of effort; it's also the safest because no
  example currently relies on the 16-bit-only pointer ABI for correctness
  (only for "happens to fit in bank `$00`").
- **A1 (`int = 32 bits`)** is the highest-impact compiler item but also
  the riskiest — every patched optimisation in QBE may have implicit
  assumptions about IR int width.
- **D1 (Mesen2 mandatory)** is the cheapest meaningful item in the list
  (3–5 days) and closes a real validation gap for shipped chip-using
  examples.

Items are **not gates** on each other in the strict sense — most can be
worked in any order. The recommended sequencing in §6 optimises for
**risk reduction first**, then **adoption unlock**, then **maintenance
hygiene**.

---

## 2. Methodology

### What's in

A defect qualifies for this catalogue when **all three** are true:

1. **It causes wrong behaviour, lost performance, or maintenance burden**
   in current shipping state. Documented mitigations don't disqualify —
   the question is whether the underlying class of bug / friction
   persists.
2. **It cannot be fixed in less than a day** of focused work. Single-PR
   fixes go to the standard issue tracker, not here.
3. **It has been confirmed during the 2026-05-07 / 2026-05-08 audit and
   response cycle.** Speculative future concerns are out — this is the
   list of things we have actually seen.

### What's out

- **Hardware constraints** (VBlank DMA budget, OAM Y +1, bank `$00` 32 KB
  cap, double-write registers). These are 65816 / SNES PPU realities; no
  amount of OpenSNES engineering eliminates them. Documented in
  `KNOWN_LIMITATIONS.md`.
- **Documentation gaps** addressed by the 8-tutorial wave shipped
  2026-05-08 (HDMA, Mode 7, DMA, Window, Colormath, SRAM, Mosaic, Math).
- **Coverage gaps** of the example library (single-example modules,
  missing patterns like BRR streaming). These are content gaps, not
  structural defects.
- **Project-process items** (release flow, CHANGELOG discipline,
  contributor onboarding). Out of technical scope for this document.

### How items were identified

- The 2026-05-07 external review (held privately, summary lives in this
  catalogue and in CHANGELOG entries from the response wave) flagged
  most of the items as Top-10 risks or in the body of the report.
- The 2026-05-08 response work surfaced new items by depth-investigating
  some Top-10 items (e.g., the bank `$00` refactor wave that turned out to
  be structurally constrained by the 16-bit pointer ABI rather than by
  user-side const array sloppiness).
- A few items come from `KNOWN_LIMITATIONS.md` and `compiler/PINS.md`
  cross-references that the audit confirmed are genuine structural
  issues, not just documentation entries.

### What this document is NOT

- A backlog. Items here are **candidates** for a chantier; the maintainer
  decides which to deploy and when. A backlog would have prioritisation
  embedded; this document presents data for the decision.
- A bug tracker. Use GitHub issues for individual bug reports. This
  document is for the cross-cutting / structural class.
- A roadmap. The ROADMAP file owns release-aligned planning. This
  document feeds into ROADMAP planning when items become committed
  chantiers.

---

## 3. Defect catalogue

### Category A — Compiler stack (cproc / QBE)

The custom `cproc + QBE w65816` toolchain is OpenSNES's strongest
differentiator from PVSnesLib (modern C11 frontend, SSA optimisations,
documented ABI). It is also the source of the deepest structural defects.
The fork burden is real (`compiler/cproc` carries 6 patches,
`compiler/qbe` carries 28); each item below either makes that burden
worse or is gated by it.

#### A1. `int = 32 bits`, `long = 64 bits` on cc65816 🔴

> **Status (2026-05-08): PARTIAL — `int=2, long=4` shipped; pointer change
> deliberately separated into a new chantier A6.** The empirical investigation
> revealed that combining the int/long change with the pointer change (the
> original Q1 Option B "all-at-once" plan from 2026-05-08 session) cascades
> catastrophically through QBE w65816's indirect-call emit pass. Splitting
> was forced by reality, not preference. See A6 below for the deferred
> pointer chantier.

**Symptom**: code using bare `int` or `long` produces 32-bit / 64-bit
arithmetic on a 16-bit CPU, doubling cycles and RAM. Particularly bites
ports from PVSnesLib (whose tcc816 has 16-bit `int`) and newcomers from C
who reflexively type `int counter`.

**Root cause**: cproc's IR was designed as a host-target compiler.
`sizeof(int) = 4` is hard-coded into the type system layer. The QBE
backend inherits this — every `int` operation generates 32-bit IR ops,
which the w65816 emit pass lowers as two 16-bit operations.

**Current mitigations**:
- `KNOWN_LIMITATIONS.md` documents the trap.
- `compiler/ABI.md` calls it out in the type-size table.
- `lib/include/snes/types.h` defines `u8`, `u16`, `s16`, `u32`, `s32` as
  the canonical types; the lib uses these everywhere.
- The new doc-drift sentinel (commit `e7e7301`) doesn't cover this — it's
  a discipline issue, not a doc-sync issue.

**Proposed fix**: modify cproc's type-size layer (or apply an IR-level
rewrite in the cproc → QBE bridge) to make `sizeof(int) = 2` on the
w65816 target. This requires:

1. Audit cproc's `decl.c`, `type.c`, `expr.c` for hard-coded type sizes.
2. Add a `target.intsize` field to cproc's target descriptor.
3. Rebuild and re-test against the 60 compiler-pattern tests.
4. **Critical**: re-verify the 28 QBE patches don't assume IR int width
   in their codegen logic. Several of the perf-related patches (composite
   constant multiply, A-cache, leaf opt) may have implicit 32-bit
   assumptions in their pattern-match conditions.

**Effort estimate (revised after empirical investigation 2026-05-08)**:
**2–4 hours** for the int+long change (4 lines of code in
`compiler/cproc/type.c`, 1 baseline regen for `basics/random`, full
validation cycle). The original 1–2 person-month estimate was based on
the assumption that the pointer change would land in the same chantier;
with pointers separated into A6, A1 is much smaller.

**Empirical findings (2026-05-08)**:
- **`int=2, long=4` alone**: 264 / 265 tests pass. The 1 failure
  (`basics/random` visual, 162 px diff) is timing-driven baseline drift
  — RNG seeded by `frame_count`, gameplay state at frame 120 differs
  but the rendering pipeline is correct. Same class as the WaitForVBlank
  baseline drift earlier (commit `13415ec` in opensnes-emu).
- **`int=2, long=4` + pointer=2** (original Q1-Option-B plan):
  254 / 263 tests pass with **9 real failures**. The cascade: function
  pointers in `static const Scene` / `static const GameLoopConfig` lose
  their bank byte. QBE w65816's indirect-call emit (in
  `gameloop.c.asm`, `scene.c.asm`, etc.) hardcodes
  `lda #$00; sta.b tcc__r9+2` as the bank byte for `jml [tcc__r9]`,
  which only works if all dispatched functions live in bank `$00`. With
  the lib's SUPERFREE'd functions free to land anywhere, the framework
  opt-ins (`gameloop`, `scene`) crash on every dispatch. See A6 for the
  proper fix.

**Dependencies / interactions**:
- A6 is the natural follow-on chantier (the pointer cascade discovered
  during A1's investigation).
- This commit alone does not invalidate any of the 28 QBE patches —
  empirically verified by the test suite passing 264/265.
- `compiler/PINS.md` does NOT need a re-numbering of patches; only the
  cproc fork gains one new patch (so 7 cproc patches instead of 6).
- Could surface latent bugs in user code that currently "works" with
  32-bit `int` but assumes 16-bit semantics (unlikely but possible).

**Acceptance criteria**:
- `sizeof(int) == 2` and `sizeof(long) == 4` on w65816 target.
- Test suite passes 265/265 in `--quick` mode.
- `--allow-known-bugs` flag still tolerates the same 5 known-bug entries
  (no new ones introduced).
- Compiler-bench (`tools/opensnes-emu/test/run-benchmark.mjs`) shows
  total cycle delta ≤ +5 % (some patches will need re-tuning; some user
  code will be faster).
- All 54 examples build and pass visual regression.
- `compiler/PINS.md` updated with the new merge-base and the patch
  re-numbering.
- `compiler/ABI.md` updated to reflect the new type sizes.

**References**:
- `KNOWN_LIMITATIONS.md` "Type & ABI gotchas" section.
- `compiler/ABI.md:303-314` (type sizes table).
- 2026-05-07 audit § 5 (Compiler stack).

---

#### A2. `volatile` is preserved through QBE — SHIPPED 2026-05-09 🟢

> **Status**: shipped 2026-05-09. The original symptom in this entry —
> "QBE backend assertion failure or silently broken assembly output" —
> turned out to be the second mode (silent miscompile, not assertion):
> cproc was discarding the `QUALVOLATILE` qualifier with a literal
> `(void)(tq & QUALVOLATILE);` and a `TODO` comment, so QBE's loadopt
> pass coalesced redundant loads of the same volatile through value-
> numbering, violating the C standard's "each access is a side effect"
> volatile semantics.
>
> **Root cause discovered**: not an SSA-pass bug at all — cproc never
> emitted volatile information into the QBE IR in the first place,
> so QBE was free to optimise away duplicate loads as redundant.
>
> **Fix**:
>
> - **cproc** (`compiler/cproc/qbe.c`): replaced the `(void)(tq & ...)`
>   discard with a real `is_volat` branch in `funcload` and `funcstore`.
>   New `funcinst_volat()` wrapper around `funcinst()` flags the just-
>   emitted instruction. The IR printer (`emitinst()`) prepends `volat`
>   between the result class and the op for loads (`%t =w volat
>   loaduh ...`) and at the line start for stores (`volat storeh ...`).
>
> - **QBE parser** (`compiler/qbe/parse.c`): new `Tvolat` token recognised
>   either at line start or after `=cls` in `parseline`. Sets
>   `is_volat=1` for the next emitted Ins.
>
> - **QBE Ins struct** (`compiler/qbe/all.h`): added `uchar volat`
>   AFTER `arg[2]` (not packed into the bitfield) so the existing
>   positional initialisers `(Ins){Op, Cls, To, {Args}}` continue to
>   compile and default the new field to 0.
>
> - **QBE loadopt** (`compiler/qbe/load.c`): the load-forwarding loop
>   now skips loads with `i->volat` set — they aren't candidates for
>   `def()` substitution. The redundant load stays as a real load.
>
> - **QBE promote** (`compiler/qbe/mem.c`): the SSA promotion pass
>   bails out of any alloca whose uses include a volatile load or
>   store. Promoting would convert the memory access into a register
>   copy and lose volatility.
>
> - **QBE gcm** (`compiler/qbe/gcm.c`): `canelim()` returns 0 for
>   volatile loads even when the result temp has no users, so a
>   volatile read is never elided as dead.
>
> - **QBE printfn** (`compiler/qbe/parse.c`): IR text round-trip
>   preserves the `volat` keyword for debugging.
>
> **Validation**: 5 reproducers compiled before the fix
> (`/tmp/a2_repro{1..5}.c` — kept ephemerally for the investigation):
>
> - Case 1 (read polling): `while (*reg & 0x80) { }` — already OK
>   pre-fix (no dedup pattern).
> - Case 2 (global flag): `while (flag) { }` — already OK.
> - Case 3 (write loop): `for (...) *port = 0;` — already OK.
> - Case 4 (read should not hoist): `while (!ready) {...}` — already OK.
> - **Case 5 (read twice, the actual bug)**: `return timer + timer;`
>   was `lda timer; sta cache; clc; adc cache` (1 load, 1 cached);
>   post-fix `lda timer; sta cache; lda timer; clc; adc cache`
>   (2 loads as required by C).
>
> Test fixture `tools/opensnes-emu/test/fixtures/compiler/test_volatiles.c`
> already existed but only checked compilation succeeded; strengthened
> in `tools/opensnes-emu/test/phases/compiler-tests.mjs` to assert
> `lda hw_status` count ≥ 3 in `test_volatile_read`, `sta hw_data`
> count ≥ 4 in `test_volatile_write`, and the read/write counts in
> `test_read_modify_write`. Full suite 409/409 PASS post-fix.
>
> **Doc updates** (KNOWN_LIMITATIONS.md, CLAUDE.md, .claude/rules/
> compiler.md, .claude/rules/new_example.md, compiler/ABI.md): the
> historical "volatile crashes QBE / use globals" warnings replaced
> with "volatile is honoured; SDK still favours globals for NMI
> handshakes for cycle-cost equivalence, not because volatile is
> broken".



**Symptom**: a C loop body that reads or writes a `volatile`-qualified
variable (typical pattern for hardware register polling) produces either
a QBE backend assertion failure or silently broken assembly output.

**Root cause**: QBE's SSA pass cannot always handle the `volatile`
qualifier's no-rewrite semantics. The pass's value-numbering and dead
store elimination phases assume rewriteability that `volatile` forbids.

**Current mitigations**:
- The lib uses plain global variables (without `volatile`) for handshake
  patterns: `vblank_flag`, `oam_update_flag`. This is documented in
  `templates/crt0.asm` and reflected in the API contracts.
- `KNOWN_LIMITATIONS.md` documents the crash and the workaround.
- For genuine MMIO semantics, users write a small assembly helper
  (per `compiler/ABI.md` "Calling assembly from C").

**Proposed fix**: patch QBE's SSA pass to handle `volatile` correctly —
typically by tagging volatile loads/stores as "do not eliminate" /
"do not reorder" markers that the optimisation passes respect.

**Effort estimate**: **1–2 weeks**. The actual patch is small once the
right invariant is identified, but understanding QBE's SSA pass
internals takes 3–5 days first. Submit upstream to QBE.

**Dependencies / interactions**:
- Independent of A1, A3, A4.
- Adds a 29th patch to `compiler/qbe`. The PINS.md discipline absorbs
  this without issue.

**Acceptance criteria**:
- A regression test with `volatile u8 *reg = ...; while (*reg) { ... }`
  pattern compiles without error and produces correct (if not optimal)
  assembly.
- Existing `vblank_flag` handshake patterns continue to work
  (regression test must include both the workaround pattern and the
  new direct `volatile` pattern).
- Patch upstreamed (or at least submitted) to QBE master.

**References**:
- `KNOWN_LIMITATIONS.md` "🟠 `volatile` in loops crashes QBE".
- `compiler/ABI.md:250-252`.

---

#### A3. TCO `[KNOWN_BUG]` residual — RESOLVED 🟢 (2026-05-09)

> **Status**: shipped 2026-05-09 by retiring the stale markers, not
> by extending the optimisation. Investigation in chantier A3
> compiled each test fixture and inspected the generated ASM:
> every optimisation the `[KNOWN_BUG]` markers gated had already
> shipped (TCO for frameless wrappers via C.1 / C.2.1 / C.2.2;
> A-cache through `pha` audited as working in C.6; lazy `rep #$20`
> emission active; `leaf_opt=1` marker correctly emitted for
> `forward_with_work`). The test assertions were drifting behind
> the codegen, not anticipating future work.
>
> The `compute_across_call.*leaf_opt=1` assertion in
> `nonleaf_frameless` was actively wrong — that function MUST get
> `leaf_opt=0` because `sum` is live across the call (SSA
> liveness rules; see `all_calls_are_tail()` gate at
> `compiler/qbe/w65816/emit.c:2882`). Rewrote it to check
> `forward_with_work.*leaf_opt=1` instead (the case where the
> optimisation legitimately applies).
>
> Concrete deliverables:
> - `tools/opensnes-emu/test/phases/compiler-tests.mjs` — 7 stale
>   `knownBug()` calls converted to `fail()` with refreshed
>   comments. MSYS2-segfault paths kept (orthogonal, Windows-only).
> - `.github/workflows/opensnes_build.yml` — `--allow-known-bugs`
>   removed from the test-suite step.
> - `.claude/rules/release.md` — pre-release checklist updated.
> - `KNOWN_LIMITATIONS.md` — "Compiler optimisation gaps" section
>   replaced with a "None as of A3" callout + MSYS2 note.
> - `ROADMAP.md` — TCO entry rewritten as shipped, "Retire residual
>   `[KNOWN_BUG]`" item closed, post-A1 type-size headline fixed.
>
> Test-suite signal: `node test/run-all-tests.mjs` (no flag) is
> 404/404 PASS. Real regressions on TCO / A-cache / lazy rep now
> hard-fail.



**Symptom**: tail call optimisation works for "frameless" leaf functions
(no local variables), correctly emitting a `jml` instead of `jsl + rtl`.
For non-frameless cases (locals on the stack, framed return), the test
suite still flags 3–4 cases as `[KNOWN_BUG]`. CI runs with
`--allow-known-bugs` to tolerate them.

**Root cause**: the QBE w65816 emit pass for TCO covers the easy
case (no frame to unwind) but not the general case (need to deallocate
the local frame before the tail jump, plus handle parameter
re-marshalling if the tail target has a different argument layout).

**Current mitigations**:
- Frameless TCO ships per `compiler/PINS.md:69-70` (`ed840fb`,
  `bab0164`).
- The 3–4 `[KNOWN_BUG]` test cases are tolerated by
  `--allow-known-bugs`.
- ROADMAP "Next steps" tracks this as the residual cleanup.

**Proposed fix**: extend the QBE w65816 TCO emit pass to handle the
framed case. This requires:

1. Analyse the frame layout and confirm we can unwind cleanly before
   the `jml`.
2. Handle parameter re-shuffling when the tail target has a different
   parameter set.
3. Add the 3–4 patterns to the compiler test suite as positive cases
   (no longer `[KNOWN_BUG]`).
4. Drop `--allow-known-bugs` from CI.

**Effort estimate**: **1–2 weeks**. The hard part is the frame-unwind
sequencing, which has overlap with the function epilogue's existing
`tax/tsa/adc/tas/txa` dance. Risk of subtle stack pointer bugs.

**Dependencies / interactions**:
- A4 (oamSet framesize) is in the same family — both are about how QBE
  manages the stack frame for cc65816's specific calling convention.
  Doing them in sequence is sensible: the audit work for A3 informs A4.
- Closes part of `--allow-known-bugs`. After A3 + A5 (leaf_opt cosmetic
  marker) are done, `--allow-known-bugs` can be dropped from CI.

**Acceptance criteria**:
- All 5 currently-tolerated `[KNOWN_BUG]` compiler tests pass without
  the flag.
- CI runs `node test/run-all-tests.mjs` (no `--allow-known-bugs`) green.
- Cycle-count benchmark shows non-frameless tail-call patterns
  improved (target: -10 to -30 % on the relevant `bench_functions.c`
  entries).
- `KNOWN_LIMITATIONS.md` and `ROADMAP.md` updated to reflect closure.

**References**:
- `compiler/PINS.md:57-77`.
- `KNOWN_LIMITATIONS.md` "Compiler optimisation gaps".
- `ROADMAP.md` "Next steps" (post-2026-05-08 reconciliation).

---

#### A4. `oamSet` framesize=158 perf cliff — RESOLVED 🟢 (2026-03-03)

**Status update**: shipped via tactical ASM rewrite (commit `39dbff8`,
2026-03-03) — NOT the QBE-coalescer chantier originally proposed below.
The ASM `oamSet` in `lib/source/sprite_oamset.asm` has framesize=0
(direct stack-relative addressing, no SSA temps), saving ~100 cycles
per call vs the C version. The acceptance criterion "framesize drops
from 158 to ≤ 32 bytes" is met (framesize is 0).

The broader perf-cliff observation surfaces in other multi-arg C
helpers (`oamSetX` 148, `oamDrawMeta` 142, `oamDrawMetaFlip` 200,
`collideRectEx` 176, `hdmaColorGradient` 162). A 2026-05-13 audit
found that only `oamSetSize` (106) has multiple example callers (3);
the rest are 0-1 callers, mostly demo/utility paths. The cliff
persists but no longer affects per-frame hot paths in shipping
examples — the priority dropped from 🟠 to 🟢. Future tightening
(QBE coalescer chantier OR per-function ASM rewrites) is opportunistic,
not blocking.

The historical description of the original symptom and proposed fix
is preserved below for context.

**Symptom (historical, pre-2026-03-03)**: the `oamSet(id, x, y, attr,
size, tile)` library function allocates **158 bytes of SSA temporaries
on the stack per call**. The function itself is correct; the cost
lives in stack manipulation. Calling `oamSet` more than ~3 times per
frame in the main loop produces visible jitter on real hardware.

**Root cause**: QBE's register allocator and SSA-temp lifetime analysis
assign large numbers of stack slots for `oamSet`'s 6-argument body. The
function is simple enough that the temps are mostly redundant, but the
allocator doesn't see it.

**Current mitigations**:
- `oamSetFast(id, x, y, attr, size, tile)` and `oamSetXYFast(id, x, y)`
  macros expand to direct `oamMemory[]` writes — zero function-call
  overhead.
- `KNOWN_LIMITATIONS.md` "Performance traps" section (added in commit
  `4b31167`) documents the trap and the mitigation.
- 5 example READMEs (breakout, collision_demo, animated_sprite, mouse,
  superscope) show the direct-write pattern in production form.

**Proposed fix**: improve QBE's register allocator / SSA-temp coalescing
for cc65816. Specific improvements:

1. **Pre-emptive coalescing**: detect SSA temps that have non-overlapping
   lifetimes and merge them into single physical slots.
2. **Aggressive register reuse**: prefer `tcc__r*` direct-page slots over
   stack slots for short-lifetime values.
3. **Function-shape heuristics**: detect "many small parameters → one
   sequential write" pattern (canonical for OAM-style helpers) and emit
   a tight inline.

**Effort estimate**: **2–4 weeks**. This is genuine compiler engineering
— the kind of work that yields wide perf improvements (every helper
function with similar shape benefits) but is hard to scope. Touching the
allocator risks regressions on the 28 patches that already exist.

**Dependencies / interactions**:
- A3 (TCO non-frameless) shares concepts (frame management) but addresses
  different code paths.
- If A4 lands cleanly, the `oamSetFast` macros become less necessary.
  They'd remain in the API for explicit "I want the macro form" intent
  but the perf gap closes.
- Likely surfaces additional perf wins on `dmaCopyVram`, `bgInitTileSet`,
  and other multi-arg helpers.

**Acceptance criteria**:
- `oamSet` framesize drops from 158 to ≤ 32 bytes.
- Cycle benchmark shows a measurable improvement on the
  `bench_functions.c` entries that involve multi-arg lib helpers.
- `KNOWN_LIMITATIONS.md` "Performance traps" section updated to reflect
  the fix; the `oamSetFast` macros stay (documented as the explicit
  zero-overhead form).
- No regression on existing 28 QBE patches' benchmark deltas.

**References**:
- `KNOWN_LIMITATIONS.md` "Performance traps" 🟡 entry (commit `4b31167`).
- `lib/include/snes/sprite.h` "Fast Macro Sprite API" section.
- 2026-05-07 audit § 6 (Library API), § 14 (Performance story).

---

#### A5. Compiler stack divergence (28+ QBE patches, 6 cproc patches) 🟡

**Symptom**: every sync with upstream `cproc` or `qbe` requires a
non-trivial rebase of the local patch stack. The maintainer-facing
discipline is documented in `compiler/PINS.md` ("Updating a pin"
section), but the underlying cost grows over time.

**Root cause**: the SDK's compiler differentiation (the 30 % vs PVSnesLib
benchmark, the named cycle wins) lives in those patches. Upstream cproc
and QBE evolve on their own roadmaps (which are entirely orthogonal to
SNES needs). Each upstream change risks merge conflicts; each local patch
risks needing a port.

**Current mitigations**:
- `compiler/PINS.md` enumerates every patch with rationale.
- `make verify-toolchain` enforces pin discipline (no silent drift).
- Patches are atomic commits, well-named, with `git log --oneline`-grade
  one-liners.
- The audit (2026-05-07) confirms this is a strength AND a maintenance
  bomb — both are true simultaneously.

**Proposed fix**: gradual upstreaming. Each patch is evaluated for
upstream submission:

1. Generic improvements (bug fixes, generic optimisations) → submit
   upstream, contribute to project, reduce local burden.
2. SNES-specific improvements (w65816 codegen patterns) → submit to QBE
   maintainers as backend-specific contributions.
3. Hard-to-upstream patches (those that depend on local cproc patches,
   or those that change shared invariants) → keep local but document
   why upstreaming is hard.

**Effort estimate**: **3–6 months** of part-time work, spread across
multiple chantiers. Cannot be a single push because upstream review
cycles take weeks per patch.

**Dependencies / interactions**:
- Mostly independent of other items, but A1, A3, A4 each add or modify
  patches that should be considered for upstreaming as part of their
  individual chantiers.

**Acceptance criteria**:
- Half (or more) of the 34 current patches accepted upstream; SDK pins
  bumped to the post-merge commits.
- The remaining patches documented as "intentionally local" with
  rationale.
- `compiler/PINS.md` updated to a clean list.

**References**:
- `compiler/PINS.md:57-77` (current patch list).
- 2026-05-07 audit § 5 (Compiler stack — "moat AND maintenance bomb").

---

#### A6. Pointer IR size + indirect-call bank-byte preservation 🔴

> **Status (2026-05-08): identified during A1 investigation, not yet
> attempted.** This chantier exists because A1's empirical investigation
> revealed that changing pointer IR size in cproc cascades catastrophically
> through QBE w65816's indirect-call emit pass. Splitting it out of A1 was
> a forced choice driven by the test-suite signal (9 failures with pointer=2,
> 1 baseline drift with pointer kept at 8).

**Symptom**: cproc's IR has pointers at **8 bytes**
(`compiler/cproc/type.c:74`, the original `mkpointertype()` size). On a
target where pointers are actually 24-bit (3 bytes — bank byte + 16-bit
address), this is wrong. Three concrete consequences:

1. **`static const` structs containing pointer fields have wrong field
   offsets.** The historical `cproc_static_pointer_struct_bug.md` issue
   in maintainer memory (fixed in v0.16.0 chantier C.5 by padding DL/DW
   init data) is exactly this: cproc lays out 8-byte fields but WLA-DX
   emits `.dl symbol` (3 bytes), so reads past the first pointer return
   garbage. C.5 papered over the symptom, not the root cause.
2. **Wasted RAM/ROM**: every pointer field in a struct allocates 8
   bytes when 3 (or 2 with implicit bank) would suffice.
3. **Indirect-call emit hardcodes bank `$00`**: when QBE w65816 emits
   `jml [tcc__r9]` for a function-pointer call, it hardcodes
   `lda #$00; sta.b tcc__r9+2` — the bank byte is never read from the
   pointer storage. This works *today* only because pointers are stored
   as 8 bytes (low 16 bits + high 16 bits + 4 bytes padding) and the
   high half happens to be zero from initialisation. **Reduce pointer
   storage to anything < 3 bytes and the bank byte is genuinely lost,
   breaking every indirect call** to functions outside bank `$00`.

**Root cause**: cproc's IR was designed for 64-bit hosts where pointers
are 8 bytes. The QBE w65816 backend was never updated to emit a
*proper* 24-bit pointer load/store sequence — instead, it relies on the
8-byte storage incidentally containing the bank byte (or zero) at a
known offset. The 28 QBE patches grew around this assumption.

**Current mitigations**:
- C.5 padding fix (chantier C.5, v0.16.0): pads `.dl symbol` data
  emissions with `.dsb N, 0` to match cproc's 8-byte field layout.
  Side-effects: extra zero bytes in const ROM, wasted ROM space.
- 8-byte pointer storage at runtime preserves the bank byte by accident
  (high half is initialised to zero from `mkpointertype()`'s
  zero-extension; for function symbols actually placed in bank `$00`,
  the zero-byte bank is correct).
- Discipline: don't put functions outside bank `$00`. Today, the lib's
  SUPERFREE'd functions can land anywhere; this works *only because*
  the test suite happens not to exercise function pointers to bank
  `$01+` functions. A1's investigation showed the cascade as soon as
  pointer storage shrinks below 3 bytes.

**Proposed fix**: 24-bit pointers throughout the compiler stack.
Specifically:

1. **cproc**: `mkpointertype()` sets `t->size = 4, t->align = 2`
   (4 bytes = 24-bit address + 1 byte alignment padding; align = 2
   matches the target's 16-bit word alignment). Why 4 not 3:
   - 3-byte fields are awkward in C struct layout and break standard
     alignment intuitions.
   - 4-byte alignment matches cc65816's existing 32-bit `int`
     conventions before A1.
   - The high byte (offset 3) becomes a "dead zone" in storage that
     callers/callees can ignore; only bytes 0-2 are read.
2. **QBE w65816 emit**: rewrite the indirect-call sequence in
   `compiler/qbe/w65816/emit.c` to read the bank byte from the
   pointer's storage:
   ```asm
   lda 0,x          ; low 16 bits of fn pointer
   sta.b tcc__r9
   sep #$20
   lda 2,x          ; bank byte (third byte of pointer)
   sta.b tcc__r9+2
   rep #$20
   jml [tcc__r9]
   ```
   Replaces the current hardcoded `lda #$00`.
3. **Static-data emission**: cproc's `qbe.c` data-init path needs to
   match — emit pointer fields as `.dl symbol` (3 bytes) followed by
   `.dsb 1, 0` padding (1 byte) to fill the 4-byte slot. C.5's
   approach scales here.
4. **Audit the 28 QBE patches**: any that read pointer fields by
   offset must be re-validated. Specifically the dynamic_flush_hook,
   nmi_callback, and other indirect-call sites in `templates/crt0.asm`
   need to be re-examined — they are hand-written assembly and may
   have implicit assumptions about pointer layout.
5. **Lib audit**: every place that sets a function pointer in
   `oamInitDynamicSprite`, `nmiSet`, `sceneRun`, `gameLoopRun`, etc.
   The setters (C-side) emit the new 4-byte layout; the readers (ASM
   in crt0 + lib) read the new layout.

**Effort estimate**: **2–4 weeks**. Breakdown:
- Week 1: design + cproc change + emit rewrite + cproc IR change.
- Week 1–2: QBE patch audit (the 28 patches). Each needs to be
  re-verified against the new 4-byte pointer layout.
- Week 2–3: lib + crt0 audit and adjustments.
- Week 3: test suite full validation, baseline regen, Mesen2 spot-check
  on chip ROMs (function pointers + SA-1 / SuperFX interactions).
- Week 4: docs + ABI update, stabilisation.

Risk profile: **High**. Touches the same emit code that had the C.5
bug; touches the indirect-call sequence used by every framework opt-in;
forces a rebuild of the maintainer's mental model of pointer
representation in the toolchain.

**Dependencies / interactions**:
- A1 (already shipped partial) is a prerequisite — the empirical
  ground truth for "what `int=2` looks like in practice" informs
  pointer choice.
- B1 / B3 / B4 (lib API 24-bit pointer extension) become *unnecessary*
  if A6 lands cleanly: the compiler emits proper 24-bit addressing
  automatically, no need for `*Bank` variants in the lib API. The
  benefits of B1 (bank `$00` headroom for the 6 tightest examples)
  apply by default after A6.
- Affects E1 (NMI lint for `$2180-$2183`): function pointers reachable
  from `nmiSet` callbacks have their bank byte preserved properly,
  enabling cleaner static analysis.
- Touches the same QBE emit code modified by the C.5 patch; risk of
  regression on `static const Struct{u8 *p;}` patterns (the v0.16.0
  asset / scene framework structs).

**Acceptance criteria**:
- `sizeof(void *) == 4` in cproc IR; `sizeof(struct { void (*f)(void); }) == 4`.
- Test suite 265/265 in `--quick` (full test suite; no `--allow-known-bugs`
  for indirect-call cases).
- All 4 SA-1 / SuperFX examples pass Mesen2 visual regression
  (function pointers across banks are exercised by chip code).
- `templates/crt0.asm` indirect-call sites (`nmi_callback`,
  `dynamic_flush_hook`) updated and verified.
- B1 / B3 / B4 closed as "subsumed by A6" in this catalogue.
- C.5 padding fix can be reverted (its raison d'être disappears with
  matched cproc/emit pointer sizes).
- `compiler/ABI.md` pointer-size table updated.
- `lib/include/snes/scene.h` and `gameloop.h` field-stability notes
  updated to reflect new 4-byte pointer field size.

**References**:
- `compiler/cproc/type.c:74` (current `mkpointertype()` `t->size = 8`).
- `compiler/qbe/w65816/emit.c` indirect-call sequence (where the
  hardcoded `lda #$00` for bank byte lives).
- `lib/build/lorom/gameloop.c.asm` — the empirical evidence of the
  bank-byte-hardcoded-zero pattern.
- `examples/basics/timer/main.c.asm:128-148` — empirical evidence of
  the static `Scene` struct layout under cproc's 4-byte-per-pointer
  emit.
- maintainer memory `cproc_static_pointer_struct_bug.md` — the C.5
  workaround story that A6 makes obsolete.
- The A1-investigation 9-failure cascade that forced the separation
  of A6 from A1 (captured during the chantier; no longer kept on
  disk — the symptom and its diagnosis are described in A1's
  "investigation log" entry above).
- v0.16.0 CHANGELOG entry for chantier C.5 (the `.dsb N, 0` padding
  fix).

**Investigation log**:

- **2026-05-09** — Partial implementation attempted (A6.1 + A6.3) and
  reverted. Diff was 163 lines in `compiler/qbe/w65816/emit.c` adding:
  (a) a new `emit_load_bankbyte()` helper, and (b) bank-byte storage
  in the four `Ostorel` branches when the source is a `CAddr` literal
  (`lda.w #:symbol` instead of `lda.w #0`). Indirect call switched
  from hardcoded `lda #$00` to `emit_load_bankbyte(r, fn, ...)`.

  The full test suite was 404/404 green with the partial fix, and
  inspection of `timer.c.asm` confirmed `lda.w #:on_init` correctly
  emitted at the bank-byte position of in-memory struct stores
  (Ostorel CAddr → memory). However, **the fix is structurally
  disconnected**:

  - A6.1 writes the bank byte to the **memory** location of the
    pointer store (e.g. `addr_of_cb + 2` for a stack-allocated local,
    or `struct_field + 2` for a heap struct).
  - A6.3 reads the bank byte from a **stack slot** of the indirect
    call's target Tmp (`slot+2,s`).

  These two locations don't connect. The Tmp consumed by Ocall is
  populated by `Oload` in QBE w65816, which is **16-bit only** —
  there is no `Oloadl` 24-bit-load path. So the bank byte A6.1
  faithfully wrote to memory is silently truncated when `Oload`
  reads only the low 16 bits into the Tmp's slot. A6.3's `slot+2,s`
  read picks up whatever was already on the stack at that offset,
  not the bank byte.

  The 404/404 pass relies on coincidence: stack slot+2 happens to be
  zero in current call patterns (slot allocations are consecutive
  2-byte slots, and the next slot's first byte is often zero on a
  fresh frame). All framework opt-in callbacks (`gameLoopRun`,
  `sceneRun`, `oamFlushDynamicSprites`) live in bank `$00` thanks
  to SUPERFREE placement, so `lda #$00` and `lda slot+2,s` emit
  identical-effect code in current builds. **First time a callback
  lands in a non-zero bank, the fix degrades to undefined behaviour
  rather than the deterministic bank-zero failure of the pre-A6
  baseline.**

  Reverted because the user's "structurelle, fondamentale" principle
  rules out shipping a fix that passes tests by luck. The disconnect
  cannot be repaired without **A6.5** (extend `Oload` to a 24-bit
  load when the result class is Kl, and widen pointer slots from 2
  to 4 bytes so the bank byte has a stable home in the temp slot).
  A6.5 in turn needs a slot-allocator change touching the same code
  paths the C.5 padding chantier hardened, so it should ship as one
  cohesive A6.4+A6.5 bundle rather than incrementally.

- **2026-05-09** — Conclusion: the A6 chantier as originally scoped
  (cproc IR pointer 8→4 + indirect-call bank preservation) requires
  three coupled changes that cannot be split:

    1. **A6.4** — cproc `mkpointertype()` `t->size = 4` (covered in
       Proposed fix #1).
    2. **A6.5** — QBE w65816 `Oload`/`Ostore` of class Kl emit 24-bit
       (3-byte) load/store, and slot allocator widens pointer-typed
       temps from 2-byte to 4-byte slots.
    3. **A6.6** — `templates/crt0.asm` and `lib/source/*.asm` audit
       for any hand-written code that reads pointer fields by hard-
       coded offset; current code assumes either 2-byte (current
       runtime ABI) or 8-byte (cproc IR) layout.

  Without A6.5 + A6.6, A6.1 + A6.3 are placebo improvements. The
  shipping path is the full bundle; no partial credit available.

---

#### A7. QBE w65816 32-bit (`Kl` class) codegen never extended past 16-bit 🔴

> **Status (2026-05-09): identified during B5 investigation, validated
> with a runtime ROM, NOT yet attempted.** This chantier exists because
> chantier A1 made `sizeof(long) == 4` in cproc IR (correct storage
> width) but did NOT extend QBE w65816's emit pass to handle the `Kl`
> instruction class. Every 32-bit arithmetic op silently truncates to
> 16-bit and the project's own test fixture only checks for the
> *presence* of `adc`/`dsb 4` — not for completeness — so no signal
> ever fired.

**Symptom**: any C code using `s32` / `u32` arithmetic on values that
exceed 16 bits silently produces wrong results. The variables have
correct 4-byte storage in WRAM (chantier A1 made `unsigned long == 4
bytes` and the storage initialiser writes both halves), but every
operation reads or writes only the low 16 bits and drops the high
half.

Reproduction (validated runtime, 2026-05-09 — a small SNES ROM that
runs the four cases below and reports OK / BUG per case visually
through the `text` module; the source is ~150 lines of C, recreatable
from the test specification in this entry alone):

| Op | Expression | Expected | Actual |
|---|---|---|---|
| Add | `0xFFFF + 1` | `0x00010000` | `0x00000000` |
| Mul | `0x100 * 0x100` | `0x00010000` | `0x00000000` |
| Shr | `0x12345678 >> 16` | `0x00001234` | `0x00000000` |
| Shl | `0x18000 << 1` | `0x00030000` | `0x00000000` |
| **Lib** | `fixDiv(FIX(1), 5)` | `~0x0033` (0.2) | `0x0000` |

The `fixDiv` row is the most material consequence: it is **shipped
library code** (`lib/source/math.c:fixDiv`). The kernel does
`s32 dividend = (s32)a << 8; return (fixed)(dividend / b);` — which
QBE compiles as a 16-bit `xba; and #$FF00; jsl __sdiv16` sequence
that loses the upper 16 bits of the dividend. Any caller passing a
non-trivial fixed-point dividend gets `0` back, silently. No example
in the current 54 exercises this path, which is why it hasn't surfaced.

**Root cause**: `compiler/qbe/w65816/emit.c` was written with the
implicit assumption that every QBE instruction's class is `Kw`
(16-bit word). The `Kl` (long, 32-bit on the abstract machine) class
is never special-cased. Concrete evidence in the emit pass:

- `Oadd` (line 1409): emits a single `clc; adc operand` — no high-half
  pair, no carry propagation past the low 16 bits.
- `Osub` (line 1440): same, single `sec; sbc`.
- `Omul`: always lowers to `jsl __mul16` (16×16 → 16); there is no
  `__mul32` runtime helper.
- `Oshl` / `Oshr` / `Osar`: emit a single `asl a` / `lsr a` / `ror`
  on the loaded 16-bit value. Shift-by-16 is special-cased to
  `lda value; lda.w #0` — i.e. always returns zero, regardless of
  whether the source had any high bits to recover.
- `Oneg` / `Oxor`: 16-bit only.
- `Oload` / `Ostore` of class `Kl`: load 16, store 16. The other half
  is implicitly zero (load) or unchanged (store).

In practice the only way a 32-bit value's high half ever becomes
non-zero is the storage-init path (a literal initialiser like
`u32 x = 0x12345678` does write both halves). Once the value enters
the arithmetic pipeline, the high half is dropped and never recovered.

**Why A1 shipped without catching this**: the unit test fixture
`tools/opensnes-emu/test/fixtures/compiler/test_u32_arithmetic.c`
checks four things:

1. `result32` is `dsb 4` ✓ (storage size is right)
2. `add32` exists in the assembly ✓
3. `result32+2` is mentioned somewhere in the file ✓ (it is — by
   the per-call `stz.w result32+2` epilogue, not by `add32` itself)
4. `add32` body contains `adc` or `clc` ✓ (one of each, but no
   second `adc` for the high half)

None of these check for *carry propagation*, *high-half load*, or
*runtime correctness*. The test was written as a smoke check ("is
the type 4 bytes wide?") — it never asserted the operations were
complete. Investigation chantier A3 cleaned up other stale `[KNOWN_BUG]`
markers in the same file but didn't tighten this assertion either.

**Concrete impact on shipped code**:

- **`lib/source/math.c:fixDiv`** — broken for any dividend > 127
  (since `(s32)a << 8` truncates). Every example that does
  `fixDiv(FIX(N), M)` for `N > 0.5` gets garbage. Today no example
  hits the path, so the bug is latent.
- **`examples/games/tetris/render.c:169-194`** — uses `(u16)(u32)(ptr_a - ptr_b)`
  patterns to compute size differences. The `(u32)` cast is
  immediately narrowed back to `u16`, so these survive — the truncation
  affects the *intermediate*, but the final cast ignores the upper 16
  anyway. Lucky in practice; fragile by design.
- **No other places** in the current tree use `s32`/`u32` arithmetic
  past 16 bits. The rest of the lib is 16-bit pure (`u16` everywhere
  in DMA / OAM / VRAM addressing).

**Proposed fix**: extend the QBE w65816 emit pass to honour the
`Kl` instruction class. The minimal acceptance set is:

1. **Slot allocator**: `Kl` temps allocate **4-byte slots** instead
   of the current 2-byte default. Touches `compiler/qbe/w65816/abi.c`'s
   slot assignment pass. (Today every slot is 2 bytes regardless of
   class, which is why `slot+2` is the next variable's storage —
   the same flaw that bit chantier A6.)
2. **Loads / stores** (`Oload`, `Ostore` with `K == Kl`): emit a
   pair of 16-bit ops covering both halves. Read low 16 then high
   16 from `(addr)` and `(addr+2)`. Symmetric for store.
3. **Add / sub** (`Oadd`, `Osub` with `K == Kl`): emit the canonical
   carry-aware sequence:
   ```
   lda lo(a); clc; adc lo(b); sta lo(result)
   lda hi(a);      adc hi(b); sta hi(result)
   ```
   (`clc` only on the first add; the second `adc` propagates the
   carry naturally.)
4. **Shifts** (`Oshl`, `Oshr`, `Osar` with `K == Kl`): emit a pair
   sequence that propagates between halves. Shift-by-16 special case
   becomes a *swap* (`lo := hi; hi := 0` for `>>`), not a discard.
5. **Negate / not / xor / and / or** (`Oneg`, `Onop`, `Oxor`, `Oand`,
   `Oor` with `K == Kl`): emit pair sequences. Negate uses the
   `eor #$FFFF; adc #1` carry-aware idiom across both halves.
6. **Multiply** (`Omul` with `K == Kl`): cannot use the existing
   16-bit `__mul16` runtime. Two options:
   - 6.a — Add `__mul32` runtime in `lib/source/runtime.asm` doing
     four 16×16 partial products via the existing 8×8 hardware
     multiplier and combining them into the low 32 bits of the
     64-bit product. Estimated 80–120 cycles per call.
   - 6.b — Inline 4-partial-product expansion at each `mul32` call
     site. Bigger code size but no runtime hop. Probably not worth
     it given how rare 32-bit multiplies are.
   Path (6.a) is the conventional choice.
7. **Divide** (`Odiv`, `Oudiv`, `Omod`, `Ourem` with `K == Kl`):
   add `__sdiv32` / `__udiv32` runtimes. Hardware divider is 16/8
   only, so the helpers do shift-and-subtract long division at
   ~200 cycles. Or fall back to the existing 16-bit divide and
   document that 32-bit divide is slow.
8. **Comparisons** (`Oceqi`, `Ocnei`, `Ocsgti`, `Ocsgei`, etc. with
   `K == Kl`): two-half compare with conditional branch on equality
   first, then on signed/unsigned comparison of the high half, fall
   through to low half. Standard pattern.

**Effort estimate**: **1–2 weeks**. Breakdown:
- Days 1–2: slot allocator widening for `Kl` (touches the same code
  path A6.4 needs — coordinate or do A6 + A7 as one chantier).
- Days 3–6: load / store / add / sub / shift / neg / xor / and / or
  pair sequences. Each has tests in the unit fixture.
- Days 7–10: `__mul32` and `__udiv32` / `__sdiv32` runtime helpers
  in ASM.
- Days 11–12: stronger test fixtures — runtime correctness, not just
  ASM-pattern checks. The new fixture should compute `0xFFFF + 1`
  and assert `result == 0x10000` at runtime, ditto for shift, ditto
  for divide. The current fixture is a smoke check; replace with
  a real correctness suite.
- Days 13–14: lib audit (any `s32` / `u32` use), `fixDiv` /
  `fixLerp` re-validation, doc + tutorial updates, ABI revision,
  CHANGELOG entry.

Risk profile: **High**. Every QBE patch that interacts with
slot offsets must be re-validated against the new 4-byte slot
layout. Same area as the C.5 padding fix and the A6 indirect-call
chantier. Test suite expansion is mandatory — the current pattern
checks would still pass even on a half-fixed implementation, so
A7 needs runtime correctness tests in `tools/opensnes-emu/test/
fixtures/runtime/` (a new directory, since the existing runtime
tests don't currently exercise 32-bit math).

**Dependencies / interactions**:

- **A1** (already shipped) is a prerequisite — A7 only matters because
  A1 made `long == 4 bytes` in cproc IR.
- **A6** (pointer ABI) shares the slot-allocator widening work. If
  A6 lands first, A7's slot work piggy-backs. If A7 lands first, A6
  inherits the widened slots for pointers.
- **B5** (`fixed32` 16.16 lib API) is **blocked by A7**. Investigation
  in B5 surfaced this defect; B5 is on hold until A7 ships. This
  catalogue's B5 entry should reference A7 as a hard dependency.
- **C.5** (`static const Struct{u8 *p;}` padding fix from v0.16.0)
  may need re-evaluation — A7's slot widening could remove the need
  for the padding workaround.

**Acceptance criteria**:

- A new runtime fixture (`tools/opensnes-emu/test/fixtures/runtime/
  u32_arithmetic.c`) executes the four reproduction cases above (and
  10–20 more covering signed/unsigned, comparisons, shifts of arbitrary
  amounts) and asserts the *runtime* values match the expected results.
  The test runs the ROM in opensnes-emu and reads result variables
  from WRAM via the snes9x bridge; it fails the build if any value
  is wrong.
- The QBE 32-bit reproduction ROM (described in this entry —
  rebuild from spec) displays five `OK` rows instead of five `BUG`
  rows. This is the canonical hand-validation. Promote the
  reproduction to a `tools/opensnes-emu/test/fixtures/runtime/`
  test asserting via WRAM at the same time so the CI gate fires
  forever on regressions.
- `lib/source/math.c:fixDiv` is verified working for all `(a, b)`
  with `a` in `[1, 32767]` and `b` in `[1, 256]`. Add a test to the
  unit fixtures.
- `compiler/PINS.md` documents new QBE patches.
- `compiler/ABI.md` "Type sizes" table revised — confirms 32-bit
  arithmetic is now correct, not just storage.

**References**:

- The runtime reproduction ROM described above (5 cases, 4
  synthetic + 1 lib-direct via `fixDiv`); the source is ~150
  lines of C using `<snes/text.h>` and is fully reconstructable
  from the table at the top of this entry. The investigation log
  ran the ROM in opensnes-emu and confirmed all 5 cases land on
  `BUG` on 2026-05-09.
- `compiler/qbe/w65816/emit.c:1409` (Oadd, single 16-bit add).
- `compiler/qbe/w65816/emit.c:1440` (Osub, single 16-bit sub).
- `compiler/qbe/w65816/emit.c:2410` (Oload, 16-bit only regardless of K).
- `lib/source/math.c:92-100` (fixDiv kernel — the live lib bug).
- `tools/opensnes-emu/test/phases/compiler-tests.mjs` "test_u32_arithmetic"
  (the pattern-check that should have been a runtime check).
- `lib/include/snes/types.h:75-90` (post-A1 conditional `s32`/`u32`).
- chantier A1 commit `6eb62e1`.
- chantier A3 commit `270cf31` (prior cleanup of stale `[KNOWN_BUG]`
  markers in the same file — A7 surfaces a *different* gap, not
  caught by A3 because A3 only audited markers that were firing).

---

### Category B — Library API surface

Six items, four of them sharing a common root cause (the 16-bit pointer
ABI). Fixing them together yields larger gains than picking one in
isolation.

#### B1. 16-bit pointer ABI in lib helpers (root cause for §15-#3 residual) 🔴

**Symptom**: 6 examples build with bank `$00` ROM under 100 bytes free;
adding a single string literal to any of them fails the build (caught by
the `BANK0_FAIL_THRESHOLD=16` ratchet shipped 2026-05-08). The ratchet
prevents regression but does not free the underlying margin. User assets
(`mapdata` 25 KB, `tileset` 13 KB, etc.) consume bank `$00` because the
lib's `mapLoad`, `bgInitTileSet`, `hdmaSetup`, etc. accept 2-byte C
pointers and assume implicit bank `$00`.

**Root cause**: per `compiler/ABI.md`, `u8 *` is 2 bytes (matching the
65816's native short pointer for performance). Functions accepting `u8 *`
read source data via 16-bit addressing, implicitly bank `$00`. User assets
that exceed bank `$00`'s 32 KB allowance force a structural conflict: lib
code in bank `$00` + user assets in bank `$00` = no room.

**Current mitigations**:
- `dmaCopyVramBank(src, bank, ...)` and `dmaCopyCGramBank` already exist
  with explicit-bank arguments. They're the right shape; they just don't
  cover all the helpers that need it.
- `BANK0_FAIL_THRESHOLD=16` in `make/common.mk` (commit `e7e7301`)
  catches imminent overflow regressions.
- `.claude/rules/bank0_budget.md` documents the refactor playbook.
- Per-example assembly DMA helpers are the user-side workaround
  (`hdma_gradient`, `mode7_perspective`, etc.).

**Proposed fix**: extend the lib's API surface with 24-bit pointer
variants for every helper that accepts user data. Concrete plan:

1. Audit `lib/include/snes/*.h` for functions accepting `u8 *` /
   `const u8 *` from user data (typical pattern: ROM-resident assets).
   Estimate: 12–15 functions.
2. For each, add a `*Bank` variant (e.g., `mapLoadBank`,
   `bgInitTileSetBank`, `hdmaSetupBank` — the latter already exists).
   The Bank variant takes an extra `u8 bank` parameter.
3. Update the C lib implementations to use 24-bit DMA / load semantics
   when the bank is non-zero; fall back to optimised 16-bit when bank
   is zero (avoid penalising the common case).
4. Re-link the 6 tightest examples with the Bank variants and bigger
   assets located in non-`$00` banks.
5. Bump `BANK0_FAIL_THRESHOLD` in `make/common.mk` to a tighter value
   (target: 256 bytes) once the 6 examples have real headroom.

**Effort estimate**: **2–3 weeks**. The function additions are
mechanical (each is ≤ 30 LOC of C + ASM helper); the audit + migration
+ tightening of the ratchet is the slow part.

**Dependencies / interactions**:
- B2 (C RAM <`$2000`) shares the same root cause but on the RAM side.
  Fixing B1 without B2 is partial — the lib's helpers still can't
  accept user RAM data above `$00:1FFF`. Fixing both together is more
  complete.
- B3 (Mode 7 helper) is one specific instance of the pattern; B1's plan
  should include `mode7LoadGraphicsBank`.
- B4 (`hdmaSetup` auto-bank) overlaps; the right resolution is to
  decide whether to (a) add `hdmaSetupBank` always-required, (b) make
  `hdmaSetup` auto-detect via assembly bridge, or (c) deprecate the
  bank-`$00`-only form.
- C1 (ASM module audit): if `mapLoad`'s bank-aware variant moves logic
  from ASM to C, it overlaps with the `map.asm` audit.
- A1 doesn't directly affect this, but a 32-bit-int change might
  reduce some pressure on bank `$00` if user data structs become
  smaller.

**Acceptance criteria**:
- `lib/include/snes/*.h` has `*Bank` variants for every public function
  accepting `u8 *` / `const u8 *` from user data.
- The 6 tightest examples (`mapscroll`, `likemario`, `mapandobjects`,
  `window`, `tetris`, `continuous_scroll`) have ≥ 256 bytes free in
  bank `$00` after migration.
- `BANK0_FAIL_THRESHOLD` bumped to 256.
- All 54 examples build and pass visual regression.
- `KNOWN_LIMITATIONS.md` updated to reflect the lifted constraint
  (severity drops on the bank-`$00`-overflow entries).
- Per-example assembly DMA helpers (currently in `hdma_gradient`,
  `mode7_perspective`, etc.) replaced by the lib's `*Bank` calls where
  applicable.

**References**:
- `compiler/ABI.md:303-314` (pointer is 2 bytes).
- `lib/include/snes/dma.h` (existing `*Bank` variants).
- `KNOWN_LIMITATIONS.md` "Bank `$00` ROM overflow".
- `.claude/rules/bank0_budget.md` (the ratchet policy).
- 2026-05-07 audit § 8 / § 15-#3 / § 11 (the bank-`$00`-tightness map).

---

#### B2. C RAM forced below `$2000` 🔴

**Symptom**: the templates' memory map reserves `$00:0000–$1FFF` (8 KB)
for C-accessible RAM. Anything above `$2000` is unreachable from C with
short addressing — `sta.l $0000,x` always reads bank `$00`, so a C
variable at `$00:2500` is silently wrong-banked. The available WRAM is
128 KB; only 8 KB is C-accessible.

**Root cause**: cc65816 emits `sta.l $0000,x` (bank-`$00`-implicit
24-bit-LDA-with-zero-bank) for direct-page-eligible RAM accesses. The
emit pass doesn't track which RAM lives in which bank, so it can't emit
the correct 24-bit form for above-`$2000` data.

**Current mitigations**:
- Templates' `memmap.inc` reserves `$00:0000–$1FFF` for C RAM and
  `$7E:0300–$051F` for the OAM buffer (page-aligned mirror).
- Custom `RAMSECTION` users must respect this; documented in
  `KNOWN_LIMITATIONS.md` as a 🔴 silent-corruption mode.
- No automated check (no equivalent of the bank-`$00`-overflow ratchet
  for RAM placement).

**Proposed fix**: teach QBE w65816's emit pass to emit proper 24-bit
addressing for RAM accesses outside `$00`. Specifically:

1. Add a target descriptor for "where can RAM live" (per-bank ranges).
2. When a RAM symbol's bank is non-zero, emit `sta.l $7E:NNNN,x`
   (full 24-bit) instead of the implicit-bank form.
3. Or simpler: make the emit pass *always* emit explicit bank for RAM
   accesses, accepting the small per-instruction cost (~1 cycle per
   access).
4. Update the linker scripts to support RAM allocation outside the
   8 KB band.

**Effort estimate**: **2–3 weeks**. The emit pass change is
straightforward but exhaustive (every RAM access must be checked); the
linker side requires care to avoid breaking existing examples.

**Dependencies / interactions**:
- A1 doesn't directly affect this, but a 32-bit-int change shrinks
  some user data structs and reduces the pressure.
- B1 doesn't share the fix path but shares the user-perception problem
  ("I can't fit my game in this small bank"). Doing them together
  delivers a coherent unblock.
- A4 (oamSet framesize) could be related if the SSA temps spill above
  `$1FFF` in extreme cases, though this hasn't been observed.

**Acceptance criteria**:
- C variables placed at `$7E:2500` (or any RAM address outside the
  8 KB band) read and write correctly via the standard C type system.
- A regression test allocates a struct above `$2000` and exercises
  read/write round-trip.
- The templates' `memmap*.inc` updated to reflect the lifted constraint
  (still recommend `$00:0000–$1FFF` for hot data, but allow above).
- `KNOWN_LIMITATIONS.md` updated.
- All 54 examples continue to work.

**References**:
- `KNOWN_LIMITATIONS.md` "All C RAM must live below `$2000`".
- `compiler/ABI.md` (no current note on this; would need to be added).
- 2026-05-07 audit § 7 (Templates & runtime).

---

#### B3. No `mode7LoadGraphics` helper for non-bank-`$00` data 🟠

**Symptom**: `dmaCopyVramMode7(tilemap, mapSize, tiles, tilesSize)`
exists but assumes both pointers are in bank `$00`. The two shipped Mode 7
examples (`mode7`, `mode7_perspective`) each carry their own assembly
DMA helper (`asm_loadMode7Data`, `asm_loadSkyData`) because their
8 bpp tile data exceeds bank `$00` and needs explicit bank addressing.

**Root cause**: same as B1 — 16-bit C pointer ABI. Specific to Mode 7
because the interleaved-VRAM format makes the helper non-trivial (two
DMAs with different `VMAIN` settings) and the user assets are
characteristically large (256 tiles × 64 bytes = 16 KB tile data alone).

**Current mitigations**:
- `dmaCopyVramMode7` works for small Mode 7 demos that fit in bank
  `$00`.
- The two shipped examples have asm helpers as production
  workarounds.
- `docs/tutorials/mode7.md` (commit `81c01f5`) documents the helper
  pattern.

**Proposed fix**: add `dmaCopyVramMode7Bank(tilemap, tilemapBank,
tilemapSize, tiles, tilesBank, tilesSize)` to the lib. Implementation
mirrors the existing `dmaCopyVramMode7` but uses the explicit bank
bytes.

**Effort estimate**: **3–5 days**. Should be done as part of B1's audit
pass — same root cause, same review reviewer.

**Dependencies / interactions**:
- Subset of B1; doing B1 first naturally surfaces B3.
- C1 (ASM modules audit) might decide to implement Mode 7 helpers in
  C if the perf delta is acceptable.

**Acceptance criteria**:
- `dmaCopyVramMode7Bank` shipped, documented in `docs/tutorials/mode7.md`.
- Both shipped Mode 7 examples migrated from per-example asm helpers to
  the lib helper. Example READMEs updated.
- Mode 7 tutorial's "no `mode7LoadGraphics` helper" gotcha section
  updated to reflect the lifted gap.

**References**:
- `lib/include/snes/dma.h` (existing `dmaCopyVramMode7`).
- `examples/graphics/backgrounds/mode7/main.c` (current asm helper).
- `docs/tutorials/mode7.md` (commit `81c01f5`).

---

#### B4. `hdmaSetup` hardcodes bank `$00` — PARTIAL 🟡 (API shipped; example migration pending)

**Status update 2026-05-13**: `hdmaSetupBank()` is shipped and
documented. `examples/graphics/effects/hdma_helpers` uses the
bank-aware variant. Four other HDMA examples (`hdma_wave`,
`window`, `gradient_colors`, `transparent_window`) still use the
unsafe `hdmaSetup()` form — they happen to keep their tables in
bank `$00`, so they currently work, but they'd break the moment
their tables spill. Severity demoted from 🟠 to 🟡: API gap is
closed, the residual risk is examples drifting toward unsafe use.

Full acceptance ("hdmaSetup deprecated for ROM tables") would
require migrating those four examples and either renaming
`hdmaSetup` to `hdmaSetupBank0` or emitting a deprecation warning.
Worth doing as a follow-up but no longer blocking.

---

**Original entry (preserved for context)**:

**Symptom**: `hdmaSetup(channel, mode, destReg, table)` accepts a
2-byte pointer and assumes the table lives in bank `$00`. If the linker
spills a `static const u8 mytable[]` into bank `$01+` (because bank
`$00` filled up), HDMA reads the wrong bank — silently wrong values
written to PPU registers per scanline.

**Root cause**: same as B1.

**Current mitigations**:
- `hdmaSetupBank(channel, mode, destReg, table, bank)` exists for
  explicit-bank tables.
- `lib/include/snes/hdma.h` documents the bank-byte limitation.
- Convention: HDMA tables that change per frame go in RAM (always
  bank `$00`), avoiding the issue.
- Example `hdma_gradient` builds its table at runtime in a SUPERFREE
  ROM section that explicitly lives in a known bank, then uses
  `hdmaSetupBank`.

**Proposed fix**: same approach as B1 — make `hdmaSetupBank` the
canonical form, deprecate `hdmaSetup` for ROM-table use cases. Or
alternatively, make `hdmaSetup` auto-detect the bank via an assembly
bridge that reads the actual symbol bank at link time.

**Effort estimate**: **3–5 days**. Should be done as part of B1.

**Dependencies / interactions**:
- Subset of B1; same fix scope.

**Acceptance criteria**:
- All HDMA examples use the bank-aware path explicitly (no `hdmaSetup`
  for ROM tables — RAM tables only).
- HDMA tutorial's "bank-byte trap" gotcha updated.
- `KNOWN_LIMITATIONS.md` updated.

**References**:
- `lib/include/snes/hdma.h:66-72` (current bank-byte limitation
  documentation).
- `docs/tutorials/hdma.md` (commit `af80abc`).

---

#### B5. No `fixed32` (16.16) in `<snes/math.h>` 🟡

**Symptom**: the lib's `fixed` type is 8.8 (`s16`), with integer range
−128 to +127. For game state that needs higher range (e.g., a player
position in a 1024 × 1024 world), the user must roll their own 16.16
or 24.8 fixed-point format and the matching helpers.

**Root cause**: scope choice — the lib ships the most commonly needed
helpers; 8.8 covers the canonical SNES use cases (screen-space
coordinates, velocity, scale). 16.16 was deferred.

**Current mitigations**:
- The Math tutorial (`docs/tutorials/math.md`, commit `5e34168`)
  documents the 8.8 limit and calls out 16.16 as "build per game".
- Several patterns (camera lerp, parallax scroll) work fine in 8.8.

**Proposed fix**: add `fixed32` (s32 for 16.16 representation) and the
matching helpers: `fix32Mul`, `fix32Div`, `fix32Sin` (with extended
LUT or interpolated 8-bit LUT), `fix32Lerp`, `fix32Abs`, `fix32Clamp`,
`FIX32`, `UNFIX32`, `FIX32_FRAC`.

**Effort estimate**: **1 week**. Implementation is mechanical but
requires care for the multiply (32-bit × 32-bit → 64-bit intermediate
on a 16-bit CPU is non-trivial; might need to use the hardware
multiplier in multi-step form or a software multiply).

**Dependencies / interactions**:
- Independent of other items.
- A1 (`int = 32-bit`) doesn't directly help — `fixed32` is `s32`
  explicitly.

**Acceptance criteria**:
- `<snes/math.h>` exports `fixed32` and the matching helpers.
- New section in `docs/tutorials/math.md` covering 16.16 use cases.
- Cycle cost of `fix32Mul` documented (target: ≤ 200 cycles).
- A new example demonstrating 16.16 — perhaps a parallax background
  with sub-pixel scroll across a large world.

**References**:
- `lib/include/snes/math.h`.
- `docs/tutorials/math.md` (commit `5e34168`).

---

#### B6. No `atan2`, `sqrt`, `pow` in `<snes/math.h>` — PARTIAL 🟡 (2 of 3 shipped, algorithmic not LUT)

**Status update 2026-05-13**: The audit reveals three helpers
already exist in `<snes/math.h>`: `u16 sqrt16(u16)`, `u8
atan2_8(s16, s16)`, and `fixed fixSqrt(fixed)`. The implementations
are algorithmic (bit-by-bit isqrt; quadrant-folded LUT for atan2),
not pure LUTs as the acceptance criterion specified, but they meet
the user need. `pow_lut` is the remaining gap.

Severity stays 🟡 — the helpers are present and used by `atan2`
example callers (1 caller for `atan2_8` per 2026-05-13 audit).
Acceptance criterion partially met; adding `pow_lut` if a future
chantier needs it is a small follow-up.

---

**Original entry (preserved for context)**:

**Symptom**: each game that needs these rolls its own LUT. Common
operations like "angle from a vector" (`atan2`), "distance between two
points" (`sqrt(dx*dx + dy*dy)`), or "exponential decay" (`pow`) are
homework instead of one-line library calls.

**Root cause**: scope choice. The PHILOSOPHY non-goal "no `printf` in
core lib" extends to "no software-float style heavy helpers in core
lib". `sqrt` and `pow` would be either software floats (slow) or
LUT-based with size tradeoffs.

**Current mitigations**:
- The Math tutorial (`docs/tutorials/math.md`) documents the gap and
  recommends per-game LUTs.

**Proposed fix**: add LUT-based helpers:

- `atan2_lut(s16 dy, s16 dx) → u8` — 256 × 256 LUT is too big (64 KB);
  use a 32 × 32 quadrant LUT with sign symmetry → 1 KB. Accuracy: 8-bit
  angle, ±1° error.
- `sqrt_lut(u16 x) → u16` — 256-entry LUT for the high byte plus
  interpolation for the low byte. ~50 cycles per call.
- `pow_lut(fixed base, u8 exponent) → fixed` — repeated multiply for
  small `exponent`; LUT for `base ∈ [0,1]` curves.

**Effort estimate**: **3–5 days** for the three helpers + tests + a
tutorial update.

**Dependencies / interactions**:
- Independent of other items.
- Consider doing B5 + B6 together — they're both math expansion.

**Acceptance criteria**:
- `<snes/math.h>` exports the three helpers.
- Cycle cost documented.
- Math tutorial updated.

**References**:
- `lib/include/snes/math.h`.
- `docs/tutorials/math.md` (commit `5e34168`).

---

### Category C — Code organisation & maintenance burden

#### C1. Five 100 % ASM modules in `lib/source/` 🟠

**Symptom**: 5 modules in `lib/source/` are 100 % assembly with no C
side: `audio.asm` 1324 LOC, `map.asm` 1240, `sprite_dynamic.asm` 1232,
`snesmod.asm` 1201, `mode7.asm` 481. **Total: 5478 LOC of hand-written
65816 assembly**. Bus factor concentrates on contributors fluent in 65816
assembly. When something breaks, the path to fix is narrow.

**Root cause**: historical perf optimisation. These modules contain the
hot paths (NMI flush, OAM upload queue, BRR playback). Writing them in
ASM gave better cycle counts than C did at the time.

**Current mitigations**:
- `.ASSERT` directives (e.g., `sprite_dynamic.asm:51-56`) catch
  C↔ASM struct offset drift at assembly time.
- The lib's perf benchmark (`tools/opensnes-emu/test/run-benchmark.mjs`)
  measures the C-side compiler perf; it doesn't directly measure ASM
  module performance, but the existing perf claim ("30 % faster than
  PVSnesLib") includes the ASM modules contributing to the win.
- `lint_asm.py` enforces `.ACCU` / `.INDEX` markers, mitigating the
  WLA-DX tracking bug.

**Proposed fix**: module-by-module audit. For each ASM module, the
question:

1. Are the hot paths genuinely cycle-critical, or is C "good enough"
   today (especially after the 28 QBE perf patches)?
2. If C is good enough → migrate to C; the codegen quality is now
   close enough that the maintainability win exceeds the cycle cost.
3. If genuinely hot → keep ASM but extract the cold paths to C, then
   document the hot-path boundary explicitly.
4. Consider: the perf gap between C and ASM has narrowed over time
   (the 28 patches each closed some gap). Re-measure before deciding.

**Effort estimate**: **4–6 weeks** total, distributed:
- `mode7.asm` (481 LOC, simplest) — 1 week
- `sprite_dynamic.asm` (1232 LOC, medium) — 1.5 weeks
- `map.asm` (1240 LOC, medium) — 1.5 weeks
- `audio.asm`, `snesmod.asm` (1324 + 1201 LOC, complex — SPC700
  interaction) — 2–3 weeks combined

**Dependencies / interactions**:
- C2 (sprite/text duplication) is a sub-set: addressing it informs the
  approach to the other modules.
- B1's audit (lib API extension) might require touching `map.asm` and
  `sprite_dynamic.asm` — combine the work.
- A4 (oamSet framesize improvements) could close some of the C-vs-ASM
  perf gap, making more migration feasible.

**Acceptance criteria**:
- For each module: a written "keep ASM / migrate to C" decision with
  benchmark backing.
- Modules migrated to C: cycle benchmark shows ≤ +10 % regression
  (acceptable for the maintainability win) or no regression (ideal).
- Modules kept in ASM: a `_internal.h` style header documenting the
  C-callable surface and the internal invariants (struct offsets,
  required register state, etc.).
- Documentation update: `PHILOSOPHY.md` or a new `lib/ARCHITECTURE.md`
  explains the C / ASM split policy after the audit.

**References**:
- 2026-05-07 audit § 4 (Architecture, third observation).
- `ROADMAP.md` "Next steps" P3.2 (sprite/text duplication audit).

---

#### C2. Sprite/text C/ASM parallel implementations 🟠

**Symptom**: `lib/source/` has both `sprite.c` (332 LOC) and the
sprite_*.asm files (oamset 173, dynamic 1232, lut 171); both `text.c`
(262 LOC) and `text.asm` (290), plus `text4bpp.c` (26) and `text4bpp.asm`
(360). Two sources of truth per module.

**Root cause**: historical migration in progress. Some functions exist
in C with a perf-guarded ASM helper for the inner loop; some have full
parallel implementations. ROADMAP P3.2 flags this as the largest
remaining cleanup item.

**Current mitigations**:
- `oamSet` has both function form (`sprite_oamset.asm`) and macro form
  (`oamSetFast` in `sprite.h`); the macro is the recommended hot-path
  pattern.
- `text4bpp` uses C for the API + ASM for the inner loop — the right
  shape, but applied inconsistently across the rest.

**Proposed fix**: per ROADMAP, "Benchmark each, decide migration vs
documentation, eliminate duplications". Concrete steps:

1. Run `tools/opensnes-emu/test/run-benchmark.mjs` against both forms
   of each duplicated function.
2. Per function, decide: keep both (with documented why), keep C
   (drop ASM), keep ASM (drop C wrapper).
3. Migrate `examples/` callers to the surviving form.
4. Update API docs.

**Effort estimate**: **2–3 weeks**, perf-critical. Already estimated
in ROADMAP as 2–3 weeks.

**Dependencies / interactions**:
- C1's audit (full ASM-module review) subsumes this; doing C2 first as
  a smaller chantier validates the methodology.
- A4 (oamSet framesize) might reduce or eliminate the perf gap that
  motivates the ASM `oamSet` — re-measure after A4.

**Acceptance criteria**:
- Each duplicated function has a single canonical form (or a documented
  rationale for both).
- All `examples/` migrated to the canonical form.
- Cycle benchmark shows ≤ +5 % regression on the affected examples.

**References**:
- `ROADMAP.md` "Next steps" P3.2.
- 2026-05-07 audit § 6 (Library API).

---

### Category D — Toolchain & emulator coverage

#### D1. snes9x doesn't detect GSU in OpenSNES SuperFX ROM headers — PARTIAL 🟡 (Mesen2 phase shipped + CI-installed)

**Status update 2026-05-13**: Path D1.a is largely shipped. The
Mesen2 visual-regression phase exists
(`tools/opensnes-emu/test/phases/visual-mesen2.mjs`), runs 4
SuperFX/SA-1 examples through Mesen2's `--testrunner`, and
`.github/workflows/opensnes_build.yml` installs Mesen2 (and xvfb
for headless) as part of the functional-tests job. CI runs are
effectively validated against Mesen2.

The "mandatory" gap is local dev only: if Mesen2 isn't installed,
the phase reports `skipped` rather than failing. CI cannot skip
because the workflow installs Mesen2 unconditionally. Severity
demoted 🔴 → 🟡: silent-vacuous-pass risk is essentially gone for
CI, present only for contributors running tests locally without
Mesen2.

Full acceptance ("CI fails if Mesen2 binary unavailable") would
require failing the phase on absence rather than skipping. Worth
doing but no longer urgent.

---

**Original entry (preserved for context)**:

**Symptom**: snes9x's libretro core, used by `tools/opensnes-emu` for
visual regression, does **not** detect the GSU chip in OpenSNES's
SuperFX ROM headers. Every SuperFX example renders the message
"GSU NOT DETECTED" and the chip code never executes. The visual baseline
captures this no-op state; future visual regression compares two
"GSU NOT DETECTED" screens and passes vacuously.

**Root cause**: snes9x's ROM-header chip-detection logic doesn't match
the byte pattern OpenSNES emits in its SuperFX `hdr_superfx.asm`.
Whether this is a snes9x bug or an OpenSNES header bug is
undetermined — both Mesen2 and bsnes detect the chip, snes9x does not.

**Current mitigations**:
- A separate `Mesen2 Visual Regression` phase in
  `tools/opensnes-emu/test/run-all-tests.mjs` runs SuperFX/SA-1
  examples through Mesen2 in `--testrunner` mode. Mesen2 detects the
  GSU correctly.
- The Mesen2 phase is gated by the presence of the Mesen2 binary
  (`scripts/install-mesen2.sh` fetches it once per CI run, cached).
- **Phase is optional**: if the binary is absent, the phase skips, and
  the rest of the test suite still passes.
- `KNOWN_LIMITATIONS.md` documents the gap.

**Proposed fix** (two paths, choose one):

**Path D1.a — Make Mesen2 phase mandatory in CI** (cheaper, faster):
- Update `.github/workflows/opensnes_build.yml` to require Mesen2 binary
  before the functional-tests job proceeds.
- Cache the Mesen2 binary properly to avoid per-run download.
- Document in `KNOWN_LIMITATIONS.md` and `BENCHMARK.md` that Mesen2 is
  the SuperFX validation reference.

Effort: **3–5 days**.

**Path D1.b — Patch ROM header so snes9x detects GSU** (more correct,
harder):
- Audit OpenSNES's `hdr_superfx.asm` against snes9x's chip-detection
  logic.
- Determine the byte pattern snes9x expects and adjust if compatible
  with hardware reality.
- Test against bsnes and Mesen2 to confirm no regression.

Effort: **1–2 weeks**, requires deep understanding of ROM header
conventions and snes9x's ROM-load path.

**Dependencies / interactions**:
- D2 (SA-1 SIWP unsourced) is loosely related — both are SuperFX/SA-1
  validation gaps.

**Acceptance criteria** (Path D1.a):
- CI fails the build if Mesen2 binary is unavailable.
- All shipped SuperFX/SA-1 examples have current Mesen2 baselines.
- Documentation updated.

**Acceptance criteria** (Path D1.b):
- snes9x's libretro core (the version pinned in `tools/opensnes-emu`)
  correctly detects the GSU in all OpenSNES SuperFX ROMs.
- Visual baselines re-captured to reflect the actual chip-running
  behavior (not the "NOT DETECTED" placeholder).
- Mesen2 phase remains as a secondary cross-check, no longer the only
  validation source.

**References**:
- `KNOWN_LIMITATIONS.md` "🟢 SuperFX: snes9x does not detect the GSU
  chip — Mesen2-headless covers it in CI".
- `tools/opensnes-emu/test/phases/visual-mesen2.mjs`.
- 2026-05-07 audit § 10 (Tests & CI, "biggest single quality risk for
  chip ROMs").

---

#### D2. SA-1 SIWP register init is "unsourced assumption" 🟡

**Symptom**: `templates/crt0.asm:557+` initialises the SA-1 by writing
`$FF` to register `$002229` (SIWP) with the inline comment "*maybe
bit=1 means WRITABLE*". The value comes from observation of working
examples, not from a published SA-1 spec. If a future SA-1 cartridge
revision behaves differently, the init silently fails and the chip is
half-configured.

**Root cause**: SA-1 documentation is fragmented. The chip's register
behaviour is documented partially in scene wikis, never officially
published. Working values come from reverse engineering existing
cartridges.

**Current mitigations**:
- `KNOWN_LIMITATIONS.md` documents the trap honestly.
- The two SA-1 examples (`sa1_hello`, `sa1_starfield`) work in
  emulators (Mesen2, bsnes-mostly).
- ROADMAP plans hardware verification as a v1.0 must-have.

**Proposed fix**: hardware validation. Test on FXPak Pro with real SA-1
cartridge, confirm the SIWP value, document the observation, update
the comment to remove the "maybe".

**Effort estimate**: **1–2 days** (hardware and tester time, not
coding time). Requires:
- Physical FXPak Pro device.
- Building OpenSNES SA-1 examples to load on it.
- Testing init sequences with various SIWP values.
- Documenting the observed behaviour.

**Dependencies / interactions**:
- D1 (Mesen2 mandatory) doesn't help — SIWP init is a hardware
  question, not an emulator question.
- A long-term "hardware verification docs" chantier (per ROADMAP) is
  the umbrella under which this lives.

**Acceptance criteria**:
- A test report (committed as `docs/hardware/SA1_VALIDATION.md` or
  similar) describing the SIWP init behaviour on real hardware.
- `templates/crt0.asm` comment updated to remove the speculative
  "maybe", citing the validation report.
- `KNOWN_LIMITATIONS.md` severity drops from 🟡 to 🟢 (validated).

**References**:
- `templates/crt0.asm:557+`.
- `KNOWN_LIMITATIONS.md` "🟡 SA-1 SIWP register init is an unsourced
  assumption".
- ROADMAP "v1.0 must-have: Hardware verification docs".

---

#### D3. SuperFX has no C compiler (intrinsic, not fixable)

**Symptom**: SuperFX's GSU is a custom RISC ISA. No port of any C
compiler to it exists; OpenSNES's SuperFX support is assembly-only
(via `wla-superfx`). Heavy compute lives in GSU assembly; the C side
of a SuperFX project orchestrates GSU jobs and reads results.

**Root cause**: GSU's instruction set, register layout, and program
memory model are sufficiently different from a typical CPU that
porting a C compiler is a research project, not an engineering task.
There is no upstream effort.

**Current mitigations**:
- `wla-superfx` assembler ships with the SDK.
- `examples/memory/superfx_hello`, `examples/graphics/effects/superfx_3d`
  document the orchestration pattern.
- `docs/tutorials/superfx.md` covers the model.

**Proposed fix**: **none addressable**. This is a hardware reality.

**Possible adjacent improvements** (smaller chantiers, not D3 fixes):
- Better assembly macros / DSL for common GSU patterns (matrix
  operations, vertex transforms).
- A higher-level "GSU job descriptor" library that lets C code build
  GSU programs at runtime without writing raw assembly.

**Effort estimate**: N/A for D3 itself; adjacent improvements ~1–2
weeks each.

**Dependencies / interactions**: none.

**Acceptance criteria**: not applicable; this item is documented for
record-keeping only.

**References**:
- `docs/tutorials/superfx.md`.
- `KNOWN_LIMITATIONS.md` "🟡 SuperFX C support is intentionally absent".

---

### Category E — Runtime / NMI / hardware-level

#### E1. WRAM data port `$2180–$2183` race in NMI 🟢 (SHIPPED 2026-05-09)

> **Status**: shipped 2026-05-09 as a hybrid Option 1 + Option 2.
> Severity downgraded 🔴 → 🟢 (caught at build time, hard fail).
> See `KNOWN_LIMITATIONS.md` (entry rewritten), `.claude/rules/
> nmi_audit.md` (lint reference added), `make/common.mk` (post-link
> hook), `.github/workflows/lint.yml` (regression-suite job).
>
> **Implementation**: `devtools/check_nmi_wram_race.py` parses
> per-example `combined.asm` + `*.c.asm` intermediates, builds the
> call graph, finds NMI callback roots (NmiHandler, DefaultNmiCallback,
> + every symbol passed to `nmiSet`/`nmiSetBank` discovered via
> `pea.w <sym>; ... jsl nmiSet`), takes the closure, and fails the
> build if any reachable function writes to `$2180-$2183`. Lib
> functions are NOT followed — they are out-of-scope (audited via
> `.claude/rules/nmi_audit.md` checklist).
>
> **Regression suite**: `devtools/test_check_nmi_wram_race.py` —
> 6 cases (clean callback, direct port write, transitive write,
> unreachable port write, `nmiSetBank` variant, indirect jump not
> followed). Wired into CI via `nmi-race-tests` job.
>
> **Sweep**: ran on all 54 examples post-build, 0 violations
> (current state is clean — the lint catches future regressions).
>
> **Bypass**: `SKIP_NMI_RACE_CHECK=1` per-build for debug. Document
> the reason in the commit if used.

**Symptom**: main-thread code writes multi-byte sequences to `$2180`
after setting an address with `$2181-$2183`. If NMI fires mid-sequence
and any code in the NMI path touches those ports, the address pointer
is silently corrupted and the main thread resumes writing garbage to a
wrong location.

**Root cause**: SNES hardware design — the WRAM data port shares
state between main thread and interrupt context, with no lock.

**Current mitigations**:
- The SDK's NMI handler (`templates/crt0.asm`) deliberately avoids
  these ports.
- `KNOWN_LIMITATIONS.md` documents the trap.
- `.claude/rules/nmi_audit.md` lists this as a checklist item before
  modifying the NMI handler.
- `templates/crt0.asm:798-802` has an inline warning at the NMI
  handler's docstring.

**The unmitigated case**: user code in a custom `nmiSet()` callback
that calls a function which goes through `$2180-$2183`. There is no
automated check.

**Proposed fix**: static lint that flags any function called (transitively)
from a `nmiSet()` callback if it writes to `$2180-$2183`. Implementation
options:

1. **Source-level grep** (cheapest): regex-scan example C and ASM for
   `$2180`/`$2183` writes inside functions named `*Nmi*` or registered
   via `nmiSet()`. False-positive prone but a useful warning.
2. **Symbol-graph analysis** (more correct): build the call graph from
   `.sym` files, identify the closure of `nmi_callback`'s call tree,
   flag any function in that closure that touches the WRAM data port.
   Implementation lives in `devtools/symmap/symmap.py` as a new check.
3. **Runtime check** (heaviest): inject a wrapper in NMI that asserts
   `$2181-$2183` are unchanged across the callback. Catches the bug at
   runtime in debug builds.

**Effort estimate**:
- Option 1: **1–2 days**.
- Option 2: **1–2 weeks** (graph analysis is the slow part).
- Option 3: **3–5 days**.

**Dependencies / interactions**:
- Independent of other items.
- If implemented as a `devtools/symmap/symmap.py` extension, integrates
  with the existing bank-`$00`-overflow check.

**Acceptance criteria**:
- A lint exists, runs in CI, fails the build (or warns) on detection.
- A `.claude/rules/nmi_audit.md` step references the lint.
- A regression test introduces a deliberate `$2180`-in-NMI bug and
  confirms the lint catches it.

**References**:
- `KNOWN_LIMITATIONS.md` "🔴 WRAM data port `$2180–$2183` is NOT safe in
  NMI".
- `templates/crt0.asm:798-802`.
- `.claude/rules/nmi_audit.md`.
- 2026-05-07 audit § 7 (Templates & runtime).

---

#### E2. Cycle-count CI gate — SHIPPED hard 2026-05-09 🟢

> **Status**: shipped 2026-05-09. The gate is now hard (fails the
> CI job on threshold breach) with a `Cycle-Regression-OK:` commit
> trailer as the deliberate-regression override. The original soft
> gate was 24 hours old when promoted — long enough to confirm its
> signal-to-noise was workable, short enough that the design didn't
> calcify around the soft mode.
>
> **Thresholds picked**: total > 5 % regression OR per-function
> > 25 % AND > 50 cycles absolute. The combined percent + absolute
> floor on the per-function arm rules out small-routine noise
> (a 4-instruction function going from 10 → 13 cycles is +30 % but
> only +3 cycles absolute — not a real regression in practice). Both
> knobs are tunable via cyclecount.py CLI flags but defaults are
> baked into the workflow.
>
> **Override mechanism**: trailer in any commit in the PR range,
> `Cycle-Regression-OK: <reason>`. Picked over PR labels (need
> write access) and over comment markers (no audit trail in git).
> Reasons live in git history forever; `git log --grep` enumerates
> every accepted trade-off with its rationale.
>
> **Files touched**:
> - `devtools/cyclecount/cyclecount.py` — added
>   `--fail-on-regression`, `--total-pct-limit`, `--fn-pct-limit`,
>   `--fn-abs-limit` flags. New `check_regression()` function
>   returns breach descriptions; main returns 1 if any breach.
> - `.github/workflows/benchmark.yml` — `continue-on-error` on
>   the comparison step retained (for comment posting), new
>   "Check for override trailer" step parses `git log --format`,
>   final "Enforce gate" step fails the job if regression and no
>   override.
> - `docs/BENCHMARK.md` — "CI gate (hard, with override trailer)"
>   section replaces the old "soft, comment-only" section. Threshold
>   table, override rationale, and short history block.
>
> **Validation**: ran cyclecount.py locally with synthetic injection
> (30 extra `rep #$30` instructions in `loop_sum`) — gate fires:
> total +6.71 % AND per-function +75.63 % / +90 cycles, both arms
> trip. With the source unchanged: gate clean, exit 0. Workflow YAML
> validates with `python3 -c 'import yaml; yaml.safe_load(...)'`.
> Runtime test suite (--quick) still 269/269 PASS — the cyclecount
> changes don't regress non-CI usage.



**Symptom**: the cycle-count CI gate shipped 2026-05-08 (commit
`98d5014`) posts a PR comment with the per-function cycle delta but
does **not** fail the build on regression. A 100-cycle regression on
an inner loop merges if the reviewer doesn't notice the comment.

**Root cause**: design choice for v1 — soft is safer to ship initially
because the benchmark surface is narrow (34 isolated functions) and a
hard gate could block legitimate refactors that trade benchmark perf
for correctness. The decision needs to be revisited once the gate has
operated on enough PRs to surface its actual signal-to-noise ratio.

**Current mitigations**:
- The PR comment provides a clear delta table.
- The reviewer is responsible for evaluating regressions.
- `docs/BENCHMARK.md` documents the "soft means soft" policy.

**Proposed fix**: design and ship a hard-gate version. Open questions:

1. **Tolerance threshold**: should a -5 % regression fail? -10 %?
   Per-function or only on the total? What about new-function additions
   (default cost +N cycles)?
2. **Baseline strategy**: should the baseline be auto-updated on every
   merge (latest baseline = current main HEAD)? Or pinned to a release
   tag (re-baselined manually each release)? Each has tradeoffs:
   auto-update can mask gradual drift; pinning requires periodic
   manual work.
3. **Override mechanism**: how does a reviewer override "yes, this -10 %
   regression is intentional, I'm trading cycles for correctness"?
   PR label, commit trailer, special comment marker?

**Effort estimate**: **1–2 weeks** including:
- Design doc covering the three open questions.
- Implementation of the chosen baseline strategy in
  `.github/workflows/benchmark.yml`.
- Testing on representative PRs (some pure-perf, some pure-doc, some
  trade-offs).
- Updating `docs/BENCHMARK.md` and `.claude/rules/release.md`.

**Dependencies / interactions**:
- Independent of other items.
- A1 (`int = 32-bit`) might shift the benchmark baseline significantly
  when it lands; coordinate timing.
- A4 (oamSet framesize) similarly affects the benchmark; the hard-gate
  baseline must be re-captured after A4.

**Acceptance criteria**:
- A documented policy for "what counts as a regression and what's the
  override path".
- The CI workflow's `continue-on-error: true` flags removed for the
  benchmark step (or replaced with a fail-on-threshold mechanism).
- Override mechanism (PR label / commit trailer) implemented and
  documented.
- A sample PR demonstrates a successful override.

**References**:
- `.github/workflows/benchmark.yml` (commit `98d5014`).
- `docs/BENCHMARK.md` (CI gate section).

---

## 4. Interactions matrix

The defects are not all independent. Below is a cross-reference matrix:
columns are "if you're doing this", rows are "and you should consider
doing that at the same time".

| Doing → / Affects ↓ | A1 | A2 | A3 | A4 | A5 | A6 | B1 | B2 | B3 | B4 | B5 | B6 | C1 | C2 | D1 | D2 | E1 | E2 |
|---:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| A1 (`int=32`) | — |   | ↗ | ↗ | ↑ | ↗ | ↘ |   |   |   |   |   |   |   |   |   |   | ⤓ |
| A2 (volatile) |   | — |   |   | ↑ |   |   |   |   |   |   |   |   |   |   |   |   |   |
| A3 (TCO non-frame) | ↗ |   | — | ↗ | ↑ |   |   |   |   |   |   |   |   |   |   |   |   | ⤓ |
| A4 (oamSet frame) | ↗ |   | ↗ | — | ↑ |   |   |   |   |   |   |   | ↘ | ↘ |   |   |   | ⤓ |
| A5 (fork divergence) | ↑ | ↑ | ↑ | ↑ | — | ↑ |   |   |   |   |   |   |   |   |   |   |   |   |
| A6 (pointer ABI) | ↗ |   |   |   | ↑ | — | ⊃ | ⊃ | ⊃ | ⊃ |   |   |   |   |   |   | ↘ | ⤓ |
| B1 (16-bit ptr API) | ↘ |   |   |   |   | ⊂ | — | ↔ | ⊂ | ⊂ |   |   | ↘ |   |   |   |   |   |
| B2 (C RAM <`$2000`) |   |   |   |   |   | ⊂ | ↔ | — |   |   |   |   |   |   |   |   |   |   |
| B3 (mode7Load helper) |   |   |   |   |   | ⊂ | ⊂ |   | — |   |   |   | ↘ |   |   |   |   |   |
| B4 (hdmaSetup auto-bank) |   |   |   |   |   | ⊂ | ⊂ |   |   | — |   |   |   |   |   |   |   |   |
| B5 (fixed32) |   |   |   |   |   |   |   |   |   |   | — | ↗ |   |   |   |   |   |   |
| B6 (atan2/sqrt/pow) |   |   |   |   |   |   |   |   |   |   | ↗ | — |   |   |   |   |   |   |
| C1 (ASM modules) |   |   |   | ↘ |   |   | ↘ |   | ↘ |   |   |   | — | ⊃ |   |   |   |   |
| C2 (sprite/text dup) |   |   |   | ↘ |   |   |   |   |   |   |   |   | ⊂ | — |   |   |   |   |
| D1 (snes9x GSU / Mesen2) |   |   |   |   |   |   |   |   |   |   |   |   |   |   | — | ↗ |   |   |
| D2 (SA-1 SIWP) |   |   |   |   |   |   |   |   |   |   |   |   |   |   | ↗ | — |   |   |
| E1 (NMI lint) |   |   |   |   |   | ↗ |   |   |   |   |   |   |   |   |   |   | — |   |
| E2 (hard gate) | ⤓ |   | ⤓ | ⤓ |   | ⤓ |   |   |   |   |   |   |   |   |   |   |   | — |

Legend:
- `⊂` — strict subset (this item is part of that one's scope)
- `⊃` — strict superset (this item subsumes that one's scope)
- `↔` — bidirectional dependency (do them together)
- `↗` — informs (work on the source informs the target)
- `↘` — unblocks (work on the source unblocks the target)
- `↑` — adds to (work on the source adds work to the target — typically
  patch-list growth)
- `⤓` — affected by timing (e.g., E2's baseline must be captured after
  A1/A3/A4 to avoid re-baselining churn)

### Cluster summary

- **Compiler cluster**: A1 (shipped partial) / A3 / A4 / A6 are
  inter-related; A5 is downstream of all of them (each adds patches).
  A2 is independent. **A6 is the new umbrella for the entire
  pointer/bank-byte chantier — discovered during A1's investigation
  on 2026-05-08; subsumes B1 / B2 / B3 / B4 in scope** (if A6 lands
  cleanly with 24-bit pointers, the lib API extension chantiers
  become unnecessary because the compiler emits proper 24-bit
  addressing automatically).
- **Library API cluster**: B1 was the umbrella before A6 was identified;
  it is now a *subset of A6*. If A6 ships, B1 can be closed
  "subsumed by A6 — no longer needed".
- **ASM organisation cluster**: C2 is inside C1's scope; A4 affects C1
  and C2 by changing the perf gap.
- **Toolchain cluster**: D1 informs D2 (both SuperFX/SA-1); D3 is
  intrinsic.
- **Runtime cluster**: E1 is independent (but A6 unblocks part of its
  scope by making bank-byte preservation explicit in indirect calls);
  E2 is downstream of compiler changes including A6.

---

## 5. Difficulty classification

Effort estimates assume **focused work** by a contributor with the
relevant background (compiler, lib, ASM, hardware testing). Calendar
time can be 2–3× longer for part-time work. Risk is the chance of
unexpected scope expansion.

| Tier | Effort | Items | Risk profile |
|---|---|---|---|
| **Tier 0 — Done** | hours | **A1 partial (`int=2, long=4`)** — shipped 2026-05-08 | None remaining. |
| **Tier 1 — Multi-month** (foundational) | — | (vacated; A1 was here pre-investigation) | — |
| **Tier 2 — Multi-week (compiler — pointer ABI)** | 2–4 weeks | **A6** (pointer ABI + bank-byte preservation) | **Very high**. Touches the same emit code as the C.5 patch; affects every framework opt-in via indirect-call sequence. Subsumes B1 / B3 / B4 if it ships. |
| **Tier 2 — Multi-week (compiler — allocator)** | 2–4 weeks | A4 (oamSet framesize) | High. Allocator work; could improve everything but could also regress. |
| **Tier 2 — Multi-week (lib API)** | 2–3 weeks | B1 (+ B3, B4 absorbed) — *only if A6 is deferred* | Medium. API-additive (no breaking changes); audit-heavy. **Closed if A6 lands**. |
| **Tier 2 — Multi-week (lib ABI)** | 2–3 weeks | B2 (C RAM <`$2000`) — *subset of A6* | Medium. **Closed if A6 lands**. |
| **Tier 2 — Multi-week (ASM audit)** | 2–3 weeks | C2 (sprite/text dup) | Medium. Perf-critical, requires benchmark backing. |
| **Tier 2 — Multi-week (ASM audit, larger)** | 4–6 weeks | C1 (5 ASM modules audit) | Medium. Subsumes C2; sequential per module. |
| **Tier 3 — Weeks** | 1–2 weeks | A2 (volatile), A3 (TCO non-frameless), E2 (hard gate) | Low–Medium. Self-contained patches. |
| **Tier 3 — Weeks (math expansion)** | 1 week each | B5 (fixed32), B6 (atan2/sqrt/pow) | Low. Additive helpers. |
| **Tier 3 — Days–week (toolchain)** | 3–5 days | D1.a (Mesen2 mandatory), B3 / B4 standalone | Low. CI / API additions. |
| **Tier 3 — Days–week (NMI lint)** | 1–2 weeks | E1 (depending on option) | Low–Medium. |
| **Tier 4 — Days** | 1–5 days | D2 (SA-1 hardware test) | Low. Requires physical hardware. |
| **Tier 5 — Long-term ongoing** | 3–6 months part-time | A5 (fork upstreaming) | Low per patch, high cumulatively. Depends on upstream review cycles. |
| **Tier 6 — Not addressable** | N/A | D3 (SuperFX no C) | Documented for record only. |

### Total addressable effort (revised)

A6 lands → B1 + B2 + B3 + B4 are subsumed (closed without separate
chantiers). The total effort for the addressable items reduces:

- **If A6 lands**: ~5–7 person-months (was 6–9; A6 absorbs ~6 weeks
  of B-cluster work).
- **If A6 is deferred indefinitely**: ~6–9 person-months as before, but
  the lib API ends up with redundant `*Bank` variants forever.

A6 is therefore the highest-leverage chantier in the catalogue — it
either replaces 4 separate B-cluster chantiers or stays as a known
deficit. There is no in-between.

---

## 6. Strategic sequencing

Three plausible deployment paths, optimising for different objectives.

### Path 1 — Risk reduction first

Order:
1. **D1 path a (Mesen2 mandatory)** — 3–5 days. Closes the SuperFX
   validation gap with minimum effort. Real risk reduction (chip ROMs
   currently ship without runtime validation).
2. **A2 (volatile crash)** — 1–2 weeks. A real compiler bug; fixing it
   removes a documented `KNOWN_LIMITATIONS` 🟠 entry.
3. **E1 (NMI lint, option 1 — source-level grep)** — 1–2 days. Cheap
   quick-win on a 🔴 silent corruption mode.
4. **D2 (SA-1 hardware test)** — 1–2 days, given hardware.
5. **A3 (TCO non-frameless)** — 1–2 weeks. Closes `--allow-known-bugs`
   in CI.

Total: ~6–8 weeks. After this path, the SDK has no remaining 🔴 silent
corruption modes documented as "user discipline mitigates" — every
🔴 has either a fix or an automated check.

**Pros**: every item is bounded, low-risk, individually shippable.
**Cons**: doesn't unlock new capabilities (no new lib API, no fixed
type sizes).

### Path 2 — Adoption unlock (**revised after A1 partial / A6 emergence**)

Order:
1. **A6 (pointer ABI + bank-byte preservation)** — 2–4 weeks. The
   single highest-leverage chantier in the catalogue. Subsumes B1 +
   B2 + B3 + B4 if it lands cleanly. Lets user data live in any bank,
   eliminates the 6-example bank-`$00`-strangled cluster, removes the
   need for `*Bank` API variants forever.
2. **D1.a (Mesen2 mandatory)** — 3–5 days. Ships in parallel; no
   cost to combine.
3. **B5 + B6 (fixed32 + math LUTs)** — 2 weeks combined. New helpers
   that game developers will actually use. Independent of A6.
4. **A4 (oamSet framesize)** — 2–4 weeks. Improves performance for
   every user across the lib's helper functions.

Total: ~3 months. After this path, the SDK is **substantially more
capable for new users** — the structural friction blocking large
projects (bank `$00` wall, 8.8 fixed-point only, oamSet perf cliff) is
lifted.

**Pros**: maximum adoption-impact per unit effort. Aligns with
ROADMAP v1.0 must-haves. A6 first means B1/B2/B3/B4 evaporate.
**Cons**: A6 is high-risk; if it fails to land, fall back to B1
(lib API extension) which is lower-risk but less complete.

### Path 3 — Foundational compiler work

Order:
1. **A6 (pointer ABI)** — 2–4 weeks. Replaces "A1 with pointers"
   from the original Path 3 plan. A1 partial (`int=2, long=4`)
   already landed.
2. **A2 (volatile crash)** — 1–2 weeks. Independent.
3. **A3 (TCO non-frameless)** — 1–2 weeks.
4. **A4 (oamSet framesize)** — 2–4 weeks.
5. **A5 (upstreaming campaign)** — long-term ongoing.

Total: ~3–4 months. After this path, the SDK has a **substantially
cleaner compiler foundation** — proper 24-bit pointers, no
`--allow-known-bugs` flag, no `volatile` bug, the `oamSet` perf cliff
gone, and (eventually) the patch divergence narrowed.

**Pros**: addresses the deepest defects; future lib + tool work
becomes easier.
**Cons**: high risk on A6 (the same code paths that had the C.5
bug), slow to ship anything user-visible.

### Recommended path (default, revised 2026-05-08)

**Path 2 (adoption unlock)** with **A6 as the leading chantier**, plus
the **risk-reduction quick wins from Path 1** (D1.a, E1 option 1)
running in parallel.

Rationale:
- A1 partial already shipped — the foundation is laid.
- A6 is the natural next step. Successful A6 collapses 4 separate
  Category-B chantiers into one chantier's deliverable.
- Path 1's quick wins (D1.a, E1) are cheap and shippable in parallel
  with A6's investigation phase.
- A6 is genuinely higher-risk than the rest; if it fails to land
  cleanly within the 2-4 week budget, fall back to B1 (the safer
  lib-API extension path) without penalty — the work done on A6's
  investigation informs B1's design.

Expected calendar: **3–4 months**, depending on contributor
availability and whether A6 succeeds on first attempt.

---

## 7. Acceptance criteria summary

A condensed table of "how do we know each item is done":

| ID | Acceptance test |
|---|---|
| A1 (partial — shipped 2026-05-08) | `sizeof(int) == 2`, `sizeof(long) == 4` on w65816; 264/265 quick suite green (1 baseline drift accepted on `basics/random`); 7 cproc patches in PINS.md (was 6); pointer change deferred to A6. |
| A2 | `volatile` loop test compiles + runs; QBE patch (29th) submitted upstream. |
| A3 | All 5 `[KNOWN_BUG]` cleared; `--allow-known-bugs` removed from CI. |
| A4 | `oamSet` framesize ≤ 32 bytes; bench shows multi-arg helpers improved. |
| A5 | ≥ half of patches accepted upstream; PINS.md updated. |
| A6 | `sizeof(void *) == 4` (24-bit + 1 pad); indirect-call emit reads bank byte from pointer; 265/265 quick suite green; SA-1 / SuperFX function-pointer examples Mesen2-validated; B1 / B2 / B3 / B4 closed as subsumed; C.5 padding fix can be reverted. |
| B1 | 12–15 `*Bank` variants shipped; 6 tightest examples ≥ 256 bytes free; ratchet bumped to 256. **Closed if A6 lands.** |
| B2 | C variables at `$7E:2500` work; regression test green; templates updated. |
| B3 | `dmaCopyVramMode7Bank` shipped; 2 Mode 7 examples migrated. |
| B4 | All HDMA examples on bank-aware path; `hdmaSetup` deprecated for ROM tables. |
| B5 | `fixed32` + helpers shipped; 16.16 example shipped; tutorial updated. |
| B6 | `atan2_lut`, `sqrt_lut`, `pow_lut` shipped; tutorial updated. |
| C1 | Per-module decision document; cycle bench stable; lib/ARCHITECTURE.md written. |
| C2 | Single canonical form per duplicated function; bench ≤ +5 %. |
| D1 | (a) Mesen2 mandatory in CI; or (b) snes9x detects GSU; baselines captured. |
| D2 | `docs/hardware/SA1_VALIDATION.md` written; SIWP comment updated; severity → 🟢. |
| D3 | Not applicable. |
| E1 | NMI lint exists, runs in CI, fails on detection; regression test green. |
| E2 | Hard-gate policy documented; override mechanism shipped; sample PR demonstrates. |

---

## 8. How to use this document

### When starting a chantier

1. Read the relevant item's full entry — symptoms, root cause,
   mitigations, proposed fix, effort, dependencies.
2. Check the Interactions Matrix — is there a sibling item to bundle?
3. Check `ROADMAP.md` — is this scheduled for a specific release?
4. Open a GitHub issue with a link to this document's section.
5. Add a `chantier:` prefix to commit messages so the work is
   identifiable in `git log`.

### When updating the document

This document is **stateful**. Items move:
- **Pending** → **In progress** → **Done** → **(removed from doc, kept
  in git history)**
- Severity may drop as mitigations land.
- Effort estimates may revise as actual work is done.

Update protocol:
1. When a chantier kicks off, mark the item "**Status**: in progress
   since YYYY-MM-DD" at the top.
2. When done, mark "**Status**: done in commit `<sha>`, YYYY-MM-DD" and
   leave the item in place for one release cycle (so the next reader
   sees the resolution context); then move to a "Resolved" appendix or
   delete.
3. When new structural defects surface, add them in the right category
   with the same template.

### When prioritising

The strategic sequencing in §6 is a recommendation, not a mandate. The
maintainer balances:
- ROADMAP commitments (v1.0, v1.1, …).
- Contributor availability and skill set.
- Real-world user pain (bug reports surfacing one item disproportionately).
- Risk appetite (Path 3 is high-reward, high-risk; Path 1 is the
  opposite).

Re-cost the items every 6 months. The cost of A1 might drop if QBE
upstream changes its IR; the cost of D1.b might drop if snes9x bumps
to a version that detects the GSU; the cost of A5 changes every
upstream release cycle.

### When a new audit happens

If a future external review re-audits the SDK, cross-reference this
document with the audit's findings. New items go into the catalogue;
audit-confirmed-resolved items move to the "Resolved" section.

---

## 9. References

- 2026-05-07 external review — original audit document held privately
  off-tree; its findings are absorbed into this catalogue, into
  `KNOWN_LIMITATIONS.md`, into `ROADMAP.md`, and into the
  `tutorials/` wave shipped during the post-audit response.
- `KNOWN_LIMITATIONS.md` — public-facing canonical list of silent
  failures.
- `ROADMAP.md` — release-aligned planning.
- `compiler/PINS.md` — current state of cproc / qbe / wla-dx forks.
- `compiler/ABI.md` — calling-convention reference (the source of the
  16-bit pointer ABI claim).
- `PHILOSOPHY.md` — design principles and non-goals.
- `.claude/rules/release.md` — pre-release checklist (which lints to
  run).
- `.claude/rules/bank0_budget.md` — the bank-`$00` ratchet policy.
- `.claude/rules/doc_consistency.md` — the doc-drift sentinel policy.
- `.github/workflows/benchmark.yml` — the cycle-count CI gate (soft
  mode, 2026-05-08).

### Commits referenced

| Item | Commit | Date |
|---|---|---|
| Bank `$00` ratchet | `e7e7301` | 2026-05-07 |
| `oamSet` perf cliff doc | `4b31167` | 2026-05-07 |
| WaitForVBlank perf | `6d438e6` | 2026-05-08 |
| Mode 7 tutorial | `81c01f5` | 2026-05-08 |
| HDMA tutorial | `af80abc` | 2026-05-08 |
| Math tutorial | `5e34168` | 2026-05-08 |
| Cycle-count CI gate | `98d5014` | 2026-05-08 |

---

*End of document. Living document — re-read and re-cost periodically.*
