---
name: mesen2-rpc input + mem.write surface, functional probe suite expanded
description: Closes the input-injection / mem-write gaps surfaced by the tetris __mul32 bug, then leverages the new surface to grow the functional probe suite from 5 to 17 probes (10 input-driven + 2 no-controller alt-controller detection + 5 observation-only) + 1 boot sweep covering all 55 examples. All 3 input patterns, all 12 SNES joypad bits, the no-controller detection path for both alt-controllers (mouse, superscope), and all 9 lib subsystems touched by examples are now covered.
type: project
---

# mesen2-rpc surface enriched + functional probes expanded — 2026-05-20

The tetris debug marathon on 2026-05-19/20 surfaced two MCP tool gaps
that mvp-demo had explicitly flagged as deferred: **no input injection**
and **no memory write**. Both worked around by patching C sources to
bypass title-screen state machines, which was acceptable for one-shot
debugging but blocked any autonomous regression coverage. This chantier
ships both tools, then uses the new surface to:

1. Grow the input-driven functional probe count from 0 to 10 (covering
   tetris, breakout, likemario, mapscroll, dynamic_map, scene_stack,
   collision_demo, aim_target, random, controller), spanning all three
   input patterns the lib exposes: on-press (D-pad hold), on-release
   (`pad_released`), and just-pressed (`padPressed`). The `controller`
   probe additionally validates every individual SNES joypad bit
   (12 buttons + 3 combos) — the lib's most granular input-decode gate.
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

Submodule head: `7a66f8f` (after the fourth wave of probes — alt-controllers).

- `b47bef8` — first wave of input-driven probes (tetris, breakout,
  likemario):
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

- `1b0c0b5` — second wave of input-driven probes (mapscroll,
  dynamic_map, scene_stack, collision_demo):
  - **mapscroll**: D-pad RIGHT/LEFT advances `xloc` under
    camera-follow; covers `mapUpdateCamera` + column streaming.
  - **dynamic_map**: A button toggles `is_map32x32` via the
    `pad0_released` mask — first probe to exercise the on-release
    pattern. The handler does 4× WaitForVBlank + 4× smapDma; the
    probe waits 60 frames between presses to let it complete.
  - **scene_stack**: START on title pushes counter (`scene_top` 1→2),
    START on counter pops (`scene_top` 2→1). Tests `scenePush` +
    `scenePop` + `padPressed()` just-pressed mask end-to-end.
  - **collision_demo**: D-pad RIGHT/DOWN advances `player_x`/`player_y`
    through the open center of the arena; coarse smoke detector for
    the `collideRect` + `collideTile` path.

- `75c1ba6` — third wave of input-driven probes:
  - **aim_target**: D-pad RIGHT/UP advances `target_x`/`target_y`
    under per-axis clamps (247 / 0). Despite the name, the example
    is the canonical `fixCos`/`fixSin`/`atan2_8` math demo; the probe
    catches the upstream "target frozen" failure that would make the
    trig readout unobservable.
  - **random**: A button advances the LFSR-based `current_value`
    via the lib's `rand()`. Two independent presses against the
    boot snapshot; combined flake rate ~2e-10 per run.
  - **controller**: the heavyweight — iterates all 12 SNES buttons
    plus 3 multi-button combinations, injects each alone via
    `input.set`, asserts `pad_keys[0]` exactly equals the expected
    mask. The lib's most granular input-decode gate: a single-bit
    regression in `lib/source/input.asm` would surface here. Error
    message names the XOR'd bits so the failure tells you which
    button is broken.

- `7a66f8f` — fourth wave: alt-controller no-controller detection
  probes (negative-path only):
  - **mouse**: asserts `mouseConnect[0]`, `mouse_x[0]`, `mouse_y[0]`,
    `mouseButton[0]` all stay 0 when no mouse is plugged. Catches a
    regression in the mouse-signature check at crt0.asm:1402-1404
    that would falsely report a mouse and produce garbage
    displacements via the bit-bang loop.
  - **superscope**: asserts `scope_con` + all 4 position vars
    (shoth/shotv + raw) + all 3 button vars (down/now/held) stay 0
    when no scope is plugged. Catches the false-positive that would
    start latching ghost PPU H/V counters every NMI.
  - **Limitation**: positive-path testing (actual mouse delta / scope
    fire) requires a Mesen2-side extension —
    `SnesDebugger::ProcessInputOverrides` currently only routes to
    `SnesController` via `dynamic_pointer_cast`. The mem.write
    injection trick used elsewhere doesn't work here: the NMI
    handler resets `mouseConnect` / `scope_con` every cycle when no
    actual hardware signature is detected, overwriting any C-side
    write before the example's main loop reads it. Tracked as
    follow-up.

