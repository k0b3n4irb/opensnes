# Where the work stands (resume note, 2026-10-29)

The note a new conversation starts from. Keep it short and current: what is
in progress, the next step, what waits for whom. History is in the git log
and the CHANGELOG.

## State

- **v0.53.0 is released** (tag on `main` at `ca31aa92`, GitHub Release with
  its five files, the site rebuilt from `main`). `develop` = `origin/develop`
  = `cfe348d4`, CI green. No worktree, no stash, nothing local, nothing
  running.
- Landed since 0.52.0, all published in 0.53.0:
  - FastROM: LoROM code addressed in the $80 mirror, so `USE_FASTROM=1`
    speeds up C and the library;
  - VRAM queue: `vramQueueFlushUntil(line)`, flush loop at 619 master
    cycles an entry, `VRAM_QUEUE_ENTRY_COST` 80, 12 bytes of RAM an entry;
  - tools built with clang; a failed link names the missing `LIB_MODULES`
    module; `__mul16` skips empty partial products; `opensnes-sample
    inspect` sizes a looped WAV as `encode` writes it; `audio.h`'s echo
    figures corrected (58,368 and 48,128);
  - `games/breakout`: steel cabinet, paddle, bricks and ball redrawn.
- Since the release, on `develop` only (goes in 0.54.0): `cfe348d4`,
  `games/breakout` made legible (saturated bricks, quiet board, black score
  panel, softer shadows).
- The two games built on this tree tried each delivery and report green.

## In progress

Nothing.

## Waiting

Nothing for the owner. OpenSNES works reactive (owner, 2026-10-29): it
answers the games, luna and the porting tools, and starts no chantier of
its own.

## Possible next work (none chosen)

- The second load elimination after gvn, withdrawn for two silent
  miscompiles with FAR arrays: `.claude/notes/tech/load_elimination_after_gvn.md`
  says how to pick it up (reduce difftest_stmt seeds 16/6 and 87/9 first).
- The const multiply's dead high word (the `mul i, 51` of a 2D row base
  computes a 32-bit product whose high word nothing reads; outside loops).
- The other examples still on art of a third party, if any is found by the
  asset review; breakout's way (mock-ups of the real screen, chosen by the
  owner, drawn in the generator) worked.

## Standing rules worth remembering

- Push `develop` after the checks (suite green, one author, no tool line,
  no name of a private project: run the grep on the pushed range); `main`,
  tags and releases wait for the owner, typed in the session or relayed by
  the coordinating session with his words quoted and recorded
  (`.claude/rules/exchanges.md`, "The owner's word").
- A compiler, public header, `make/common.mk`, template or crt0 change goes
  in a separate worktree; a full rebuild of this tree is announced to the
  sessions that build on it, before and after.
- Heavy commands (full rebuilds, suites, corpora) go through the machine's
  queue, one at a time.
- The pre-commit hook does not see a suite run through the queue: check the
  log for `ALL CHECKS PASSED` and set the day's marker by hand.
