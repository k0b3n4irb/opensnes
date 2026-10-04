# Chantier — Retire Mesen2 + snesdbg (luna as sole emulator, incl. interactive debug)

**Status: COMPLETE (2026-07-05, same-day round-trip).** Scoped in the
morning (joint read-only audit); luna-side spec filed as
[luna#63](https://github.com/k0b3n4irb/luna/issues/63) → #65 MCP wiring /
#66 breakpoints / #67 .sym / #68 GUI. The luna team shipped #65+#66+#67
**the same day** as **luna v1.6.0** ("interactive-debugger parity").
OpenSNES side, all phases done: Phase 0 (doctor detects luna), Phase 1
(docs/tutorials/debugging.md), snesdbg deleted (Lua gone from the repo),
Phase 2 rules/docs re-pointed (Mesen2 optional), and the pin bumped
v1.1.0 → v1.6.0 with the documented rebaseline (2 visual + 2 WRAM drifts,
all verified as legitimate accuracy churn: HDMA-faithful gradient shift,
timing-phase cube rotation). Acceptance met: the four workflows run over
`luna mcp` with native symbols and first-class breakpoints, no Mesen2,
no Lua. Remaining upstream (non-blocking): #68 GUI step/bp controls.

**Goal:** luna becomes the single emulator for OpenSNES — automated pillars
(done since the luna migration) *and* interactive debugging (today still
Mesen2 + the `devtools/snesdbg` Lua library). Aligns with the maintainer's
API-pruning principle (no superfluous surface) and closes the sister-suite
story: SDK + emulator + debugger, zero external emulator dependency.

## État des lieux (2026-07-05, objective)

### luna side — much closer than assumed

`luna-api::Emulator` (single facade for CLI/GUI/MCP) already has, verified
in-code and battle-tested (Tom Harte suites, golden-ROM tests):

- `poke_memory` + `set_cpu_register` (writes exist — commonly assumed missing)
- `save_state`/`load_state` (bincode, round-trip determinism unit-tested)
- 65C816 **and** SPC700 disassemblers
- `run_until_pc` / `run_until_mem_write/read` (single-step polling, not real bps)
- a real per-memory-access hook in the bus (`trace_mem_access`) + CPU
  instruction trace ring buffer — the substrate breakpoints need
- reads of every space (WRAM/VRAM/CGRAM/OAM/ARAM/SRAM/coproc)
- tilemap/tiles/palette/sprite-sheet PNG renders (headless viewer parity)
- GUI debug panels: CPU/SPC regs, hex viewers, disasm, sprites, palette,
  tilemap, event viewer (pause-only; no bp/step UI)

**The four gaps** (full detail in luna#63): (1) no first-class
breakpoint/watchpoint registry — the one new abstraction; (2) zero `.sym`
support; (3) MCP exposes only 17 of ~80 API methods (~15-25 lines per new
tool); (4) GUI lacks bp/step controls (optional — MCP is the primary path).

### OpenSNES side — what actually depends on Mesen2

- **`devtools/snesdbg/`** (~2,360 LOC Lua + examples): structurally bound to
  Mesen2's `emu.*` API (`memory.lua:52 toMesenAddr`). **Zero automation
  wiring** — no Makefile/CI/test references; purely manual (Mesen2 Script
  Window). Its only emulator-agnostic part (`.sym` parsing) is already
  re-implemented in `tools/luna-test/probes/lib.py`.
- **Workflow rules still making Mesen2 mandatory** (contradicting
  `testing.md`, which already declares luna the sole visual reference):
  `compiler.md:30,44`, `debugging.md:21,42-46`, `release.md:41,95`,
  `templates.md:63`, `regression_method.md:27`, `testing.md:55,110`,
  `.claude/skills/port-example/SKILL.md:287`.
- **User-facing docs recommending Mesen2**: `README.md:136`,
  `CONTRIBUTING.md:16,110`, `GETTING_STARTED.md`, `TROUBLESHOOTING.md`,
  tutorials (sa1/superfx/sram/hdma), `devtools/README.md:58`,
  `tools/README.md:8,58`, `ROADMAP.md:134,201`.
- **`scripts/opensnes` doctor** detects mesen/bsnes/snes9x but **not luna**
  (`:118,344,403,437`) — gap regardless of this chantier.
- **`lib/include/snes/debug.h`**: `SNES_BREAK`/`consoleMesenBreakpoint` doc
  comments name Mesen2 (the WDM mechanism itself is emulator-agnostic —
  luna already treats WDM as an assert channel).
- **Keep untouched**: historical accuracy citations (Mesen2 as the oracle
  that settled SA-1 SIWP `$FF` polarity — `REGISTERS.md:553,569`,
  `sa1.md`); they document why the code is the way it is.
- The Mesen2 MCP bridge (40 tools, user-global config, not in-repo) is the
  **reference feature list** for `luna mcp` parity.

## Phased plan

**Phase 0 (now, independent of luna#63):** add luna to the `opensnes doctor`
emulator detection; nothing else moves.

**Phase 1 (after luna#63 P1-P3 land):** port the four snesdbg example
workflows to `luna mcp` recipes (watch-by-name, symbol-annotated trace,
OAM shadow-vs-hardware, live game debug); document them where snesdbg's
README lived.

**Phase 2 (retirement):** delete `devtools/snesdbg/` (nothing automated
depends on it); re-point the 7 workflow-gating rule files + port-example
skill from "Mesen2" to "luna GUI / `luna mcp`"; sweep the user-facing docs
(Mesen2 stays only as *an* optional third-party emulator and as historical
accuracy citations); update `debug.h` doc comments; regenerate Doxygen.

**Acceptance:** the four workflows reproducible via `luna mcp` without
Mesen2 installed; `make lint-docs` green; no rule file names Mesen2 as
required.

## Cross-references

- [luna#63](https://github.com/k0b3n4irb/luna/issues/63) — the luna-side spec.
- `.claude/notes/chantiers/luna_migration.md` — the automated-pillar
  predecessor of this chantier.
- `.claude/notes/conventions/api_pruning.md` — the principle this serves.
- `tools/luna-test/probes/lib.py` — existing Python `.sym` resolution the
  luna-side `.sym` support can borrow from (with `symbols.lua`).
