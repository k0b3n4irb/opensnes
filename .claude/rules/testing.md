# Testing Workflow (Auto-loaded)

IMPORTANT: Every change must pass this workflow before commit.

## Test Command

```bash
cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
```

This runs 8 phases (~390 checks total):
1. **Preconditions** — toolchain and dependencies present
2. **Compiler tests** — 60 C→ASM pattern checks
3. **Build** — all 53 examples compile cleanly
4. **Static analysis** — ROM headers, memory maps, symbol overlaps
5. **Runtime execution** — emulated run of each ROM
6. **Visual regression** — screenshot comparison against baselines
7. **Lag detection** — frame timing verification

## Change Classification

| Class | What changed | Required validation |
|-------|-------------|-------------------|
| **A** | Compiler (cproc/qbe/wla-dx) or runtime (crt0, runtime.asm) | `make clean && make` + full test suite + Mesen2 on ALL affected examples |
| **B** | Library module (lib/source/) | `make lib` + test suite + Mesen2 on examples using that module |
| **C** | Single example or new example | Build that example + test suite + Mesen2 on that example |
| **D** | Docs, Makefile, tools only | Test suite only (no Mesen2 needed) |

## 3-Pillar Validation

Every non-trivial change requires all 3 pillars:

1. **opensnes-emu** (automated) — `node test/run-all-tests.mjs --quick`
2. **Mesen2** (manual) — ask the user to validate visually in Mesen2 emulator
3. **Full rebuild** — `make clean && make` must succeed with zero warnings

## Before Commit Checklist

1. `make clean && make` — zero warnings
2. `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick`
3. **Triage impacted examples — short list, not exhaustive dump.** Apply the
   workflow in the next section (Impacted-Examples Triage). The output is a
   smart-selected list with one "what to look for" line per entry, not a wall
   of paths. Do NOT commit until the user confirms that selection.
4. **For library changes (Class B)**: grep all example Makefiles for the changed
   module name in LIB_MODULES to enumerate the candidate set, then triage.
5. **NEVER commit without user validation.** Do not assume examples work because
   they compiled. Visual validation in Mesen2 is required on the triaged subset.
6. Conventional Commits format in message

## Impacted-Examples Triage

For every change, run this three-step pipeline before asking the user to validate:

1. **Identify** — enumerate every example whose compiled bytes could change,
   not just every file edited. Class A compiler changes touch all 53 examples
   in principle; Class B library changes touch the subset that links the
   changed module; Class C example changes touch only one. Use the diagnostic
   tools available (e.g. `CC_TRACE_TCO` for compiler optimisations, `grep
   LIB_MODULES` for library changes, `find -newer` after a rebuild) to get a
   real list, not a guess.

2. **Triage** — collapse the candidate set down to a short list of representative
   examples, dropping redundant coverage. Heuristics for keeping an entry:
   - covers a **distinct code path** that no other entry exercises
     (e.g. the lone `framesize=2 phantom frame` site for a TCO change);
   - exercises a **specific module combination** (audio + DMA + scrolling)
     other entries don't;
   - is a **lib site invoked from many examples** — pick *one* example using
     it, not all of them;
   - lives in **games** (longer interactive paths with input + state machine)
     when the change touches anything around frame timing or state.
   Drop entries that are pure duplicates of a kept one (same module, same
   shape, no extra coverage).

3. **Present** — give the user the kept list as a small table with one
   "what to look for" per entry. Each line should answer "if this regresses,
   what visible symptom would I expect, and which button or scenario surfaces
   it?" Don't ask the user to fish for bugs blind. Example shape:

   | Example | Why kept | What to look for |
   |---|---|---|
   | `examples/maps/dynamic_map` | exercises 2 framed-tail-call wrappers | press **D** (SNES A) — the map should toggle 32×32 ↔ 64×64 cleanly without flicker |
   | `examples/graphics/effects/mosaic` | only `framesize=2` phantom-frame site in the diagnostic trace | press **A** (SNES Y) — fade-in should complete and stop, no garbage tiles |

   Keep the list to 2–5 entries when possible. If the change genuinely needs
   wider coverage say so explicitly with the reasoning.

**Why triage instead of an exhaustive list:** Class A changes can touch all 53
examples; asking the user to walk through every one is both unrealistic and
wasteful — most are redundant for any given change. The triage forces explicit
reasoning about which axes of coverage actually matter for *this* change, and
the "what to look for" makes the validation an active check (the user knows
the failure shape), not a passive "press buttons and hope".

## Debugging Ported Examples

When a ported example has visual bugs:
1. **Compare with the PVSnesLib original** — build the original PVSnesLib example
   and compare the generated ASM output (stack offsets, VRAM layout, register values)
2. **Check VRAM layout** — sprite tiles, font tiles, and tilemaps must not overlap
3. **Check OBJSEL** — Name Base and Name Select gap determine where tiles 256+ are read
4. **Never guess** — use Mesen2 debugger (VRAM viewer, OAM viewer) to verify actual state
