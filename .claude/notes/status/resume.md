# Where the work stands (resume note, 2026-10-28)

The note a new conversation starts from. Keep it short and current: what is
in progress, the next step, what waits for whom. History is in the git log
and the CHANGELOG.

## State

- `develop` = `origin/develop` = `2f14bdd6`, CI green (Lint, Build & Release,
  Fuzz). No worktree, nothing local.
- Landed on 2026-10-26/28, all pushed:
  - `a061a2db` FastROM: LoROM code addressed in the $80 mirror, so
    `USE_FASTROM=1` speeds up C and the library;
  - `5f4c3ee6` VRAM queue: `vramQueueFlushUntil(line)`, flush loop at 619
    master cycles an entry, `VRAM_QUEUE_ENTRY_COST` 80;
  - `04b87be6` tools built with clang (`tools/tool.mk`);
  - `ce43baa0` a failed link names the missing `LIB_MODULES` module
    (`lib/build/<map>/symbols.txt`);
  - `2f14bdd6` `__mul16` skips empty partial products (-25 %).
- The two games built on this tree tried each delivery and report green.

## In progress

Nothing.

## Waiting

- **Owner**: whether to set up a sub-agent table for this project (agents
  in `.claude/agents/`, `opusplan` / `effortLevel` in `.claude/settings.json`).
  A table was proposed in the owner's session; it is configuration of the
  harness, so it waits for the owner's word there.

## Possible next work (none chosen)

- The second load elimination after gvn, withdrawn for two silent
  miscompiles with FAR arrays: `.claude/notes/tech/load_elimination_after_gvn.md`
  says how to pick it up (reduce difftest_stmt seeds 16/6 and 87/9 first).
- The const multiply's dead high word (the `mul i, 51` of a 2D row base
  computes a 32-bit product whose high word nothing reads; outside loops).

## Standing rules worth remembering

- Push `develop` after the checks (suite green, one author, no tool line,
  no name of a private project); `main`, tags and releases wait for the
  owner.
- A compiler, public header, `make/common.mk`, template or crt0 change goes
  in a separate worktree; a full rebuild of this tree is announced to the
  sessions that build on it, before and after.
- Heavy commands (full rebuilds, suites, corpora) go through the machine's
  queue, one at a time.
