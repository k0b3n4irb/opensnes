---
name: mesen2-rpc Phase 1 — BP firing breakthrough
description: Resolution of the cpu.run_until breakpoint-doesn't-fire bug. Two-issue diagnosis (shortcut auto-repeat + CodeBreak undiscriminated). Status: critical path validated.
type: project
---

# mesen2-rpc Phase 1 — BP firing resolved

**Date**: 2026-05-14
**Branch**: `k0b3n4irb/mesen2-rpc` → `dev/rpc-server`
**Commit**: `37b87e16`

**Why:** The MVP validation criterion was "BP fires from JSON-RPC client".
Without that, cpu.run_until is useless and the entire autonomous-debug
proposition fails.

**How to apply:** When investigating BP-not-firing in any
mesen2-derivative, check these two paths FIRST before chasing C++
breakpoint manager logic — both are non-obvious from reading the code:

## Pitfall 1 — ShortcutKeyHandler auto-repeat

`EmuApi.ExecuteShortcut(EmulatorShortcut.RunSingleFrame)` triggers
`ShortcutKeyHandler::ProcessRunSingleFrame()` which sets `_needRepeat = true`
and a timer. After 500ms it auto-fires `ProcessRunSingleFrame()` again at
50ms intervals — emulating press-and-hold for GUI users. There is NO
public `ReleaseShortcut(RunSingleFrame)` API call; the release is keyed
to `ConsoleNotificationType::ReleaseShortcut` which is only emitted by
key polling.

In headless RPC mode, calling `ExecuteShortcut(RunSingleFrame)` once
LATCHES the shortcut on indefinitely. Each repeat fire calls
`PauseOnNextFrame -> Step(SpecificScanline, 240, PpuStep)`, re-installing
a PpuStep break every 50ms. Any subsequent "free run" (Resume) is
immediately interrupted by the next auto-repeat.

**Fix**: Don't use the shortcut. Use `DebugApi.Step(CpuType.Snes, 1,
StepType.PpuFrame)` directly. Same effect (advance one frame, pause),
no shortcut latch.

## Pitfall 2 — CodeBreak is not "breakpoint hit"

`ConsoleNotificationType::CodeBreak` is fired by `Debugger::SleepUntilResume`
for ALL break sources, not just BP hits. The full source list (see
`Core/Debugger/DebugTypes.h:298` BreakSource enum):

- `Breakpoint`    — actual BP hit (what we want)
- `Pause`         — `EmuApi.Pause()` calls `Step(1, Step, Pause)`
- `CpuStep`       — single-instruction step
- `PpuStep`       — scanline/frame step (RunSingleFrame, Step(PpuFrame))
- `Irq` / `Nmi`   — interrupt entry
- `BreakOnBrk` etc. — exception breaks

The C++ side packs source info into a `BreakEvent` struct passed via
the notification's `void* parameter`. The C# side has a marshallable
struct at `UI/Interop/DebugApi.cs:1593`:

```csharp
public struct BreakEvent {
    public BreakSource Source;
    public CpuType SourceCpu;
    public MemoryOperationInfo Operation;
    public Int32 BreakpointId;
}
```

**Fix in our listener** (`RpcServer.cs`):

```csharp
if(e.NotificationType == ConsoleNotificationType.CodeBreak) {
    BreakEvent evt = Marshal.PtrToStructure<BreakEvent>(e.Parameter);
    if(evt.Source == BreakSource.Breakpoint) {
        _codeBreakSignal.Set();
    }
}
```

## Two-level pause model

A third subtle gotcha worth recording: there are TWO independent pause
flags:

| Flag | Set by | Cleared by |
|------|--------|-----------|
| Public `_paused` | `EmuApi.Pause()` when no debugger attached | `EmuApi.Resume()` when no debugger |
| Debugger `_waitForBreakResume` | `Debugger::SleepUntilResume` (any break) | `Debugger::Run()` |

**When a debugger IS attached** (our ConsoleMode case):

- `EmuApi.Pause()` calls `Step(1, Step, Pause)` — sets `_waitForBreakResume = true` after one instruction
- `EmuApi.Resume()` AND `DebugApi.ResumeExecution()` both call `Debugger::Run()` (clears `_waitForBreakResume`)
- `EmuApi.IsPaused()` routes through `Debugger::IsPaused()` which returns `_waitForBreakResume`

For cpu.run_until's Wait phase to work, BOTH paths must be cleared:
```csharp
DebugApi.ResumeExecution();   // clears _waitForBreakResume
EmuApi.Resume();              // clears the public _paused (redundant when debugger attached, but defensive)
```

## Validated latency

- `cpu.run_until` at current PC: **14ms** (BP installed + emu wakes + BP fires + state read)
- Real-world target: <50ms end-to-end through MCP → still well within budget

## Critical-path validation criterion

> *Reproduce the detection of the acache bug d'A6+A7 automatiquement,
> sans assistance humaine, en moins d'une seconde via API/MCP.*

With BP firing confirmed working at 14ms, the MVP validation criterion
is unblocked. Next steps: build the Node/TypeScript client library and
the MCP server wrapper, then write the acache-detection test.

## Files touched

- `Mesen2/UI/Utilities/RpcServer.cs` — Step-based run_frames + BreakSource discrimination + dual-Resume in cpu.run_until
- `Mesen2/UI/Utilities/CommandLineHelper.cs` — `--rpc-server=PORT` parser (earlier commits)
- `Mesen2/UI/Program.cs` — RpcServer.Run dispatch (earlier commits)
- `Mesen2/UI/UI.csproj` — StreamJsonRpc 2.21.10 dep (earlier commits)

## Tool surface as of session end (24 / ~32 planned)

| Group | Tools |
|---|---|
| cpu (6) | pc, register, state, step, step_n, run_until |
| mem (4) | read_byte, read_word, read_dword, read_range |
| bp (4) | add, clear, clear_all, list, wait_for_hit |
| emu (4) | load_rom, reset, run_frames, is_paused |
| ppu (2) | register, state |
| snap (4) | save, restore, list, discard |

Validated end-to-end against `hello_world.sfc`:
- BP firing on first execution of address (25ms)
- BP wait when pre-installed (30ms)
- PPU state matches expected (BgMode=0, Brightness=15, MainScreen=1)
- Snapshot time-travel (save@30, restore@60→30 works)
- All input validation returns -32602 with helpful messages

## What's NOT yet in the surface (8 tools missing from plan)

- `mem.search(space, pattern_hex)` — find byte sequences
- `bp.remove(id)` — currently named `bp.clear`; plan had `bp.remove`
- `ppu.tilemap(bg)` / `ppu.tile(addr, bpp)` / `ppu.sprite(i)` / `ppu.palette_color(i)` — graphical asset accessors
- `watch.add/check_changes/remove` — passive change-detection
- `trace.start/read/stop` — execution trace surface
- `disasm.at(addr, n)` / `disasm.label_at(addr)` — disassembly

## Next session

Top priorities:
1. **MVP demo validation**: write the acache-bug detection script per
   the plan's Phase 5 criterion. Tests bp.add at the loop label, runs
   until break, mem.read_word at $001FE6, asserts i < 12.
2. **mem.search** + **disasm.at** — high-value, used by acache demo.
3. **Node client library scaffolding** — Phase 4 starts here.
4. Address the WAI/step interaction: cpu.step on a WAI instruction
   doesn't advance because IsPaused never goes false during the 100ms
   wait window. Either bump the timeout or detect WAI and signal an
   "advance one frame first" hint to callers.
