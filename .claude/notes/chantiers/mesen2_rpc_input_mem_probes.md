---
name: mesen2-rpc input + mem.write surface, functional probe suite tripled
description: Closes the input-injection / mem-write gaps surfaced by the tetris __mul32 bug, then leverages the new surface to triple the functional probe suite and add an autonomous boot sweep across all 55 examples.
type: project
---

# mesen2-rpc surface enriched + functional probes expanded — 2026-05-20

The tetris debug marathon on 2026-05-19/20 surfaced two MCP tool gaps
that mvp-demo had explicitly flagged as deferred: **no input injection**
and **no memory write**. Both worked around by patching C sources to
bypass title-screen state machines, which was acceptable for one-shot
debugging but blocked any autonomous regression coverage. This chantier
ships both tools, then uses the new surface to:

1. Triple the input-driven functional probe count (1 → 4, then to 4 + 1
   sweep = 5 covering all 55 examples).
2. Refactor the entire probe suite to parse `.sym` files at runtime
   instead of hardcoding addresses, killing the silent-drift class that
   bit us when mul32/div32 RAMSECTION shifted bank-0 layout.
3. Confirm via autonomous sweep that all 55 examples are structurally
   healthy beyond the screenshot frame (NMI alive, screen on).

**Why:** Without input + mem.write, the MCP surface could only
*observe* — it couldn't *steer*. Steering is what makes a probe a
gate-keeper instead of a one-shot diagnostic. The whole point of the
mesen2-rpc chantier was to convert hours of human-driven Mesen2 GUI
work into seconds of autonomous validation; that promise can only
land for input-gated examples (every game, every interactive demo)
once the input + mem.write tools exist.

**How to apply:**
1. **Authoring new probes**: import `loadSymbolsForRom` from
   `test/functional/lib/symbols.mjs`, get addresses by name, use
   `client.input.set(port, buttons)` to drive scenarios, `client.mem.write*`
   if you need to skip state. **Do not** hardcode 16-bit addresses — that
   class of fragility is now solved.
2. **Suspecting a regression**: run `npm run test:functional` from the
   emu submodule. The boot sweep catches structural breakage across the
   tree; the per-example probes catch interactive logic breakage. The
   8 hand-written probes return results in ~25s; the sweep adds ~170s.
3. **Adding a new interactive example**: write a probe that drives its
   golden path via `input.set` and asserts the visible state change
   (sprite position, game state, score). The sym-parser makes this
   ~30 lines for typical cases.

## What shipped

### Mesen2-rpc TS server side (`k0b3n4irb/mesen2-rpc:dev/rpc-server`)

- `464ef21e` — `feat(rpc): add input.set tool` — wraps
  `DebugApi.SetInputOverrides`, port 0..7, SNES joypad mask (B=$8000,
  Y=$4000, SELECT=$2000, START=$1000, UP=$0800, DOWN=$0400, LEFT=$0200,
  RIGHT=$0100, A=$80, X=$40, L=$20, R=$10). Passing 0 releases.
- `b9a22115` — `feat(rpc): add mem.write_byte / word / range` —
  symmetric with the read counterparts; bypasses CPU memory protection
  via `DebugApi.SetMemoryValue(s)`. Hex string for `mem.write_range`
  (even-length, no separator).

Branch head: `7749511` (after subsequent ROI work; see Probes section).

Total deferred-tool count: **31**.

### Lib fix (develop)

- `40f2691` — `fix(lib): drop spurious .EXPORT in mul32/div32` — closes
  the 6 build warnings (`Trying to export an unknown definition`). WLA-DX
  `.EXPORT` is for `.DEFINE` symbols, not labels; labels in FREE sections
  are visible across object files automatically. The post-mul32 alias
  fix had carried over the export-the-label idiom from the old
  `.DEFINE` aliasing pattern; dropping the directives fixes the warning
  noise without changing linkage.

### Functional probes (opensnes-emu submodule, feat/functional-probes)

Submodule head: `7749511` (after sym-parsing refactor + boot sweep).

- `b47bef8` — input-driven probes for tetris, breakout, likemario:
  - **tetris**: START transitions `game_state` 0→1; LEFT decrements
    `cur_col`. Symmetric ROI proof for the same `__mul32` bug class.
  - **breakout**: D-pad RIGHT/LEFT moves paddle (`px`).
  - **likemario**: D-pad RIGHT advances `mario_x`; A button produces
    `mario_yvel < 0` (upward jump impulse).
  - Plus refresh of 3 hardcoded addresses in cgram/dma/hdma that drifted
    after the mul32 RAM layout change.

- `7ebea9a` — `refactor(test): parse .sym at probe runtime`.
  New `test/functional/lib/symbols.mjs` exposing `loadSymbolsForRom`,
  with `sym.addr(name)` / `sym.size(name)` that throw clear errors
  when symbols are missing or moved. All 8 probes refactored. Pure
  hardware constants (MMIO regs, joypad masks) stay as literals.

- `7749511` — `feat(test): autonomous boot sweep across all examples`.
  Single probe (`boot-sweep.test.mjs`) iterates every `.sfc` under
  `examples/`, runs 180 frames per ROM, asserts `frame_count` advances
  (NMI alive) and `current_brightness > 0` (screen on). One spawn, N
  loadRom calls. **55/55 examples currently green in ~170s**. Skippable
  via `SKIP_BOOT_SWEEP=1` for fast iteration.

