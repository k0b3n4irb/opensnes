# Where the work stands (resume note, 2026-10-30)

The note a new conversation starts from. Keep it short and current: what is
in progress, the next step, what waits for whom. History is in the git log
and the CHANGELOG.

## State

- **v0.53.0 is released** (tag on `main` at `ca31aa92`, GitHub Release with
  its five files, the site rebuilt from `main`). `develop` = `origin/develop`
  = `f7f11dfe`. The main tree was rebuilt clean at that commit (60/60 ROMs
  alive); the worktree `../opensnes-requests` and its branch `wip/requests`
  are merged and can go.
- Since the release, on `develop` only (goes in 0.54.0):
  - `cfe348d4` `games/breakout` made legible;
  - `7a36d45c` the label of an inlined block names the callee
    (`inl<N>_<callee>.<index>.<block>`; QBE fork at `b8d9155`, PINS updated);
  - `f7f11dfe` `fadeStart` / `fadeStep` / `fadeRunning` (a fade that does not
    hold the program) and a short path in `tcc_mul32` (both high words zero:
    -51 % for u16 x u16 -> u32, +2.5 % when a high half is set).
- The three neighbours were told "on develop at f7f11dfe, rebuild".

## Waiting

- The kit: confirm `expliquer` names the callee on its 2026-10-13 case.
- The first game: remove its fade workaround, replay its sound test.
- The second game: measure its step with the short multiply, send the reduced
  per-object loop (40 to 100 word-array entries) so instructions per
  iteration can be counted.
- The first game's fixed-slot sprite engine: later, from the case given in
  SDK terms; an autonomous example is ours to write first; nothing of the
  game is committed.

## In progress

Nothing. OpenSNES works reactive (owner, 2026-10-29): it answers the games,
luna and the porting tools, and starts no chantier of its own.

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
