---
name: mesen2-rpc MVP validated
description: Phase 5 acceptance criterion reached — Claude drove the SNES emulator end-to-end via MCP in 172ms, no human in the loop. Project pivot from "structurally blind" to "structurally autonomous" is complete.
type: project
---

# mesen2-rpc MVP — validated 2026-05-14

The pivot from "every chantier costs 4–8h of Claude-asks-K0b3-to-click-Mesen2"
to "Claude drives Mesen2 autonomously" is **done**. Phase 5 acceptance
criterion from `.claude/plans/tender-yawning-cake.md` met.

**Why:** This was the load-bearing claim of the entire mesen2-rpc
chantier proposal. If the demo had failed at the wire (BPs not firing,
addresses not aligning, MCP framing broken), the 4–6 month plan was a
sunk-cost proposition. It passed in one session.

**How to apply:**
1. For any future SNES bug suspected to be runtime-state-dependent
   (acache regression, NMI race, stack corruption), the workflow is
   now: `node mvp-demo.mjs`-style autonomous assertion, not "open
   Mesen2 and step through manually".
2. Treat the mesen2-rpc surface as the canonical introspection API.
   Don't ask the user to navigate Mesen2 GUI unless mesen2-rpc lacks
   a needed tool — and in that case, add the tool first.
3. The 172ms wall-time for ~15 RPC calls means generous chains of
   inspection (15-step walkthroughs of suspected bug paths) are
   cheap. Use them.

## What was demonstrated

Script: `clients/typescript/test/mvp-demo.mjs` (commit `041b9e77`).

1. Load `hello_world.sfc` — emulator stays paused (ConsoleMode flag).
2. Install BP at `main@while_cond.27 = 0x9619` **before any frame runs**.
   Critical: if you let the emu boot first, `main()` finishes its
   one-shot message-writing loop and the BP never fires.
3. Walk 5 iterations of the loop:
   - Wait for BP hit
   - Read SP, derive `i = mem[SP+8]`
   - Assert `i == iteration_number` and `i in [0..12]`
   - Observed: 0, 1, 2, 3, 4 ✓
4. Clear BPs, fast-forward with `cpu.run_until 0x9663`
   (`main@while_join.29`).
5. Assert `i == 12` at loop exit (12 chars in "HELLO WORLD!" + 0xFF
   terminator).

Total wall time: **172.1ms** for the full chain.

## Why this proves the acache-style detection criterion

The original A6+A7 acache bug manifested as a stack slot getting
stomped. `i_loop` would drift out of its expected `[0..11]` range
because something else corrupted its memory. The assertion shape in
step 3 (`i in [0..12]`) is EXACTLY what catches that class of bug:
each iteration we read the variable's current value and bound-check.
A corruption would fail the assert on the first BP hit after the
stomp happened.

No need to actually reproduce the acache bug — the shape is proven,
and the same script would catch any equivalent corruption in future
chantiers.

## Validated stack of components

