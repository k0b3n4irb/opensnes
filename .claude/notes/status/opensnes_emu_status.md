---
name: opensnes-emu status and remaining work
description: Debug emulator project status, remaining test failures, and next steps for SDK maturity analysis
type: project
---

## opensnes-emu — Project Status (2026-03-15)

**Location**: `tools/opensnes-emu/` (separate git repo, excluded from opensnes via .gitignore)
**MCP**: Connected via `.mcp.json` at project root. 14 tools available.

### What Works
- WASM core (snes9x) compiled, loads and runs ROMs
- MCP server: emu_load_rom, emu_run_frames, emu_screenshot, emu_read_vram/cgram/oam/memory, emu_cpu_state, emu_ppu_state, emu_set_input, emu_reset, emu_save_state/load_state, emu_status
- Consolidated test runner: `node tools/opensnes-emu/test/run-all-tests.mjs --quick`
- 120 checks total: 7 preconditions + 63 static + 50 runtime

### Bugs Found & Fixed This Session
1. **HDMA channel 7 conflict** (commit 01b2ac5) — NMI OAM DMA on ch7 destroyed HDMA config
2. **fixMul overflow** (commit 5a3279b) — rewrote in ASM with SNES hardware multiplier
3. **textInitEx test** (commit df71542) — word address vs byte address expectation
4. **objUpdateXY docs** (commit 20802e9) — header said objhandle, impl expects raw index
5. **alloc TSA** (commit c5c838e) — alloc computes absolute stack address

### 4 Remaining Test Failures (need investigation)
1. **animation** — crash (PC in WRAM, DB=$53). Likely struct initializer or CopyInitData issue with `Animation` struct containing pointer fields.
2. **collision** (4/22 failed) — NOT alloc alias (TSA fix didn't help). Need deeper investigation of collidePoint/rectInit failures.
3. **object** (3/24 failed) — workspace roundtrip: SYNC_FROM saves workspace→buffer but objGetPointer only does SYNC_TO (buffer→workspace), losing pending writes. May be API design limitation, not a bug.
4. **sprite** (1/74 failed) — single remaining failure. Need to identify which specific assertion.

### Next Steps — Ordered Plan

**Step 1: PVSnesLib vs OpenSNES ROM analysis**
- Build PVSnesLib examples, load in opensnes-emu
- Load equivalent OpenSNES examples, compare screenshots/PPU state/WRAM
- Catalog differences: visual, behavioral, performance
- This gives feedback on SDK maturity gaps

**Step 2: Fix bugs discovered by analysis**
- Fix the 4 remaining test failures (animation, collision, object, sprite)
- Fix any new bugs found in PVSnesLib comparison
- Classify each fix: test code (A), library (B), compiler (C)

**Step 3: Migrate tests/ → opensnes-emu (INCREMENTAL)**
- For each test module in tests/:
  1. Port it into opensnes-emu runner (run-all-tests.mjs)
  2. Verify parity: same PASS/FAIL results
  3. Delete the original from tests/
- Order: unit tests first, then compiler tests, then example validation
- **Goal: tests/ directory disappears entirely**
- opensnes-emu becomes the ONLY test infrastructure

**Step 4: Visual regression baselines**
- Capture reference screenshots for all 38 examples
- Store in opensnes-emu/test/baselines/
- Automated comparison on every run

**Why:** opensnes-emu proved its value by finding 18 runtime bugs that compile-only tests missed. Some examples run "incroyablement plus rapide" after the alloc TSA fix. The user wants ONE tool, ONE source of truth.

**How to apply:** When investigating test failures, always check: (A) test code bug, (B) library bug, (C) compiler bug. Use opensnes-emu to read WRAM values and compare with expected.