### Parent develop pointer bumps

- `b6e655d`, `8403941`, `961c5f7`, `eaa195c`, `eba4329`, `d4cc2a0`,
  `e2abe08`, `10a0628`, `32a09da`, `3de8932` — each bumps the
  opensnes-emu submodule pointer to track the probe-suite
  progression. Plus the orphan `b5_fix32_orbit_sketch.md` preserved
  at `b6e655d` (was an untracked draft inside the emu submodule's
  wrong directory). `eba4329` filed this chantier note; `e2abe08`
  and `32a09da` updated it for waves 2 and 3.

## Functional suite, before vs after

| Phase     | Before chantier | After chantier |
|-----------|--------------|-------------|
| Probes    | 5 (hdma, dma, cgram, oam, scroll) | 17 + 1 sweep covering 55 examples |
| Inputs    | 0 (all observation-only)         | 10 input-driven + 2 negative-path alt-controller |
| Input pattern coverage | n/a       | on-press (D-pad hold), on-release (`pad_released`), just-pressed (`padPressed`) — all three lib masks |
| Joypad-bit coverage | n/a              | every individual SNES button (12 buttons + 3 combos via `controller.test.mjs`) |
| Alt-controller coverage | n/a          | no-controller detection path for mouse + scope (false-positive guard) |
| Address   | Hardcoded literals               | Parsed from `.sym` at module load |
| Runtime   | ~10s for 5 probes                | ~57s for 17 probes + ~170s sweep |
| Failure mode on lib bss shift | Silent (assertions hit wrong byte) | Clear error: `Symbol "foo" not found in foo.sym` |

### Subsystem coverage matrix

| Subsystem                       | Probe                       |
|---------------------------------|-----------------------------|
| CGRAM upload (dmaCopyCGram)     | cgram                       |
| VRAM upload (dmaCopyVram)       | dma                         |
| HDMA channel + enable           | hdma                        |
| OAM shadow buffer + Y quirk     | oam                         |
| BG scroll cache                 | scroll                      |
| Map engine (camera-follow)      | mapscroll                   |
| Dynamic VRAM re-init            | dynamic_map                 |
| Scene stack (push/pop)          | scene_stack                 |
| AABB + tile collision           | collision_demo              |
| Game state machine + input      | tetris, breakout, likemario |
| Trig LUT host (fixCos/atan2_8)  | aim_target                  |
| LFSR PRNG (rand)                | random                      |
| Per-bit joypad decode           | controller                  |
| Mouse no-controller detection   | mouse                       |
| Scope no-controller detection   | superscope                  |
| NMI + force-blank avoidance     | boot-sweep (× 55 examples)  |

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

5. **Probe timing must respect the example's handler cost**. Probes
   that drive an action requiring heavy work in the C handler
   (dynamic_map's toggle: 4× WaitForVBlank + 4× smapDma; ~30 frames
   blocking) need a settle window longer than the handler — otherwise
   the *next* press is consumed during the still-running prior
   handler. First version of the dynamic_map probe used 8 frames
   between presses; the second toggle silently missed. 60 frames
   worked. **Rule of thumb**: if the C handler does ≥1 WaitForVBlank,
   the inter-press settle should be ≥ (handler_vblanks × 2) frames.

6. **The MCP surface enrichments paid off in the same session**. Both
   input.set and mem.write_* were shipped at the start of the session
   and applied within the same hours to the probe suite. No deferred
   ROI: the tools are already eating their cost.

## Partial alt-controller surface (2026-05-21)

Mesen2 commit `2f118512` on `dev/rpc-server` ships the **infrastructure**
for positive-path SnesMouse / SuperScope overrides, but the actual
dispatch is disabled pending a heap-corruption fix.

### What ships and works

- **`controller.connect(port, type)` RPC method**: hot-swaps Port1/Port2
  controller type at runtime via SnesConfig + a new
  `ConfigApi.RefreshControlDevices()` DllExport. Types: `"controller"`,
  `"mouse"`, `"scope"`, `"none"`. The C++ side calls
  `SnesControlManager::UpdateControlDevices()` which sees the config
  diff and rebuilds the device list.
