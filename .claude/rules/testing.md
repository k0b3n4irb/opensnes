# Testing Workflow (Auto-loaded)

IMPORTANT: Every change must pass this workflow before commit.

## Test Command

The test harness runs on **luna** (cycle-accurate native emulator, pinned
binary — no Node/WASM/Mesen2). One-shot via `make tests`, or step by step:

```bash
scripts/install-luna.sh                              # fetch pinned luna (tools/luna-test/luna.version)
python3 tools/luna-test/luna_runner.py --coverage    # corpus liveness (NMI/VBlank + CPU state)
python3 tools/luna-test/luna_runner.py --compare     # visual regression (framebuffer SHA-256 vs baselines)
python3 tools/luna-test/probes/run_all.py            # functional probes (scripted input → WRAM asserts)
```

Coverage covers every example (`luna_runner.py --list`). Static analysis
(`symmap.py`), the build (`make`), and compiler C→ASM checks remain separate
make/devtools steps. luna runs SA-1 / Super FX / DSP-1 natively — no chip-ROM
side channel. Migration off snes9x-WASM: `.claude/notes/chantiers/luna_migration.md`.

## Change Classification

| Class | What changed | Required validation |
|-------|-------------|-------------------|
| **A** | Compiler (cproc/qbe/wla-dx) or runtime (crt0, runtime.asm) | `make clean && make` + full `make tests` (luna) on ALL affected examples |
| **B** | Library module (lib/source/) | `make lib` + `make tests` covering examples using that module |
| **C** | Single example or new example | Build that example + `make tests` (`luna_runner.py --only <ex>`) |
| **D** | Docs, Makefile, tools only | `make tests` only |

## 2-Pillar Validation

luna unifies what used to be two emulators (snes9x automated + Mesen2 manual),
so validation is now 2 pillars:

1. **luna** (automated) — `make tests` (coverage + visual regression + probes).
   luna is cycle-accurate and runs the chips, so it is both the automated suite
   and the visual reference (no separate manual Mesen2 pass). For deep
   interactive debugging, luna's GUI / MCP (`luna mcp`) is available.
2. **Full rebuild** — `make clean && make` must succeed with zero warnings.

## Before Commit Checklist

1. `make clean && make` — zero warnings
2. `make tests` (luna coverage + visual regression + probes)
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
   not just every file edited. Class A compiler changes touch every example
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
