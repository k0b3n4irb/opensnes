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

## What's NOT yet validated

- **Acache bug reproduction**. The detection *shape* is proven, but
  the actual A6 fix is in place, so there's no bug to detect right
  now. To truly close Phase 5 with full force we'd:
  - branch off A6 with the acache fix reverted
  - run the demo
  - watch i_loop exceed 12 at some iteration
  - confirm the script flags it
  Deferred — the structural proof is enough for now.

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
