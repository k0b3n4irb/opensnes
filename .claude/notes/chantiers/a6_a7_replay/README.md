# A6+A7 atomic patch replay — next-session entry point

## Purpose

Captures the 9-site atomic patch + 4 Kl-opt guards from the 2026-05-11
session (commits e2ba69c and c7b0d06 documented the attempt and rollback).
Re-applies them mechanically so the next focused session starts from
the same compiler state without 30 min of re-typing.

## What's in here

| File | Purpose |
|---|---|
| `apply_a6_a7_edits.sh` | Replay script: guards branch state, applies patches, builds, smoke-tests |
| `cproc.patch`          | A6.1 (pointer size/align 8/8 → 4/2 in `type.c`) |
| `qbe.patch`            | A6.4 + A6.6 + A6.9 + A6.10 + A6.11 + A7.3 + A7.4 + A7.5 + 4 Kl-opt guards (`emit.c`, `w65816/emit.c`, `w65816/abi.c`) |
| `README.md`            | This file |

## Branch model

The patches are committed to **`wip/a6-a7-atomic-v3`** (branched from
develop at c7b0d06). The replay script ONLY works on that branch — it
asserts so explicitly. develop must stay at A6.8 baseline (269/269)
throughout this chantier.

## Session entry point (use this checklist)

1. **Confirm 8h+ uninterrupted block available.** This chantier needs
   Mesen2 step-through diagnostic; it cannot be done in fragments.

2. **Switch branch and replay:**
   ```sh
   git checkout wip/a6-a7-atomic-v3
   bash .claude/notes/chantiers/a6_a7_replay/apply_a6_a7_edits.sh
   ```
   Expected: ~30s to apply + build, then a smoke build of hello_world.

3. **Verify baseline regression count** (sanity check that nothing has
   drifted since 2026-05-11):
   ```sh
   make clean && make
   cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
   ```
   Expected: ~221/267 (-46 vs the 269/269 develop baseline). If the
   count is wildly different (>±5), something has drifted — pause,
   investigate, do not proceed.

4. **Phase 1 — Mesen2 diagnostic (~2-3h):**
   - Pick `text/hello_world` first (simplest, full screen diff = 57344
     px → most visible failure).
   - Open the ROM in Mesen2, set a breakpoint at `main` (use the
     `.sym` file to find the address).
   - Step through `consoleInit() → setMode() → bgSetMapPtr() → ...
     dmaCopyVram()` watching for the first wrong value.
   - Likely suspects (working hypothesis):
     - Lib ASM `dmaCopyVram` reading the wrong byte for bank
       detection now that 4 bytes are pushed instead of 2.
     - Data-section pointer width change (A6.4) shifting struct
       layouts that ASM lib code accesses.
   - Document the mechanism in
     `.claude/notes/chantiers/a6_a7_unified_audit.md` under a new
     `## 2026-05-XX Mesen2 diagnostic` section BEFORE applying any
     fix.

5. **Phase 2 — Test the A6.4 hypothesis (~1h):**
   ```sh
   # Revert just A6.4 (keep dtype_size[DL] = 8)
   sed -i 's/\[DL\] = 4/[DL] = 8/' compiler/qbe/emit.c
   make -C compiler/qbe CC=clang && cp compiler/qbe/qbe bin/qbe
   make clean && make
   cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
   ```
   - If failures drop significantly → A6.4 is the systemic problem,
     decouple it as a separate chantier.
   - If failures unchanged → A6.4 is innocent; the bug is elsewhere
     (more likely in lib ASM or in another Kl IR op).

6. **Phase 3 — Targeted fixes** based on Phase 1 findings. Each fix:
   - Documented in the audit doc with file:line and rationale BEFORE
     the edit.
   - Tested against the SAME failing example that revealed it.
   - Only after isolated validation, re-run the full suite.

7. **Phase 4 — Validation:**
   - `make clean && make` zero warnings.
   - `node test/run-all-tests.mjs --quick` → 269/269.
   - Mesen2 manual validation on 3-5 representative examples
     (hello_world + 1 sprite + 1 game + 1 audio).

8. **Phase 5 — Commit:**
   - Squash to ONE atomic commit on `wip/a6-a7-atomic-v3` if work
     diverged into multiple WIP commits.
   - Cherry-pick / merge to develop.
   - Bump `compiler/PINS.md` with new qbe and cproc SHAs.
   - Commit the parent change on develop with conventional commit
     message `feat(compiler): chantier A6+A7 — full pointer ABI` or
     similar.

## Abort conditions (revisit, don't push through)

- **Phase 1 ≥ 3h with no clear mechanism** → escalate to user, do not
  blindly try patches.
- **Phase 3 reveals > 3 new sub-sites** → this is cycle 4 of the
  audit-implement-discover-prerequisite pattern; pause and re-plan
  the chantier scope rather than chasing the tail.
- **Lib ASM rework becomes necessary** → split into chantier A6.12+,
  do NOT bundle into this patch. The 9-site compiler patch should
  ship as a coherent unit even if lib ASM lag.

## Patch regeneration (if the underlying baseline drifts)

If develop advances and the patches no longer apply cleanly:

```sh
# From a state where the edits are applied:
(cd compiler/qbe && git diff) > .claude/notes/chantiers/a6_a7_replay/qbe.patch
(cd compiler/cproc && git diff) > .claude/notes/chantiers/a6_a7_replay/cproc.patch
```

The patches encode raw `git diff` output so they tolerate minor
context drift via `git apply`'s 3-way merge.

## Cross-references

- `.claude/notes/chantiers/a6_a7_unified_audit.md` — full chantier
  history (5 sessions, 3 audit-implement cycles, ~700+ lines of
  diagnostic notes).
- `.claude/notes/chantiers/a7_phase0_handoff.md` — Phase 0 repro source.
- `.claude/notes/chantiers/a7_phase1_design.md` — original site mapping.
- Commit `4aa4989` — A6.8 large-frame addressing (shipped, baseline).
- Commit `e2ba69c` — audit doc with 7-site plan (pre-implementation).
- Commit `c7b0d06` — 2026-05-11 attempt findings (rolled back).