- **`DebugMouseOverride` / `DebugScopeOverride` structs** (C++ in
  `Core/Debugger/DebugTypes.h`, C# in `UI/Interop/DebugApi.cs`).
  Kept separate from the cross-console `DebugControllerState` to avoid
  perturbing its layout.
- **`SnesMouse::SetMovementFromOverride(MouseMovement)` and
  `SuperScope::TriggerLatchAfterOverride()`** public wrappers around
  protected base methods.
- **`SuperScope::Buttons` enum** lifted from `private` to `public` so
  the debugger can name the bits.
- **TS client `controller.connect()` method** + MCP
  `snes_controller_connect` tool.

### What ships but currently errors

- `input.set_mouse(port, dx, dy, left, right)` — RPC method present;
  returns JSON-RPC error -32601 "not yet implemented" with a
  pointer to the heap-corruption issue.
- `input.set_scope(port, x, y, fire, cursor, turbo, pause)` — same shape.
- TS `input.setMouse()` / `input.setScope()` exposed; surface the
  C# error.
- MCP `snes_input_set_mouse` / `snes_input_set_scope` tool defs
  match.

### The heap-corruption bug

First implementation added override arrays to `Debugger.h`:
```cpp
DebugMouseOverride _mouseOverrides[8] = {};
DebugScopeOverride _scopeOverrides[8] = {};
```

This caused `free(): invalid pointer` / `malloc(): unaligned tcache
chunk detected` on subsequent joypad `input.set` calls — *before* any
mouse/scope override was issued. The corruption surfaced through
spawn-mesen.mjs but NOT through manual command-line launches of the
same binary calling the same RPC sequence. Unclear root cause.

Hypotheses:
1. C# `DllImport`-marshalled struct layout doesn't match C++ default
   alignment for the new `int16_t`+`bool` mixed-size structs.
2. Some downstream consumer of the `Debugger` object's member offsets
   assumes the pre-extension layout (no other consumer was identified
   in a manual code search).
3. Test harness state (spawn-mesen.mjs's `stdio: ["ignore", "ignore",
   "pipe"]` vs. manual TTY launch) interacts with the new code path.

### Resolution paths

1. **`#pragma pack(1)` / `[StructLayout(Pack=1)]`** on the new structs
   to remove platform-dependent alignment ambiguity.
2. **Heap-allocate the override arrays** (`unique_ptr<DebugMouseOverride[]>`)
   so the `Debugger` object's own size doesn't change.
3. **Static state instead of `Debugger` members** — keep the override
   tables in a separate translation unit.

Recommended: try path 2 first (least invasive, sidesteps any potential
struct-alignment issues entirely).

## Pending follow-ups

- **Wire functional probes into the canonical `--quick` path?** Task
  #112 was deferred (separate is cleaner; quick stays snes9x-only,
  ~30-60s; functional stays Mesen2-based, ~40s + sweep). Re-evaluate
  if a future regression class is *only* catchable via the functional
  surface and the user wants pre-commit gating.

- **Alt-controller positive-path probes** (mouse delta consumption,
  Super Scope fire latching, sensitivity changes). **PARTIAL**: the
  Mesen2-side surface infrastructure is in place on 2026-05-21 (commit
  `2f118512` on `dev/rpc-server`), but the actual dispatch is disabled
  pending a heap-corruption bug fix. See the "Partial alt-controller
  surface" section below.

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

## Branch / commit state at end of chantier

- **opensnes develop**: `3de8932` (10 commits since the start of the
  cleanup-and-extend phase: baselines bump → export warnings fix →
  probes bump → boot-sweep bump → chantier note → 2nd wave bump →
  chantier note update → 3rd wave bump → chantier note update →
  4th wave alt-controller bump).
- **opensnes-emu feat/functional-probes**: `7a66f8f` (7 commits:
  baselines refresh → input probes wave 1 → sym-parsing refactor →
  boot sweep → input probes wave 2 → input probes wave 3 →
  alt-controller no-controller probes).
- **mesen2-rpc dev/rpc-server**: `2f118512` (input.set + mem.write_*
  earlier; `2f118512` adds the partial alt-controller surface —
  controller.connect works, input.set_mouse/scope return -32601
  pending heap-corruption fix).
