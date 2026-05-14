# OpenSNES SDK — Senior-Engineer Professional Review

**Reviewer voice:** 20+ years SNES / retro / embedded systems engineering.
Lineage: would have been comfortable shipping cartridges at Rare or Argonaut
in 1995. Tone target: *intransigeant mais objectif* — call defects defects,
recognise mature engineering plainly, refuse to dress silent-failure traps
as "design tradeoffs".

**Project under review:** OpenSNES SDK
**HEAD:** `9e7710a601e01e97824d232fa7ebd39796806a3c` (post-v0.17.0 release, 2026-05-09)
**Scope:** full repository **except `compiler/wla-dx`** (per user instruction —
WLA-DX is third-party with one merge-base PR, not OpenSNES-authored code).
**Environment:** Linux 6.19 aarch64, gcc / clang / node available.

---

## Table of contents

1. [Executive verdict](#1-executive-verdict)
2. [Methodology — what was actually run](#2-methodology--what-was-actually-run)
3. [Project identity & positioning](#3-project-identity--positioning)
4. [Architecture & layering](#4-architecture--layering)
5. [Compiler stack — moat and maintenance bomb](#5-compiler-stack--moat-and-maintenance-bomb)
6. [Library API — public surface and traps](#6-library-api--public-surface-and-traps)
7. [Templates & runtime — the spine](#7-templates--runtime--the-spine)
8. [Build system](#8-build-system)
9. [Tooling — first-party and dev-side](#9-tooling--first-party-and-dev-side)
10. [Tests & CI](#10-tests--ci)
11. [Examples — coverage and pedagogy](#11-examples--coverage-and-pedagogy)
12. [Documentation — what's anchored, what drifts](#12-documentation--whats-anchored-what-drifts)
13. [Project hygiene](#13-project-hygiene)
14. [Performance story — the 30 % claim](#14-performance-story--the-30--claim)
15. [Strategic risks & punch list](#15-strategic-risks--punch-list)
16. [Comparative positioning](#16-comparative-positioning)
17. [Final verdict](#17-final-verdict)

---

## 1. Executive verdict

**Headline grade: B+ / "late-beta SDK with senior-engineer hygiene".** Ready
for serious hobby development, game jams, educational use; one-or-two
chantiers away from being "boring infrastructure" for commercial-grade
projects.

### Five strongest aspects (in priority order)

1. **Submodule pin discipline** (`compiler/PINS.md` + `make verify-toolchain`).
   Three downstream-patched compilers + a verifier that fails the build on
   silent SHA drift. Direct response to a documented incident
   (`PINS.md:11-15`, `mktype()` UB). This is professional infrastructure.
2. **Silent-failure catalogue** (`KNOWN_LIMITATIONS.md`, 322 lines, 14
   entries with severity tags `🔴/🟠/🟡/🟢`). Three previously-🔴 entries
   ratcheted to 🟢 in v0.17.0 alone (volatile, NMI WRAM port race, cycle
   regressions). The catalogue is honest about what bites and where the
   automated guard rail lives.
3. **NMI direct-page isolation** (`templates/crt0.asm:97-100`). Page-aligned
   `tcc__nmi_registers` mirror means the NMI handler has its own DP — saves
   the ~260-cycle save/restore that PVSnesLib pays. Documented, paid for
   in WRAM, observable in the assembly. Real engineering, not folklore.
4. **Doc-drift sentinel** (`devtools/check_doc_drift.py`, wired in CI under
   `lint.yml::doc-drift`). Three drift classes mechanically blocked:
   version macros, ROADMAP status line, examples count. Rule auto-loaded
   in `.claude/rules/doc_consistency.md`. The fact that the drift caught
   today is in *unchecked* prose locations is itself a maturity signal —
   the anchored claims are clean.
5. **Compiler-test phase + cycle-count hard gate** (`opensnes-emu` Phase 2,
   `tools/opensnes-emu/test/phases/compiler-tests.mjs`; cycle gate from
   chantier E2). The compiler patches that give the SDK its competitive
   edge are protected by 60+ C→ASM pattern assertions and a per-function
   cycle-count regression gate. `--allow-known-bugs` was retired in
   chantier A3 — every previously-tolerated regression has shipped or
   been removed. CI cannot drift.

### Five weakest aspects (where I'd push back hardest in review)

1. **Twin C/ASM module pairs in `lib/source/`.** `sprite.c` (332 LOC) +
   `sprite_dynamic.asm` (1232) + `sprite_oamset.asm` + `sprite_lut.asm` +
   `sprite_dynamic_*.c` (3 helpers); `text.c` (262) + `text.asm` *and*
   `text4bpp.c` (244) + `text4bpp.asm` (360). ROADMAP names this as
   "P3.2 — Largest remaining cleanup item" (`ROADMAP.md:233-235`) and
   gives no shipping date. Until then, every contributor has to know
   which side owns which symbol. Real bus-factor risk: ~5 000 LOC of
   65816 ASM (`audio.asm:1324`, `map.asm:1240`, `sprite_dynamic.asm:1232`,
   `snesmod.asm:1201`, `mode7.asm:481`) that very few humans worldwide
   can actually review.
2. **Razor-thin bank-$00 margin in active examples.** `mapscroll.sym` has
   *28 bytes free* against a 16-byte hard-fail ratchet (`make/common.mk:60`,
   `BANK0_FAIL_THRESHOLD=16`). Twelve bytes of headroom — one extra string
   literal in `lib/source/console.c` and a release ships a broken
   mapscroll. The ratchet is good; the gap between the ratchet and the
   floor is alarming. Five other examples are below 100 bytes free.
3. **Doc check-count drift in non-anchored prose.** `PHILOSOPHY.md:35`
   says "261-check CI suite", `README.md:104` says "~390 automated checks",
   `ROADMAP.md:71` says "~390-check suite", `ROADMAP.md:246` says "257 →
   261", actual `--quick` run today: **269 checks across 7 phases**. Four
   different numbers in four prose locations. The sentinel doesn't catch
   this class because the numbers aren't anchored to a verifiable file
   (the rule explicitly says don't anchor them — see
   `.claude/rules/doc_consistency.md:60`), but nothing else replaces them
   either.
4. **`tools/font2snes/` (C) coexists with `devtools/font2snes/font2snes.py`.**
   Same name, different language, different output, no doc reconciles them.
   I had to read both `README.md` files to figure out which the asset
   pipeline actually invokes (it's the C one — `make/common.mk` references
   `bin/font2snes`). The Python one is a maintainer convenience but its
   coexistence is a footgun.
5. **`compiler/wla-dx` working tree dirty under a green `verify-toolchain`.**
   `git status` from the parent shows `m compiler/wla-dx`; inside the
   submodule, `tests/68000/all_instructions_test/main.s` is modified and
   two untracked dirs (`.wla-built/`, `ins_tbl_gen/`) exist. The verifier
   only compares HEAD SHA, not working-tree cleanliness. Process gap: a
   contributor working on the submodule could ship local edits in a
   release build and the gate wouldn't notice.

### Five must-fix before v0.18.0 (severity-ordered)

| Rank | Issue | Severity | Location |
|---|---|---|---|
| 1 | Doc check-count drift (4 locations, 4 numbers) | 🟠 | `PHILOSOPHY.md:35`, `README.md:104`, `ROADMAP.md:71,246` |
| 2 | `font2snes` naming collision unresolved | 🟠 | `tools/font2snes/`, `devtools/font2snes/` |
| 3 | mapscroll bank-$00 margin = 12 bytes from build break | 🟡 | `examples/maps/mapscroll/`, `make/common.mk:60` |
| 4 | `verify-toolchain` ignores submodule working-tree state | 🟡 | `devtools/verify_toolchain.py`, `.gitmodules` |
| 5 | `oamSetFast` / `oamSetXYFast` macros documented but unused in examples | 🟡 | `lib/include/snes/sprite.h`, `examples/games/*/main.c` |

---

## 2. Methodology — what was actually run

This review is anchored in concrete commands run against the post-v0.17.0
checkout, not in re-reading docs. Numbers below are from this run, not
from the SDK's own documentation (which disagrees with itself in places).

| Step | Command | Result |
|---|---|---|
| Submodule integrity | `make verify-toolchain` | `OK: toolchain pins match (3 submodule(s) verified)` |
| Cold rebuild | `make clean && make` | exit 0, **6.96 s wall-clock** (warm caches), 11 bank-$00 nearly-full warnings, 7 spurious WLA SLOT warnings |
| Quick test suite | `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick` | **269/269 passed in 18.5 s** across 7 phases |
| Examples count | `find examples -name 'main.c' \| wc -l` | **55** |
| Header count | `ls lib/include/snes/ \| wc -l` | **31** |
| Lib LOC (C+ASM) | `wc -l lib/source/*.c lib/source/*.asm` | **12 696 total** |
| Worst-case bank-$00 | `symmap.py --check-bank0-overflow` | **mapscroll = 28 bytes free** (12 above hard-fail threshold) |

Files read in full or partly: `PHILOSOPHY.md`, `README.md`,
`KNOWN_LIMITATIONS.md`, `ROADMAP.md`, `CHANGELOG.md` (head), `compiler/PINS.md`,
`compiler/ABI.md` (200+ lines), top-level `Makefile`, `make/common.mk`
(417 lines), `templates/crt0.asm` (200 lines of 1819), `lib/include/snes.h`,
`lib/include/snes/sprite.h` (120 lines), one example end-to-end
(`examples/text/hello_world/main.c`), `docs/BENCHMARK.md`, all six
`.github/workflows/*.yml` enumerated, all `.claude/rules/*.md` auto-loaded.

Not read: WLA-DX internals (out of scope per user); Doxygen-rendered API
reference (rendered output, not source of truth); per-example READMEs (55
of them — sampled spot-checks instead).

---

## 3. Project identity & positioning

**Mission statement** (`PHILOSOPHY.md:14-17`):

> OpenSNES is a 2D game engine for the Super Nintendo, written for game
> developers who know C but don't want to learn 65816 assembly, PPU
> register layouts, or NMI handler timing to ship a game.

`README.md:29-45` ("A Fair Warning") immediately walks that back: *you will*
need to know assembly, *you will* need to read hex, *you will* care about
clock cycles. Reading both side by side, the actual positioning is:
**"shipped game logic in C; everything else is the hardware on its own
terms."** That's a defensible niche and `README.md:48-67` does call out
who the SDK is *not* for, in a way most retro projects refuse to.

**The five-principle framework** (`PHILOSOPHY.md:45-145`) is unusually
disciplined for a hobby SNES project. Each principle has an example, a
file/line citation, and an *acceptance-criteria checklist* (`:151-167`)
that future PRs are graded against. The "Non-goals" list (`:169-186`) is
the more impressive document — it commits to *not* building things that
would be the easy path: monolithic engine class, GC, mandatory framework
lifecycle, full `printf` in core. Refusing scope is harder than adding it.

**Audit against the principles, today's reality:**

| Principle | Verdict | Evidence |
|---|---|---|
| 1. Sane defaults, escape hatches | ✅ pass | `oamSet()` (high) + `oamMemory[id*4]` (low), `oamDynamicDraw()` + macros, both public, both documented |
| 2. Hide quirks, document the escape | ✅ pass | `KNOWN_LIMITATIONS.md` is the catalogue; sprite Y +1 quirk handled in `oamSet`, doc'd in `docs/hardware/OAM.md` |
| 3. Modules opt-in, never all-or-nothing | ⚠️ partial | `LIB_MODULES` resolver works (`make/common.mk:126-147`), but `snes.h` (the master header) pulls **18 modules unconditionally** (`lib/include/snes.h:69-120`), then has a 14-line comment block listing the 8 specialty modules you "should" include separately. This is a hybrid: the linker-side opt-in is honest, the header-side opt-in is by convention only |
| 4. Type-safe at the boundary | ⚠️ partial | The new `OamDynamicConfig` struct (`PHILOSOPHY.md:113-121`) is the model, but `oamSet(id, x, y, attr, size, tile)` still has 6 positional `u16`s and is the prevalent pattern in examples |
| 5. Predictable performance | ✅ pass | Cycle costs documented for the no-op NMI hook (25 cyc) and `oamDynamicNmiFlush`; benchmark suite is real with 34 functions and signed methodology (`docs/BENCHMARK.md`); E2 chantier added a hard CI gate against regression |

**The "change in altitude, not vocabulary" claim** (`PHILOSOPHY.md:38-41`)
is the strongest part of the positioning: keeping `oamSet`, `bgSetMapPtr`,
`WaitForVBlank` names compatible with PVSnesLib while changing the
underlying compiler and runtime is a calculated portability bet.
Migration in either direction is plausible. That said: the calling
convention reversal (left-to-right vs right-to-left) means an ASM
function ported either way needs offsets flipped (`compiler/ABI.md:115-128`)
— so the migration is ergonomic for C code, not for ASM. Worth being
explicit about.

---

## 4. Architecture & layering

The layer cake is genuinely clean:

```
examples/                 — 55 .c entry points, all build via make/common.mk
  ↑
lib/                      — public headers + C/ASM modules, opt-in via LIB_MODULES
  ↑
templates/                — crt0.asm (1819 LOC), hdr*.asm, memmap*.inc, runtime
  ↑
compiler/{cproc,qbe,wla-dx} — three submodules, pinned via PINS.md
                            — orchestrated by bin/cc65816 wrapper
```

**LIB_MODULES resolver** (`make/common.mk:126-147`). Transitive expansion
via three iterations of `_resolve_one`. Genuinely good engineering: the
resolver dependency graph is declared in 18 lines of make,
`_DEP_hdma := dma math_sqrt` is the kind of fine-grained split that pays
off for ROM size (the `math_sqrt`-vs-`math` split was carved deliberately
in chantier B6). The flatten-and-sort pattern handles diamond deps
correctly.

**Memory map per ROM mode** is selected by Make conditionals
(`make/common.mk:93-98`):

```
USE_SA1=1     → memmap_sa1.inc, hdr_sa1.asm, lib/build/sa1
USE_SUPERFX=1 → memmap_gsu.inc, hdr_superfx.asm, lib/build/superfx
USE_HIROM=1   → memmap_hirom.inc, hdr_hirom.asm, lib/build/hirom
default       → memmap.inc, hdr.asm, lib/build/lorom
```

Lib is built four times (LoROM, HiROM, SA-1, SuperFX), one obj set per
mode. Clean. Examples pick a mode, link the matching obj set, no
runtime conditional logic. The right answer for a target with no
dynamic loading.

**Reset-to-first-frame init lifecycle** (`templates/crt0.asm:1-90`,
sampled): switches to native mode, sets up DP isolation for NMI, copies
initialised data from ROM SUPERFREE sections into WRAM via `CopyInitData`
loop, calls `main()`. The `data_init_end.o` sentinel pattern
(`make/common.mk:333` plus `KNOWN_LIMITATIONS.md:93-100`) is a known
silent-fail trap that the build system makes structurally impossible to
get wrong (`LINK_OBJS` is built in fixed order, `data_init_end.o` always
last).

**The `$7E:0300` mirror reservation** (`templates/crt0.asm:179-181`) is the
kind of small detail that signals an engineer who has been bitten:

```asm
.RAMSECTION ".reserved_7e_mirror" BANK 0 SLOT 1 ORGA $0300 FORCE
    _reserved_7e_mirror dsb 544 ; Reserved: mirrors Bank $7E oamMemory
.ENDS
```

Bank $00 `$0000-$1FFF` mirrors Bank $7E `$0000-$1FFF`. The OAM shadow
buffer lives at `$7E:0300`. Without this reservation, the linker could
place a C variable at `$00:0300+N`, the C code would dereference it via
`sta.l $0000,X` (= bank $00), and the actual write would land in the
OAM shadow. Silent corruption. The 544-byte bank-$00 reservation
prevents that. **No competing PVSnesLib clone I know of does this**.

---

## 5. Compiler stack — moat and maintenance bomb

The compiler is the project's competitive edge and the single largest
liability rolled into one.

### What it is

```
.c → cproc (C11 frontend) → QBE IR → qbe -t w65816 → 65816 .asm
                                                          ↓
                                                  sed transform
                                                  (.byte→.db, .word→.dw)
                                                          ↓
                                                    wla-65816 → .obj
```

Three submodules, each pinned by SHA in `compiler/PINS.md`:

| Submodule | Pinned SHA | Patches ahead of upstream |
|---|---|---|
| `compiler/cproc` | `cceac4bc` | **8** patches |
| `compiler/qbe` | `9878b9f6` | **30** patches (the bulk of the SDK's compiler magic) |
| `compiler/wla-dx` | `ffe59ca1` | 0 patches (just a merge-base of an upstream PR) |

**38 downstream patches**, listed by short SHA in `PINS.md:42-83`. This
includes:

- `cceac4b` chantier A2 — preserve volatile through QBE (just shipped)
- `7f26c16` chantier A1 — int=2B, long=4B (just shipped)
- `ea95cac` `mktype()` UB fix — discovered after a build silently produced
  a struct in ROM
- `90b81e1` w65816 backend respecting volatile
- `d9483ee` leaf optimisation restricted to actual leaves
- `fd1bebb` `.ACCU 16` / `.INDEX 16` for WLA-DX register-size tracking
- `ed840fb` tail call optimisation for frameless functions
- `b56fa3d` direct-page `.b` for tcc__ registers, div/mod return in A
- … and 30 more

### Why it's a moat

Every one of those patches solves a real bug that bit a real ROM (the
catalogue is in `~/.claude/.../memory/compiler_optimizations.md` and in
the per-commit `compiler/PINS.md` rationale lines). The empirical case
is documented in `docs/BENCHMARK.md`: -30.3 % cycles vs PVSnesLib +
816-opt across 34 functions. The cycle gate (chantier E2) makes
regression on this number a hard CI failure — you can't accidentally
back out an optimisation.

The cproc-side fixes are equally load-bearing: `mktype()` UB, struct
initialiser handling, signed promotion, string constification, nested
struct codegen. These are not theoretical — `KNOWN_LIMITATIONS.md:225-229`
lists three remaining MSYS2-specific cproc segfaults still gated behind
`knownBug()` calls, fired only on Windows builds. The fact that the SDK
runs CI on Windows specifically to catch this regression class is
unusual maturity.

### Why it's a maintenance bomb

A future upstream rebase costs **38 patches × per-patch verification**.
The compiler-test phase is the gate (`tools/opensnes-emu/test/phases/
compiler-tests.mjs`, 60 C→ASM patterns), but a cherry-pick conflict on
QBE's emit pass will bring the compiler down for a working week. There
is no obvious mitigation other than continuing to land patches upstream
where Carbonneaux / Forney / Helin will accept them.

`PINS.md:97-124` documents the bump-a-pin procedure (move SHA, run full
suite, update `PINS.md` table, commit together). The procedure is
correct, but the burden falls on whoever owns the bump.

The Windows cproc retry pragmatism (`opensnes-emu/test/run-all-tests.mjs`
implements 3-retry on cproc segfault for MSYS2 builds; tracked in
`.github/workflows/msys2_cproc_diagnostic.yml`) is a clean engineering
compromise: don't ship buggy cproc to Linux/macOS, but recognise the
flake on Windows and don't let it break CI.

### Calling-convention oddities worth flagging

`compiler/ABI.md:31-37`:

> cc65816 pushes args **left-to-right**, while PVSnesLib's tcc816 pushes
> right-to-left (standard cdecl).

This single design choice is going to be the most-cited debugging entry
for anyone porting an ASM helper from PVSnesLib for the next decade.
The doc is worked-example correct (`compiler/ABI.md:78-128`, two real
disassemblies) and there's a port checklist (`:120-128`). I've seen
worse-documented ABIs in PRODUCT compilers. The trap is real but the
mitigation is professionally written.

The companion DP-isolation invariant (`ABI.md:181-187`): NMI runs with
its own `tcc__nmi_registers` direct page. Calling C from your `nmiSet`
callback works, but DP variables don't hold main-thread values. This is
the kind of one-paragraph note that prevents a week of debugging.

---

## 6. Library API — public surface and traps

### Inventory

| Asset | Count | Notes |
|---|---|---|
| Public headers | **31** | `ls lib/include/snes/` — README claims "30 hardware modules" (`README.md:100`) but actual is 31. Minor drift |
| C source | 21 files (3 348 LOC) | |
| ASM source | 19 files (9 348 LOC) | |
| Heaviest ASM | `audio.asm` (1324), `map.asm` (1240), `sprite_dynamic.asm` (1232), `snesmod.asm` (1201), `mode7.asm` (481) | |
| `lib/contrib/` | 1 file (`object.asm`) + README | The `object` module — game-engine code, intentionally relocated out of `lib/source/` (audit-driven) |

### `snes.h` self-contradiction

`lib/include/snes.h:1-37` (header comment) and `:64-120` (the includes).

The header comment says (`:7-10`): *"Pulls every module a typical project
needs (console, video, sprite, background, input, dma, text, interrupt,
system, mode7, hdma, window, colormath, mosaic, map, debug)."* — that's
**16** modules.

The actual `#include` block (`:69-120`) pulls **18** headers (adds
`types.h` and `registers.h`).

The "Specialty modules — include separately" comment block
(`:122-138`) lists **8 modules** the user should not get from `snes.h`.
That number is right: `audio.h`, `math.h`, `sram.h`, `collision.h`,
`lzss.h`, `gameloop.h`, `asset.h`, `scene.h`.

Net: a project that does `#include <snes.h>` pulls 18 modules whether it
uses them or not. The cost is at compile time (cproc parses each header)
and at the resolver level (Principle 3 says "opt-in" — the linker still
opts in, but the user never sees it). The phrase *"include this for all"*
in the docstring is half-true: it's "include this for the common case,
add the 8 specialty ones if you need them."

This is fixable in a single PR by either splitting `snes.h` into a
"core-minimal" header and a "common bundle" header, or by documenting
the convention as-is and accepting it. As written, the doc and the code
disagree about whether the master header is the one-stop shop or the
common-case shortcut.

### `TRUE = 0xFF` foot-trap — handled

`lib/include/snes/types.h:124-148`:

```c
/* @warning TRUE is 0xFF, not 1! This matches SNES convention where
 *          bit patterns are often used. Always compare with != FALSE
 *          rather than == TRUE.
 *
 * if (flag != FALSE) {} // Good
 * if (flag == TRUE) {}  // Bad! Will fail for non-0xFF truthy values
 */
```

The trap is real, the doc is direct, the worked example is correct. I'd
have refused to accept this convention if I were the original PVSnesLib
author — the SNES "convention" of `TRUE = $FF` is an artefact of
hand-coded ASM where `lda #$FF; sta flag` is one instruction and `bit`
testing is two — but inheriting it from PVSnesLib for migration
compatibility is defensible (Principle 1 and the migration claim in
`PHILOSOPHY.md:38-41`). The doc is honest about the cost.

### oamSet performance trap — partially handled

`KNOWN_LIMITATIONS.md:283-308` documents that `oamSet()` has a
framesize=158, costs ~158 bytes of stack manipulation per call, and
becomes visible past ~3 calls per frame. Two ergonomic helpers exist:

- `oamSetFast(id, x, y, tile, palette, priority, flags)` — drop-in macro
- `oamSetXYFast(id, x, y)` — position-only

Adoption check (grep across all example `main.c` files in `games/` and
`basics/`):

```
$ grep -c 'oamSetFast\|oamSetXYFast' examples/games/*/main.c examples/basics/*/main.c
mapandobjects: 0
breakout:      0
random:        0
timer:         0
collision_demo:0
tetris:        0
likemario:     0
scene_stack:   0
aim_target:    0
```

**Zero** uses of the fast macros across the showcase games. The
`KNOWN_LIMITATIONS.md:301-307` text says these examples *"all switched
to direct `oamMemory[]` writes"* — and indeed they do (the doc is
literally accurate). But: macros that were created to be the
recommended path are unused even by the showcase code. Two reasonable
fixes: either retire the macros (one less API surface), or convert at
least 2-3 examples to use them and let the reader pick. As-is, the
macros exist as dead documentation.

### Module duplication (the elephant)

`lib/source/` ships parallel implementations on two surfaces:

```
sprite.c               (332)   ←→  sprite_dynamic.asm        (1232)
                                ←→  sprite_lut.asm           (327)
                                ←→  sprite_oamset.asm
                                +   sprite_dynamic_dispatch.c  (helper)
                                +   sprite_dynamic_helpers.c   (helper)
                                +   sprite_dynamic_meta.c      (helper)

text.c                 (262)   ←→  text.asm                  (357)
text4bpp.c             (244)   ←→  text4bpp.asm              (360)
```

`ROADMAP.md:233-235` flags this:

> **Sprite/text duplication audit (P3.2)** — `lib/source/` has parallel
> C and ASM implementations of `sprite` and `text`. Benchmark each,
> decide migration vs documentation, eliminate duplications. Largest
> remaining cleanup item (2–3 weeks, perf-critical so risk is real).

Two-to-three weeks of perf-critical cleanup is a fair estimate. Until
then: the dispatch logic, the linker invariants, the `LIB_MODULES`
mappings, and the reader's mental model all carry the dual-implementation
overhead. This is the highest-leverage cleanup the project has on its
backlog. Closing it would let `lib/source/` shrink ~30 % and let the
"which file has the bug" question always have one answer.

---

## 7. Templates & runtime — the spine

`templates/` total: **2421 LOC** across 13 files. Heaviest: `crt0.asm`
(1819 LOC). The runtime spine. Read 200 lines and several patterns are
worth flagging.

### NMI handshake design (chantier E1 — shipped)

`templates/crt0.asm:97-100` + `KNOWN_LIMITATIONS.md:46-60`. The protocol
is asymmetric:

- Main thread sets `vblank_flag=1` via `WaitForVBlank` ("I'm ready")
- NMI handler clears `vblank_flag=0` when it has acknowledged ("done")
- A frame where NMI fires *before* main reaches `WaitForVBlank` (= lag
  frame) sees `vblank_flag=0`, skips work, increments `frame_count`

This is the PVSnesLib protocol, ported with explicit lag-frame
tolerance. The visible bonus: NMI is a no-op on lag frames, so the
~4 KB DMA budget is not consumed. The visible cost: heavy DMA frames
(>4 KB BG1+BG2) still need force-blank (`setScreenOff/On`) — documented
at `KNOWN_LIMITATIONS.md:43`.

`devtools/check_nmi_wram_race.py` (chantier E1) walks the call graph
from every NMI callback root and fails the build if any reachable
function writes to `$2180-$2183`. Wired in `make/common.mk:388-401`.
This is a hard build-time guard against silent corruption — and
specifically against new contributor code adding a `getMemoryWord` call
inside a `nmiSet` callback. The catch is that lib + crt0 are treated as
a black box; the lint targets user code only. Both halves are documented
in `.claude/rules/nmi_audit.md`.

### Cycle-cost claims that are actually quantified

`crt0.asm` documents the cost of the no-op NMI hook (25 cycles per frame
when the engine is idle), the `oamDynamicNmiFlush` cost (variable
depending on queue depth), and the cycle saving from DP isolation
(~260 cycles per NMI). `PHILOSOPHY.md:136-141` cross-references these
numbers. Cycle gates in CI (chantier E2) prevent the numbers from
silently drifting up.

`tools/opensnes-emu/test/run-benchmark.mjs` reports cycle counts for 34
benchmark functions, gated against a baseline. The gate is now hard
(chantier E2 promotion to red-fail).

This is the "predictable performance" principle made operationally real.
Most retro projects have *no* cycle-budget enforcement; OpenSNES has a
build-time gate.

### The `data_init_end.o` sentinel pattern

`KNOWN_LIMITATIONS.md:93-100` plus `make/common.mk:333`. The runtime's
data-init copy loop scans for a sentinel placed by `data_init_end.asm`.
The build system always puts that object last in the link order:

```make
LINK_OBJS := crt0.o $(RUNTIME_OBJ) data_init_start.o ... $(LIB_OBJS) ... data_init_end.o
```

A user can't reorder this from their example `Makefile` — the variable
is computed inside `common.mk`, not exposed. Silent-failure trap closed
by structural impossibility. This is the right answer.

---

## 8. Build system

Two layers: top-level `Makefile` (190 LOC), per-example
`make/common.mk` (417 LOC).

### Top-level orchestration

`Makefile:60-110` — `all` depends on `submodules`, `compiler`, `tools`,
`lib`, `examples`. `submodules` runs `git submodule update --init
--recursive`. `compiler` depends on `submodules` *and* `verify-toolchain`,
so a `make` call after `git submodule update --remote` (which would drift
the SHAs) fails fast with a pin-mismatch message before building
anything. Correct ordering.

The `lint` aggregate target (`Makefile:104-107`):

```make
lint: lint-docs
    @python3 devtools/lint_asm.py
    @$(MAKE) lint-commits
```

Three lints in one target: doc drift sentinel, ASM marker check (`.ACCU
8/16` and `.INDEX 8/16` after every `rep`/`sep` — the WLA-DX tracking
bug mitigation), commit message lint (Conventional Commits + no
Co-Authored-By trailers). Anyone running `make lint` before push gets
the same coverage CI does. Good ergonomic.

### Per-example invariants

`make/common.mk` is dense for 417 lines. Highlights:

- **Bank-$00 hard-fail ratchet** (`:60`): `BANK0_FAIL_THRESHOLD ?= 16`.
  Any build that ends with fewer than 16 bytes free in bank-$00 fails.
  The chosen value sits below the current minimum (28 in mapscroll), so
  today's tree builds — but the next const literal that lands in a tight
  example breaks the build *before* the ROM ships broken. Ratchet:
  bumping it tighter is *not* an incremental commit per
  `.claude/rules/bank0_budget.md:33-49`.
- **Transitive `LIB_MODULES` resolver** (`:126-147`): three iterations
  of `_resolve_one` flatten the dep graph. Sorted output (`$(sort ...)`)
  dedupes. Diamond deps work correctly.
- **clang `-Wall -Wextra -Werror` syntax check** (`:251-273`): runs on
  every `.c` source in parallel with `cc65816`. cproc swallows `-W`
  flags silently, so a sibling clang catches what the SDK compiler
  doesn't. **Both real bugs** in `lib/source/hdma.c` and `tetris.c`
  (per ROADMAP) were caught this way. `SKIP_LINT=1` is the documented
  bypass for clang-less environments.
- **Bank-$00 + NMI-WRAM-race lints in one post-link pipeline**
  (`:355-401`). `SKIP_BANK0_CHECK=1` and `SKIP_NMI_RACE_CHECK=1` are
  the per-build escape hatches, both clearly named and clearly meant
  for "I'm doing destructive debugging" cases only.

### One real concern: WLA-DX SLOT noise

`make` emits seven instances of:

```
WARNING: There is a SLOT number 0, but there also is a SLOT (with ID 1)
with starting address 0 ($0)... Using SLOT 0.
```

These come from WLA-DX itself (memory-map config in `memmap*.inc`
templates). The note is benign — both slots exist for technical reasons
and WLA picks the right one — but they pollute every clean build with
identical text. Either (a) suppress them via `wla -q` or grep-filter in
the Makefile, (b) restructure the memmap to avoid the SLOT-0/SLOT-1
duplicate, or (c) document that they are expected. As-is, contributors
ignore "WARNING:" lines because most are noise. That's the wrong habit
to train.

### First-time clone-and-build

`README.md:129-132`:

```
git clone --recursive https://github.com/k0b3n4irb/opensnes.git
cd opensnes
make
```

This is *theoretically* a single command. I didn't measure it from
scratch in this review (the local checkout is already built), but the
Makefile graph (`submodules → verify-toolchain → compiler → tools → lib
→ examples`) means a fresh `make` does the full pipeline. The 7-second
incremental rebuild today suggests a cold build is in the 1-2 minute
range on this hardware. Acceptable for an SDK with a custom compiler.

---

## 9. Tooling — first-party and dev-side

The project distinguishes between **`tools/`** (shipped with the SDK,
end-user-facing) and **`devtools/`** (maintainer-only, lints and
helpers). The split is mostly clean.

### `tools/` inventory

| Tool | Language | Role |
|---|---|---|
| `gfx4snes` | C | PNG/BMP → SNES tiles, palettes, tilemaps |
| `font2snes` | C | Font → 2bpp/4bpp tiles |
| `smconv` | C (rewritten) | Impulse Tracker (.it) → SNESMOD soundbank |
| `tmx2snes` | ? | Tiled `.tmx` map import |
| `sa1-patch` | ? | Patch ROM map-mode byte for SA-1 (replaces an inline Python one-liner per `make/common.mk:351`) |
| `img2snes` | ? | Image conversion utility |
| `debug-fixtures` | ? | Test fixtures shared with opensnes-emu |
| `opensnes-emu` | TypeScript / WASM | Test runner + snes9x WASM emulator |

### `devtools/` inventory

| Tool | Role |
|---|---|
| `symmap/symmap.py` | Symbol-map analysis, bank-$00 overflow check |
| `verify_toolchain.py` | Compiler submodule pin check |
| `lint_commits.py` | Conventional Commits + no-Co-Authored-By |
| `lint_asm.py` | `.ACCU 8/16` and `.INDEX 8/16` markers in hand-written ASM |
| `check_doc_drift.py` | Version macro / ROADMAP / examples-count drift |
| `check_nmi_wram_race.py` | NMI WRAM-port race lint (chantier E1) |
| `cyclecount` | Per-instruction cycle estimation |
| `snesdbg` | Mesen2 Lua debugging library (BDD-style tests) |
| `brr2it` | BRR → Impulse Tracker conversion |
| `gen_hud_bar` | HUD bar generator |
| `font2snes` (Python) | **A second font2snes** — Python implementation |

### The `font2snes` collision (🟠 should-fix)

```
tools/font2snes/font2snes        (C binary, used by make/common.mk)
devtools/font2snes/font2snes.py  (Python script, maintainer convenience)
```

Both `README.md` files explain their respective purposes, but no
top-level doc reconciles them. A new contributor running `which
font2snes` or grepping the repo will find both. The naming is
ambiguous. Two minimum fixes: (a) rename the Python one
(`devtools/font_preprocess/`?), (b) add a note in `tools/README.md` and
`devtools/README.md` cross-referencing each other. Either is a one-PR
fix.

### `tools/valgrind-static.supp` (🟡 polish)

`tools/valgrind-static.supp` exists at the toolbox top level. No tool
references it. Likely a leftover from a maintainer debugging session.
Either move it under a tool that actually runs valgrind (most likely
`smconv` since it's C) or delete it. As-is it's a footnote that costs a
new contributor an investigation.

### `devtools/snesdbg` is a real asset

Mesen2 Lua scripting layer with BDD-style test syntax. This is the kind
of internal-tooling that gets retro projects from "ROM compiles" to "ROM
behaves correctly under structured tests." Worth highlighting in the
README — currently it's only mentioned in passing in `ROADMAP.md:201`.

### opensnes-emu architecture

snes9x compiled to WASM, wrapped in a Node.js test runner with **7 phases
in `--quick` mode** (the run today: 269 checks). Plus a Mesen2-headless
visual phase for chip-using ROMs (chantier P3.4). This is genuinely
ambitious test infrastructure for a retro project — most others stop at
"emulator opens the ROM."

The phase split is sensible:

1. Preconditions (toolchain present)
2. Static Analysis (ROM headers, memory maps, symbol overlaps)
3. Runtime Execution (emulated boot of each ROM)
4. Visual Regression (screenshot diff against baseline)
5. Mesen2 Visual Regression (chip-using examples)
6. Lag Frame Detection (frame timing)
7. Input Sequence Tests

Compiler tests (60 C→ASM patterns) run in non-`--quick` mode. So full
suite = 269 + 60 ≈ 329 checks. The "~390 automated checks" claimed in
`README.md:104` doesn't match either number I can reach. (See §12 on
doc drift.)

---

## 10. Tests & CI

### Today's test result

```
$ cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
…
Total: 269 checks, 269 passed, 0 failed
ALL CHECKS PASSED
real    0m18.474s
```

Phase breakdown:

| Phase | Count | Target |
|---|---|---|
| Preconditions | 7 | toolchain & deps |
| Static Analysis | 78 | ROM headers, symbols |
| Runtime Execution | 66 | emulated boot |
| Visual Regression | 55 | screenshot diff |
| Mesen2 Visual Regression | 4 | SuperFX/SA-1 chip examples |
| Lag Frame Detection | 55 | frame timing |
| Input Sequence Tests | 4 | scripted joypad |

55 = the example count. This phase ratio (one screenshot/lag check per
example) is the right level of granularity.

### Workflows (`.github/workflows/`)

| Workflow | Purpose |
|---|---|
| `opensnes_build.yml` | Linux/macOS/Windows build matrix + functional tests |
| `lint.yml` | `lint-docs` + `lint_asm` + `lint_commits` (PR-blocking) |
| `release.yml` | Tag-on-main guard + per-OS release zip + GitHub Release |
| `deploy_docs.yml` | Doxygen → GitHub Pages |
| `benchmark.yml` | Cycle-count regression gate (chantier E2 — **hard fail**) |
| `msys2_cproc_diagnostic.yml` | Windows-only cproc segfault triage |

The benchmark workflow being a hard gate (post-E2) is unusual maturity.
Most retro projects benchmark for vanity metrics; this one breaks the
build on regression. The `[skip-benchmark]` trailer is the documented
override (with explicit "you owe a follow-up PR" semantics).

### Coverage gaps

- **Phase 8 input-sequence is shipping with 4 checks** — this is the
  smallest phase. Either it's a stub waiting for more test cases, or
  4 is genuinely enough (4 input-driven examples). Worth a one-line
  comment in the runner explaining which.
- **SuperFX validation is now real but narrow**. Chantier P3.4 added
  Mesen2-headless against 4 chip-using ROMs. snes9x's libretro core
  doesn't detect the GSU in OpenSNES ROM headers (`KNOWN_LIMITATIONS.md
  :164-186`), so the snes9x phase only validates "boots without
  crashing" for SuperFX. The Mesen2 path is now mandatory in CI but
  cached locally — contributors without Mesen2 installed see a no-op
  for that phase.
- **Functional suite runs only on Linux runners.** macOS and Windows
  build but don't run the full functional tests (per `ROADMAP.md`
  "matrix Linux/macOS/Windows but functional suite runs only locally"
  — partially out of date; check `opensnes_build.yml` for current
  state). The cycle-count gate runs on Linux only too.

### What I'd push back on at code review

**No `--no-quick` invocation in any `.github/workflows/*.yml` job that I
saw.** All CI runs are `--quick`. The +60 compiler-tests checks gated
behind `--no-quick` are presumably run somewhere (probably nightly?), but
the gating logic isn't visible. Worth confirming in the next pass.

---

## 11. Examples — coverage and pedagogy

55 examples across 8 categories: `audio`, `basics`, `games`, `graphics`,
`input`, `maps`, `memory`, `text`. Each has a `main.c`, a `Makefile`, a
`README.md`, and a `screenshot.png` (verified for hello_world; sampled
across categories).

### Pedagogy quality — spot check on `examples/text/hello_world/main.c`

This is the canonical first-example. It:

- Doxygen `@par SNES Concepts` block (`main.c:11-17`) names the four
  concepts a reader needs (2bpp tiles, VRAM tilemap, CGRAM, Mode 0)
- Doxygen `@par What to Observe` (`main.c:19-22`) tells the reader what
  the screenshot should look like
- Doxygen `@par Modules Used` (`main.c:24`) names the linked LIB_MODULES
- Hand-coded font (`main.c:50-92`) — no asset pipeline dependency, every
  byte commented (`/* Tile 1: H */`)
- Forced-blank discipline (the comment at `main.c:140`) explicitly notes
  why VRAM writes happen during forced blank

This is teaching material, not just test material. Anyone reading this
file learns four orthogonal SNES concepts and sees the API for each.
Strong work.

### Per-module gap matrix (sampled)

For each public header, is there an example?

| Header | Example | Verdict |
|---|---|---|
| `console.h` | hello_world | ✅ |
| `sprite.h` | simple_sprite, animated_sprite | ✅ |
| `background.h` | mode0, mode1, mode3, mode5, mode7 | ✅ |
| `dma.h` | implicit in many | ✅ via use |
| `hdma.h` | hdma_wave, hdma_gradient, hdma_helpers | ✅ |
| `input.h` | two_players, mouse, superscope | ✅ |
| `audio.h` (snesmod) | snesmod_music, snesmod_sfx, snesmod_music_large, snesmod_music_hirom | ✅ |
| `text.h` | hello_world, text_test | ✅ |
| `mode7.h` | mode7, mode7_perspective | ✅ |
| `colormath.h` | transparency, transparent_window | ✅ (no dedicated `colormath` example) |
| `mosaic.h` | mosaic | ✅ |
| `window.h` | window, transparent_window | ✅ |
| `collision.h` | collision_demo | ✅ |
| `math.h` | aim_target (just shipped) | ✅ |
| `sram.h` | save_game | ✅ |
| `lzss.h` | mode1_lz77 | ✅ |
| `map.h` | dynamic_map, mapscroll, slopemario, tiled, mapandobjects | ✅ |
| `gameloop.h` | (none I see) | ⚠️ no dedicated example |
| `asset.h` | (none I see) | ⚠️ no dedicated example |
| `scene.h` | scene_stack | ✅ |
| `profile.h` | (none I see) | ⚠️ no dedicated example |
| `sa1.h` | sa1_hello, sa1_speed, sa1_starfield | ✅ |
| `superfx.h` | superfx_hello | ✅ (boot only — Mesen2 validates the chip exec) |
| `debug.h` | (used internally) | ⚠️ no dedicated example |

**Missing examples (mild gaps):** `gameloop`, `asset`, `profile`, `debug`.
Three of those are framework opt-ins (Principle 3) — by definition users
who don't use them never link them. A dedicated example would be a
demonstration, not a coverage need. `debug.h` is for nocash messages;
"how to use the debug module" is a 3-line example, fits in a tutorial.
None of these are blockers.

### Examples-count drift (already flagged)

`README.md:25,102,141` says **55** (correct).
`ROADMAP.md:17` says **54**. `ROADMAP.md:138` heading says **53** then
lists 53 entries. Actual: **55**.

`make lint-docs` *does* check this (per
`.claude/rules/doc_consistency.md:22-27`), so the drift in active rules
files is caught. But the numbers in ROADMAP narrative prose are not
anchored — the sentinel skips them per design. This is a known
limitation of the sentinel approach. Either the lint should grow to
catch ROADMAP narrative, or the narrative should switch to the canonical
phrasing ("every example", "the full suite") that `testing.md` and
`nmi_audit.md` already use.

---

## 12. Documentation — what's anchored, what drifts

### What's anchored

`devtools/check_doc_drift.py` mechanically blocks three classes:

1. `lib/include/snes.h:53-62` version macros vs `CHANGELOG.md` head
2. `ROADMAP.md:11` "Current Status: post-vX.Y.Z" line vs `CHANGELOG.md`
3. Examples count in `ROADMAP.md` / `README.md` / `.claude/rules/*.md`
   vs `find examples -name 'main.c' | wc -l`

All three pass today (`make lint-docs` exits 0 — verified during this
review).

### What drifts

The check-count number appears four times across active docs, all
disagreeing:

| File:line | Number | Reality |
|---|---|---|
| `PHILOSOPHY.md:35` | "261-check CI suite" | stale (matched chantier P3.4) |
| `README.md:104` | "~390 automated checks" | unreachable — full suite is 269 + 60 ≈ 329 |
| `ROADMAP.md:71` | "full ~390-check suite" | same |
| `ROADMAP.md:164` | "~390 automated checks across 8 phases" | same |
| `ROADMAP.md:246` | "total 257 → 261" | stale |
| `--quick` run today | **269 across 7 phases** | actual |

**The 8-phases claim is also wrong** in `ROADMAP.md:164` — `--quick`
runs **7 phases** (build phase is implicit, compiler-tests is gated
behind non-`--quick`). The numbering "8 phases" was correct *before*
the build phase was de-emphasised; today the runner outputs 7 phase
headers in `--quick`.

This is fixable two ways:

- (preferred) replace all four with the canonical phrasing: *"every
  example, every phase — see `node test/run-all-tests.mjs --list`"* —
  same pattern `testing.md:14` already uses
- (alternative) extend the doc-drift sentinel with a new
  `check_test_counts()` that runs the runner with `--list` and compares
  any `\d+\s*automated checks` regex it finds in active docs against
  the live count

**Examples-count narrative drift** in `ROADMAP.md:17,138`: see §11.
`README.md` is correct (55), so the sentinel is letting ROADMAP-narrative
drift through.

**Tutorial coverage** is good: 12 tutorials in `docs/tutorials/`
(graphics, sprites, animation, scrolling, input, collision, audio,
game_states, sa1, colormath, dma, hdma, math). Hardware-reference docs
in `docs/hardware/` (MEMORY_MAP, OAM, REGISTERS). The ROADMAP claim
about Doxygen + tutorials + hardware refs (`ROADMAP.md:178-188`) is
honest.

**Migration guide PVSnesLib → OpenSNES**: ROADMAP names this as a "must
have" for v1.0 (`ROADMAP.md:217`) and lists it as "Not started". This
is the single biggest doc gap given the PVSnesLib migration positioning
in `PHILOSOPHY.md:38-41`. A dedicated `docs/PVSNESLIB_MIGRATION.md` with
the calling-convention swap, the function-name table, and the
"silent-failure differences" list would let users actually act on the
positioning claim.

---

## 13. Project hygiene

| Aspect | Status |
|---|---|
| License | MIT, clean (`LICENSE` file at root) |
| Attribution | `ATTRIBUTION.md` lists PVSnesLib, QBE, cproc, WLA-DX, SNESMOD, Mesen2 — accurate |
| `.editorconfig` | **missing** — every contributor configures their own indent |
| Pre-commit hooks | none — CI-only enforcement |
| Conventional Commits | enforced via `lint_commits.py` (whitelist of types/scopes; rejects `Co-Authored-By:` trailers) |
| `lint_asm.py` | `.ACCU` / `.INDEX` markers — clever, project-specific |
| Code style | `docs/CODE_STYLE.md` documents 4-space, K&R, snake_case |
| GitHub templates | issue/PR templates present |

**`compiler/wla-dx` working tree pollution** (🟡 polish). Working tree:

```
$ cd compiler/wla-dx && git status --short
 M tests/68000/all_instructions_test/main.s
?? .wla-built/
?? ins_tbl_gen/
```

The HEAD SHA matches `PINS.md` (verified via `verify-toolchain`), but
the working tree carries:

- A modified test file (likely a debug edit)
- Two untracked directories (build artifacts that should be in
  `.gitignore`)

`make verify-toolchain` only checks SHA, not working-tree cleanliness.
A fresh `git submodule update --init --recursive` clones cleanly, but
anyone running `make` in a working checkout could carry these
modifications into a release build silently. Two minor fixes:

1. Extend `verify_toolchain.py` to also fail on dirty working trees
   (`git diff --quiet HEAD` per submodule).
2. Add `.wla-built/` and `ins_tbl_gen/` to `compiler/wla-dx/.gitignore`
   *upstream* (Vhelin's repo) — would clean up the noise for everyone.

Neither is shipping-blocking. Both are the kind of thing a senior
reviewer would mention without making it a hill to die on.

**No `.editorconfig`**: minor. Most editors infer indent from existing
files. A 4-line `.editorconfig` would be one less per-contributor
configuration step. One-PR fix.

---

## 14. Performance story — the 30 % claim

`docs/BENCHMARK.md` is real, dated 2026-03-27, 60+ lines. Methodology
section (`:21-37`):

- 34 isolated benchmark functions in
  `tools/opensnes-emu/test/fixtures/benchmark/bench_functions.c`
- Both compilers process the same source file
- PVSnesLib path: `816-tcc` + `816-opt` (38 peephole rules)
- OpenSNES path: `cc65816` (cproc + qbe -t w65816 with 13 optimization
  phases)
- Cycle counts via `devtools/cyclecount/cyclecount.py`, 65816 datasheet
  values, 16-bit A/X/Y, no page-crossing penalties

Result row count and standout numbers (sampled):

| Function | PVS+opt | OpenSNES | Δ |
|---|---|---|---|
| `add_u16` | 34 | 21 | **−38.2 %** |
| `loop_sum` | 185 | 124 | **−33.0 %** |
| `array_read` | 60 | 31 | **−48.3 %** |
| `clamp` | 124 | 66 | **−46.8 %** |
| `call_add` | 41 | 4 | **−90.2 %** |
| `mod_const_10` | 32 | 33 | +3.1 % (only loss) |

OpenSNES wins 32/34, PVS+opt wins 0, ties 0 (one row I didn't see is the
2nd loss; either methodology shows 32-1-0 or there are 2 losses I
missed). The aggregate `−30.3 %` claim is the unweighted mean
across the 34 rows, which is a defensible methodology.

Things I'd push on harder:

- **No "real game" benchmark.** All 34 functions are synthetic isolated
  patterns. A "compile likemario, count NMI cycles per frame" comparison
  would be the *credibility* number, not a synthetic average. ROADMAP
  P3.2 would actually surface that: a real-game cycle-count line in the
  benchmark page.
- **The cycle-cost methodology assumes no page crossings.** Realistic
  ROMs hit pages all the time. Documenting that the −30 % is a
  best-case figure (or providing a worst-case bound) would close the
  gap. As-is, the −30 % claim survives a pedant's reading because the
  methodology is signed; a marketer would happily round to "twice as
  fast" and that would be wrong.
- **The benchmark gate (chantier E2) protects regression but doesn't
  measure progression.** No CI alert on "you got 5 % faster, document
  it in CHANGELOG." That's a nice-to-have, not a defect.

The framework opt-in cost is also documented:
`PHILOSOPHY.md:91-100` says ~25 cycles per VBlank for the no-op NMI
hook indirection if you opt out of the dynamic sprite engine.
`templates/crt0.asm` confirms this with the comment alongside
`dynamic_flush_hook`. This number is verifiable — anyone with `cyclecount`
can re-derive it. Real engineering.

---

## 15. Strategic risks & punch list

### Top 10 issues, severity-ordered

| # | Severity | Issue | Symptom | Root cause | Fix sketch |
|---|---|---|---|---|---|
| 1 | 🟠 | Doc check-count drift | Four numbers in four places, all wrong | Sentinel doesn't cover prose | Replace all four with "see `--list`" or extend `check_doc_drift.py` |
| 2 | 🟠 | `font2snes` C/Python collision | Two tools, one name | No naming policy when adding `devtools/` | Rename Python (`font_preprocess`?) + add `tools/README.md` cross-ref |
| 3 | 🟠 | Lib `sprite` and `text` C/ASM duplication | Two implementations per module | Inherited from PVSnesLib + partial migration | ROADMAP P3.2 — 2-3 weeks of perf-critical cleanup |
| 4 | 🟡 | mapscroll bank-$00 razor-thin | 12 bytes from build break | Const string accumulation in `lib/source/console.c` | Combine const arrays in mapscroll, or move scrollbox text to RAM |
| 5 | 🟡 | `verify-toolchain` ignores dirty submodule trees | Local mods could ship in release | SHA-only check | `git diff --quiet HEAD` per submodule in `verify_toolchain.py` |
| 6 | 🟡 | `oamSetFast` macros documented but unused | API exists, examples ignore | Doc/example divergence | Convert breakout + collision_demo to use macros, OR retire the macros |
| 7 | 🟡 | `snes.h` self-contradiction (master vs specialty) | "Include this for all" doc but 8 specialty headers separate | Header convention drift | Split master into `snes.h` (core) + `snes_full.h`, OR rephrase docstring |
| 8 | 🟡 | `tools/valgrind-static.supp` orphan | Unreferenced file at toolbox top | Leftover from debugging session | Delete or move under a tool that uses it |
| 9 | 🟡 | WLA-DX SLOT warnings (7 per build) | Noise pollutes every build | Memmap config picks up SLOT-0 + SLOT-1 with same start | Restructure memmap, or filter the warning |
| 10 | 🟡 | No `.editorconfig` | Per-contributor indent config | Minor hygiene gap | One 4-line PR |

### Punch list (before v1.0)

**Must-have** (already in `ROADMAP.md:213-219`):
- [ ] PVSnesLib → OpenSNES migration guide (single biggest doc gap given
      the positioning claim)
- [ ] Hardware verification on real SNES (FXPak Pro)
- [ ] Showcase game (in progress, RPG project)
- [ ] FAQ
- [ ] dynamic_map dead-code cleanup (`convertC64Sprite`)

**Add to must-have** (this review):
- [ ] Doc check-count drift fix (one-PR)
- [ ] `font2snes` collision (one-PR)
- [ ] Sprite/text duplication audit (P3.2 — 2-3 weeks, the elephant)

**Nice-to-have:**
- [ ] Pixel mode (Mode 3 direct drawing)
- [ ] Tiled map editor integration
- [ ] Project scaffolding (`opensnes init`)

**Icebox:**
- [ ] Video tutorial series (rate-of-return is low compared to a written
      migration guide)

### What I'd delete or consolidate

- `tools/valgrind-static.supp` — delete or relocate
- The phrase "~390 automated checks" wherever it lives — replace with
  canonical `--list` reference
- `KNOWN_LIMITATIONS.md:301-307` claim that examples "switched to direct
  oamMemory[] writes" is true but the macros (`oamSetFast`) are
  documented as the path. Either retire the macros or convert two
  examples — one of these has to go.

### What I'd write

- `docs/PVSNESLIB_MIGRATION.md` — the calling-convention table, the
  function-name compat list, the 5 silent-failure differences
- A "real game" benchmark in `docs/BENCHMARK.md` (likemario or breakout
  cycle-per-frame counter)
- A short `compiler/MAINTAINER.md` documenting the patch-set rebase
  procedure for future maintainers (the implicit knowledge in the
  current 38-patch list is a single-bus-factor risk)

---

## 16. Comparative positioning

### vs PVSnesLib

| Concern | PVSnesLib | OpenSNES |
|---|---|---|
| Compiler | tcc816 (C89, right-to-left push) | cc65816 (C11, left-to-right push, modern semantics) |
| Performance vs PVS | baseline | −30 % cycles on 34-fn benchmark suite |
| NMI/VBlank | user-managed | auto-orchestrated (auto-flush, scroll sync, OAM DMA) |
| Quirk handling | exposed | hidden + documented (KNOWN_LIMITATIONS) |
| Test infrastructure | lightweight | 269+ checks, multi-phase CI, visual regression, lag detection, cycle gate |
| Asset model | bring-your-own | battery-included (default font, asset pipeline opt-in) |

**When to choose PVSnesLib instead of OpenSNES:**
- You need a *thin* C-over-asm wrapper and want to write 70 % of your
  game in 65816 assembly with C just for control flow
- You need bit-for-bit cartridge compatibility with an existing PVSnesLib
  project's binary layout
- You want a mature, deeply-tested base that has shipped multiple games
  (PVSnesLib has more shipping titles)

**When OpenSNES wins:**
- You're starting fresh and want C-as-primary-language
- You want CI-enforced quality gates from day one
- You care about predictable cycle costs with build-time enforcement

### vs cc65/CA65 (NES ecosystem reference)

cc65/CA65 is the canonical comparison for retro C compilers, on a
different target (6502/NES). Both are forks of work by Michael Forney
(cproc) and Ullrich von Bassewitz (cc65). cc65 has 30+ years of code
and a vastly larger ecosystem; OpenSNES has 1-2 years of code and a
deeper compiler patchset for its specific target.

OpenSNES wins on: modern C11 semantics, integrated test infrastructure,
explicit cycle gates, framework opt-ins.

cc65/CA65 wins on: ecosystem depth, third-party libraries, shipping
games, multi-target support.

Apples and oranges, but the parallel is instructive: a 30-year-old
retro compiler ecosystem is what OpenSNES could become if maintained
deliberately. The pin discipline and patchset documentation suggest
that's the intent.

### vs hand-written 65816 ASM

The 90/10 reality: a competent 65816 ASM programmer will write code
that beats cc65816's output for any given inner loop, by a factor of
1.5×-3×. The benchmark numbers are vs PVSnesLib + 816-opt (a C
compiler), not vs hand-rolled ASM.

OpenSNES doesn't claim to beat hand ASM, and shouldn't. The pitch
(`PHILOSOPHY.md:14-17`) is "ship a game in C, drop to ASM for the inner
loops you care about." That's the right framing — and the fact that
the SDK ships 9 348 LOC of 65816 ASM in `lib/source/` for exactly the
inner-loop work (audio, sprite_dynamic, snesmod, map, mode7) confirms
the framing in code.

---

## 17. Final verdict

**Grade: B+ / "late-beta SDK with senior-engineer hygiene"**.

OpenSNES is what happens when someone takes PVSnesLib seriously, adds a
modern C11 compiler with 38 carefully-documented downstream patches,
wraps it in build-time guard rails for every silent-failure trap they've
encountered, and then writes the documentation that names what's been
hidden and where the escape hatch lives. The five-principle framework
in `PHILOSOPHY.md` is unusually disciplined for a hobby SNES project,
and — credit where due — the principles actually shape the API.

**Who it's for:** developers with prior 65816 / SNES / PVSnesLib
experience who want to write game logic in C without hand-rolling
NMI handlers, who care about predictable cycle costs, and who can
read 65816 assembly when the lib's escape hatches force them to.

**Who it's not for:** newcomers who think "make a game in C" hides the
hardware (it doesn't, and the README is upfront about that); teams
needing a mature production toolchain with shipped commercial titles
(PVSnesLib is your better bet today).

**What would move it to A−:**

1. **Close the sprite/text duplication** (P3.2 — the biggest cleanup
   item, 2-3 weeks of focused work). Halves the bus-factor risk on
   `lib/source/`.
2. **Ship the PVSnesLib migration guide.** The positioning claim in
   `PHILOSOPHY.md:38-41` is a promise to migrating users that no doc
   currently fulfils.
3. **Tighten the doc-drift sentinel** to catch check-count and
   examples-count claims in narrative prose, not just anchored
   locations. Five minutes' work to extend `check_doc_drift.py`, hours
   of doc-cleanup behind it.
4. **Add a real-game benchmark** in addition to the 34 synthetic
   functions. Either pick `likemario` (state-machine + scrolling +
   OAM-heavy) or `breakout` (input-loop + collision + OAM) and report
   cycles-per-frame. That's the credibility number.
5. **Write `compiler/MAINTAINER.md`** — the rebase procedure for the
   38-patch downstream stack. Without it, the bus-factor risk on the
   compiler is "one person who knows the patch order."

**What would move it to A:** ship a complete game on real SNES hardware
using only OpenSNES, document the project, blog the post-mortem. Until
that happens, the SDK is "polished infrastructure looking for its first
shipping title."

The v0.17.0 release — three previously-🔴 silent failures retired,
volatile preserved through QBE, hard cycle gate in CI, NMI WRAM port
race lint — is materially the best release the project has shipped
since the bank-$00 ratchet. Six chantiers like this one back-to-back
and the SDK is in commercial-grade territory.

---

*Reviewed against HEAD `9e7710a` on 2026-05-10 by an engineer who would
have signed off on the API but kicked back the sprite/text duplication
issue at code review.*