### Parent develop pointer bumps

- `b6e655d`, `8403941`, `961c5f7`, `eaa195c` — each bumps the
  opensnes-emu submodule pointer to track the probe-suite progression.
  Plus the orphan `b5_fix32_orbit_sketch.md` preserved at `b6e655d`
  (was an untracked draft inside the emu submodule's wrong directory).

## Functional suite, before vs after

| Phase     | Before today | After today |
|-----------|--------------|-------------|
| Probes    | 5 (hdma, dma, cgram, oam, scroll) | 8 + 1 sweep covering 55 examples |
| Inputs    | 0 (all observation-only)         | 3 input-driven (tetris, breakout, likemario) |
| Address   | Hardcoded literals               | Parsed from `.sym` at module load |
| Runtime   | ~10s for 5 probes                | ~25s for 8 probes + ~170s sweep |
| Failure mode on lib bss shift | Silent (assertions hit wrong byte) | Clear error: `Symbol "foo" not found in foo.sym` |

## Lessons learned

1. **Hardcoded 16-bit addresses are a silent-fail trap**. The pattern
   `const ADDR = 0xNNNN; // from foo.sym` is irresistible at probe-write
   time (one-line, no setup), but every lib change rolls the dice on it.
   The post-mul32 RAM layout shift broke 3 probes without a single
   probe-code line changing. **Rule of thumb**: if the value is a
   symbol name, resolve it via the sym file. If it's a hardware
   constant (MMIO offset, MMIO flag value, joypad mask), literal is
   fine.

2. **WLA-DX `.EXPORT` is for `.DEFINE` symbols, not labels**. The
   confusion is that the directive is named ambiguously. Labels in FREE
   sections are exported automatically via the section's symbol table;
   you don't need (and shouldn't write) `.EXPORT label_name`. If a
   future label needs to be visible across files, just put it in a
   FREE section. The 6 warnings in the previous wave came from
   carrying over the old `.DEFINE __X tcc_X ; .EXPORT __X` idiom into
   the new direct-label scheme. Drop both lines, not just the
   `.DEFINE`.

3. **Boot-sweep is genuinely orthogonal to visual regression**. They
   sound redundant ("both check examples work after boot") but they
   catch disjoint failure classes:
   - Visual: shape/color of the rendered frame.
   - Boot sweep: NMI vitality + force-blank avoidance over a longer
     time window than the screenshot.
   A regression where an example renders the title frame correctly
   then locks up would slip through visual regression but get caught
   by the sweep. Worth the 170s.

4. **One Mesen spawn, N loadRoms is fast enough**. The first design
   instinct was one spawn per example (clean, isolated, no state
   bleed). Reality: spawning Mesen takes ~3s, but `client.emu.loadRom`
   is essentially free (~50ms) and properly resets emulator state.
   55 spawns would be ~165s of pure setup overhead; 1 spawn + 55
   loadRoms is ~170s total. The savings buy headroom for adding more
   per-ROM checks without bloating wall time.

5. **The MCP surface enrichments paid off in the same session**. Both
   input.set and mem.write_* were shipped at the start of the session
   and applied within the same hours to the probe suite. No deferred
   ROI: the tools are already eating their cost.

## Pending follow-ups

- **Wire functional probes into the canonical `--quick` path?** Task
  #112 was deferred (separate is cleaner; quick stays snes9x-only,
  ~30-60s; functional stays Mesen2-based, ~25s + sweep). Re-evaluate
  if a future regression class is *only* catchable via the functional
  surface and the user wants pre-commit gating.

- **More input-driven probes** for currently-uncovered interactive
  examples: mapscroll (D-pad scroll), dynamic_map (UP/DOWN toggle),
  collision_demo (D-pad sprite move), aim_target (B button shoot),
  scene_stack (push/pop on START).

- **GSU/SA-1 introspection extensions to mesen2-rpc**. The current
  surface is 65816-only. Adding GSU register/PC inspection and SA-1
  CPU2 state would unlock chip-specific probes (SuperFX 3D, SA-1
  hello/starfield). Track under structural defects if it becomes a
  blocker.

- **B5 fixed32 chantier**. The orphan `b5_fix32_orbit_sketch.md` is
  preserved as a starting sketch. When B5 lands, the input-driven
  probe pattern will validate the orbit motion end-to-end.

## Cross-references

- Prior chantier: [[mesen2_rpc_mvp_validated]] (validation of the
  observation-only surface, May 14)
- Plan: `.claude/plans/tender-yawning-cake.md`
- Probe authoring: `tools/opensnes-emu/test/functional/README.md`
- Sym parser source: `tools/opensnes-emu/test/functional/lib/symbols.mjs`
- B5 follow-up: [[b5_fix32_orbit_sketch]]
- Repo: <https://github.com/k0b3n4irb/mesen2-rpc>

## Branch / commit state at end of session

- **opensnes develop**: `eaa195c` (4 commits since the start of the
  cleanup-and-extend phase: baselines bump → export warnings fix →
  probes bump → boot-sweep bump).
- **opensnes-emu feat/functional-probes**: `7749511` (4 commits:
  baselines refresh → input probes → sym-parsing refactor → boot sweep).
- **mesen2-rpc dev/rpc-server**: `b9a22115` (last published 2 commits
  added input.set + mem.write_*; later commits not pushed).
