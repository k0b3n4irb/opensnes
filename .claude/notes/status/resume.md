# Where the work stands (resume note, 2026-10-30, evening)

The note a new conversation starts from. Keep it short and current: what is
in progress, the next step, what waits for whom. History is in the git log
and the CHANGELOG.

## State

- **v0.53.0 is released** (tag on `main` at `ca31aa92`, GitHub Release with
  its five files, the site rebuilt from `main`). The worktree
  `../opensnes-requests` and its branch `wip/requests` are merged and can go.
- Since the release, on `develop` only (goes in 0.54.0):
  - `cfe348d4` `games/breakout` made legible;
  - `7a36d45c` the label of an inlined block names the callee;
  - `f7f11dfe` `fadeStart` / `fadeStep` / `fadeRunning` and a short path in
    `tcc_mul32` (the second game measured it: no gain for it, its u16
    products were gone before; closed);
  - `4dca0011` `opensnes-rom check` removes wlalink's `RAM_USAGE_SLOT_*` rows
    from the `.sym` (asked by the porting kit, whose profile reading they
    misled; `--keep-ram-markers` keeps them). Suite green on the stripped
    corpus.
  - the tech note `per_object_loop_codegen.md` and A11 of
    `STRUCTURAL_DEFECTS.md` (the backend never indexes with Y).
- The neighbours were told "on develop at f7f11dfe, rebuild"; the kit's
  lot is announced in its thread (a relink is enough: the tool, not the lib).

## Promised, dated

- **2026-10-31 (Friday)**: the measurement owed to the first game — an
  autonomous case (18 sprites 32x32 at fixed VRAM slots, 130 synthetic
  frames of 512 bytes, one tick per two frames, changes of the shape of its
  distribution: mean 4, p90 10, bursts of 18; 5 frames per VBlank) measured
  on luna against the dynamic engine plus the VRAM queue
  (`vramQueueFlushUntil`). Then a yes or a no on a fixed-slot sprite module
  (0.54 if yes), in its thread. Its question still open: are its ≈ 130
  instructions per sprite the "frame changed?" decision plus the queueing,
  without the send?
- **2026-11-02 (Monday)**: the compiler lot "per-object loop" for the second
  game, in a worktree: the five wastes of
  `.claude/notes/tech/per_object_loop_codegen.md` (phi copies, dead
  temporary store, load scheduling, reload around `ldx`), from 70 to about
  55 instructions per iteration on the reduced case; measured on that case
  before and after (instructions on the common path, luna cycles over 64
  entries), difftest hunts, corpus A/B. Announced to the game at delivery.

## Waiting

- luna: its answer to `partners/luna/2026-10-30_to_luna_profile-mirror-fold.md`
  (`profile` folds a bank-$00 PC — the vectors' `jml` in `NmiEntry` — onto
  the last RAM label, because every ROM label is `80:xxxx`; asked: fold the
  mirror, never credit a ROM PC to a RAM label). The kit's thread is closed
  (markers gone from its profile; its 39 PCs were `$00:8000-$824D`, this).
- A tool that reads the compiler output: confirm it names the callee of an
  inlined block on its earlier case.
- The first game: remove its fade workaround, replay its sound test.

## In progress

Nothing between turns. OpenSNES works reactive (owner, 2026-10-29): it
answers the games, luna and the porting tools, and starts no chantier of
its own; the two dated items above answer requests.

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
