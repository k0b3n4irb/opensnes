# Interactive Debugging with luna {#tutorial_debugging}

luna is not just the test harness — it is OpenSNES's interactive debugger.
Its headless CLI and its MCP server expose the full machine state (CPU, PPU,
APU, DMA, every memory space) to scripts and AI agents, with no Lua layer
and no external emulator required.

This guide shows the standard debugging workflows. Since luna v1.6.0 the
debugger is feature-complete for these: first-class breakpoints
(`bp_add` / `run_until_break`), native WLA-DX symbol support
(`load_symbols` / `resolve_symbol` — debug by variable name), MCP-side
disassembly, save/load states and CPU/memory traces.

## Setup

The pinned binary is installed by the SDK:

```bash
scripts/install-luna.sh            # → tools/luna-test/bin/luna
```

Two ways to drive it:

- **CLI** — one-shot commands (`luna state`, `luna assets-dump`, …).
- **MCP** — a live debugging session for Claude Code or any MCP client:

```bash
claude mcp add luna -- tools/luna-test/bin/luna mcp
```

The MCP session exposes the full debugger surface: run control
(`load_rom`, `reset`, `step`, `step_until_frame`, `run_until_pc`,
`run_until_mem_write/read`), **breakpoints** (`bp_add`, `bp_remove`,
`bp_list`, `bp_clear_all`, `run_until_break`), **symbols**
(`load_symbols`, `resolve_symbol`), memory (`peek_memory`, `poke_memory`,
`search_memory`, `peek_vram`, `peek_aram`, `peek_cgram`), inspection
(`state`, `screenshot`, `disasm_cpu`, `disasm_spc`,
`render_tilemap/vram_tiles/palette/sprite_sheet`), traces
(`enable/take_cpu_trace`, `enable/take_mem_trace`), save/load states and
input (`set_joypad`, `set_mouse`, `set_superscope`, `set_cpu_register`).

## Working with symbols

Every build produces a wlalink `.sym` next to the ROM. Load it once per
session (`load_symbols` over MCP) — every address-taking tool then accepts
variable and function names directly: `peek_memory` on `monster_x`,
`bp_add` on a function label. `resolve_symbol` answers one-off lookups,
and `disasm_cpu` output is symbol-annotated. (The manual fallback stays a
one-liner: `grep -i ' monster_x$' game.sym`.)

## Recipe 1 — Inspect the machine at a point in time

```bash
tools/luna-test/bin/luna state -n 3000000 --screenshot /tmp/shot.png game.sfc
```

The JSON snapshot contains the full CPU registers, PPU state (scroll, mode,
windows, complete CGRAM and OAM), DMA channels, scheduler (frame/NMI
counts) — everything the Mesen2 watch panel showed, machine-readable.
Over MCP: `load_rom` → `step` → `state`.

## Recipe 2 — "Who writes this variable?"

The classic corruption hunt, over MCP:

1. `load_rom`, `load_symbols`, then `bp_add` (write watchpoint on the
   variable's name) and `run_until_break`.
2. Execution halts on the writing instruction at full speed: `state`
   gives the PC, `disasm_cpu` shows the guilty code with symbol
   annotations, `peek_memory` confirms the new value.
3. `run_until_break` again to catch the next writer.

This replaces the old snesdbg `dbg.watch()` callback — the "callback" is
simply whatever you (or the agent driving MCP) do at each stop.

## Recipe 3 — Break at a function

`bp_add` on the function's label, `run_until_break`, then single-step
with `step` and follow `disasm_cpu` / `state.cpu` to trace the execution
path. Multiple breakpoints can be armed at once (`bp_list` shows them).

## Recipe 4 — Sprite/OAM debugging (shadow buffer vs hardware)

The SDK keeps a shadow OAM (`oambuffer`) that the NMI handler DMAs to the
PPU. When a sprite misbehaves, compare the two sides:

```bash
tools/luna-test/bin/luna assets-dump -n 3000000 --out /tmp/dump game.sfc
```

This writes `oam.json` (the hardware side, parsed), the sprite sheet, the
VRAM tile sheet, the four BG tilemaps, the CGRAM palette and the composited
screen as PNGs. Read the shadow side with `peek_memory` at the `oambuffer`
symbol and compare entry by entry — a mismatch means the OAM DMA didn't run
(check `oam_update_flag`) or wrote stale data.

`state` also embeds the full parsed OAM (`ppu.oam_full`), so a pure-MCP
comparison needs no file round-trip.

## Recipe 5 — Poke and observe

Test a hypothesis without rebuilding: `poke_memory` the variable,
`step_until_frame`, `screenshot`. Example: force a player's X position and
watch whether the sprite follows (if it doesn't, the shadow buffer isn't
being flushed).

## Recipe 6 — Temporal artefacts (flicker, page-flip desync)

A single screenshot can't show a frame-to-frame blink:

```bash
tools/luna-test/bin/luna frames -n 3000000 --count 8 --out /tmp/frames game.sfc
```

captures strictly consecutive PPU frames, each tagged with its frame number
and forced-blank state.

## Recipe 7 — the ROM tells you itself

Everything above is the host asking questions of the ROM. `<snes/debug.h>`
is the other direction: three one-line tools that let the ROM speak, and
that cost nothing on real hardware.

```makefile
LIB_MODULES += debug
```

```c
#include <snes/debug.h>

SNES_NOCASH("level 2 loaded");      /* a line in luna's nocash log */
SNES_ASSERT(enemy_count <= MAX);    /* breaks if the invariant fails */
SNES_BREAK();                       /* break here, unconditionally */
```

- `SNES_BREAK()` emits `WDM $00`. luna captures it as an assert event and
  Mesen2 halts on it. On a real console `WDM` is a two-byte no-op, so the
  call is inert rather than harmful if it ships.
- `SNES_NOCASH(msg)` writes a null-terminated string byte by byte to the
  debug port at `$21FC`, which luna, Mesen2 and no$sns all log. It takes a
  plain string — there is no format-string support.
- `SNES_ASSERT(cond)` is `SNES_BREAK()` behind a test, and compiles to
  nothing when `NDEBUG` is defined, so assertions can stay in the source.

**The harness already listens.** Every visual-regression capture runs with
`--wdm-out`, and the runner records whether the channel fired: an assertion
that trips during a corpus run shows up as a failure without anyone writing
a test for it. Manifests can also assert on it directly — `wdm_empty` for
"nothing tripped", `nocash_contains` for a specific message. So an invariant
you write as `SNES_ASSERT` in your game is checked on every push, for free,
by the same pass that compares pixels.

## What Mesen2 is still for

Nothing is *required* anymore: the workflows above cover the watch /
breakpoint / OAM / poke use cases headlessly. Third-party GUI emulators
(Mesen2, bsnes) remain fine for eyeballing gameplay, and Mesen2 stays cited
in the hardware docs as the accuracy reference that settled some SDK
behaviours (e.g. SA-1 SIWP polarity) — a historical role, not a dependency.