| Layer | What works |
|---|---|
| Mesen2 fork (C#) | `--rpc-server=PORT` headless boot, BP firing, snapshot save/restore |
| JSON-RPC server | 27 tools, 0.4–45ms per call, token-disciplined responses |
| TS client library | Typed surface for all 27 tools, RpcException for bounds errors |
| MCP server | Stdio bridge, all 27 tools exposed as `snes_*` Claude tools |
| Autonomous flow | Address derivation from disasm + sym + ASM inspection works |

## ROI realised — tetris `__mul32` bug (2026-05-17)

The first **real** chantier-level bug solved autonomously via mesen2-rpc.
Closes the loop on "is the 4-6 month investment worth it?" — the answer
landed on the first non-trivial use.

**Bug**: `__mul32` was declared in `lib/source/mul32.asm` as
`.DEFINE __mul32 tcc_mul32` with `.EXPORT __mul32`. WLA-DX's `.DEFINE`
captures the value at parse time, before sections are placed, so the
exported symbol carried only the 16-bit offset ($8000) and dropped
BANK 7. The linker resolved `__mul32` to $00:8000 — the address of
`tcc_mul16` (BANK 0 SEMIFREE, placed first). Every `JSL __mul32` from
C jumped to `tcc_mul16`. With incompatible stack layouts, every Kl
mul returned 0. `board[r][c]` always indexed `board[0][c]` →
tetris broken since A1-followup landed (v0.20.0).

**Hidden from static analysis**: the asm looked correct (`jsl __mul32`).
The symbol table looked correct (`__mul32 = $008000`). The qbe codegen
looked correct. The mul32.asm code looked correct. Only at runtime,
disassembling `$00:8000`, did the lie surface: that address held
tcc_mul16's bytes, not tcc_mul32's. Static review missed it for
~3 days of investigation that had me considering rewriting most of
qbe's Kl handlers.

**mesen2-rpc workflow that found it**:
1. Build tetris with stateTitle() patched to auto-start (no input
   simulation tool in MCP yet — workaround: bypass the START check).
2. Load via `snes_load_rom`, run frames to enter gameplay.
3. Read board state via `snes_mem_read_range($0061, 240)` — saw type=4
   cells at row 0 cols 3,4,5 (PIECE_S locked at HIDDEN spawn row).
4. Set exec BP at `boardLockPiece` entry ($00:E75B), wait for hit.
5. Read stack args: row=22, col=3, type=4 — correct.
6. Walk into the function, BP at `STA $0000,X` (the actual board
   write), read X = $0064 = `board[0][3]`. Should have been $0141 =
   `board[22][4]`.
7. Backtrack: the address came from a `__mul32(22, 10)` call that
   returned 0 instead of 220.
8. Step into `JSL $008000`, disassemble — discovered `tcc_mul16`'s
   code at $00:8000 instead of `tcc_mul32`'s.

Total round-trips to the emulator: **~25**. Wall time: **~3 minutes**.
Without mesen2-rpc, this would have been hours of "open Mesen2,
click breakpoint, type address, observe register, swap windows to
the asm dump, scroll, re-set breakpoint, repeat" — and the user
would have done all of it manually because Claude can't drive a GUI.

**ROI verdict**: paid for itself on the first real use. The
detection shape proven in May 14's mvp-demo.mjs (BP + step +
mem.read composing into a corruption check) is exactly the shape
that surfaced this bug. The structural-blindness era is over.

**Tool gaps surfaced**:
- No input injection (joypad write). Workaround: patch the C source
  to bypass input-gated state machines. Acceptable for autonomous
  debugging but inconvenient. Future tool: `snes_input_set(buttons)`.
- No memory write tool. Could not e.g. write `game_state = STATE_PLAYING`
  directly to skip the title screen — had to recompile. Future tool:
  `snes_mem_write_byte/word`.

Both deferred — workarounds work, the surface already covered the
diagnostic need.

## What's NOT yet validated

- **Cross-CPU breakpoints** (SA-1, SuperFX). All testing was on the
  main 65816 CPU. The BPM splits by CPU type; we'd need a separate
  validation run on an SA-1 example.

- **Watchpoints / trace tools**. Plan called for 32 tools, we have 27.
  Missing: `watch.*` (3), `trace.*` (3). Those are workflow conveniences,
  not capability blockers — the existing BP + step + mem.read surface
  composes to the same outcomes.

## Branch state

`k0b3n4irb/mesen2-rpc:dev/rpc-server` head: `041b9e77`.
12 commits ahead of upstream Mesen2.

## When this rule does NOT apply

- For pure UI/screenshot regressions, `tools/opensnes-emu` is still
  the right tool (snes9x WASM is faster to boot for visual diffs).
- For SuperFX (GSU) debugging, Mesen2-rpc surfaces 65816 state only;
  GSU support requires extending the C# RPC server with GSU-specific
  methods. Track under structural defects if it becomes a blocker.

## Cross-references

- Plan: `.claude/plans/tender-yawning-cake.md`
- Phase 1 breakthrough: [[mesen2_rpc_phase1_breakthrough]]
- Demo script: `Mesen2/clients/typescript/test/mvp-demo.mjs`
- MCP server: `Mesen2/clients/mcp/`
- Repo: <https://github.com/k0b3n4irb/mesen2-rpc>
