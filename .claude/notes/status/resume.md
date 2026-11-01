# Where the work stands (resume note, 2026-10-31)

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

## Done 2026-10-31

- The compiler lot "per-object loop", delivered ahead of its date: 64.2 to
  55.2 instructions and -14.7 % cycles an entry on the second game's reduced
  case (`.claude/notes/tech/per_object_loop_codegen.md`), announced in its
  thread; it closes it. What is left of that loop is A11 (Y never indexes).
- The measurement owed to the first game: no fixed-slot module, a saving of
  9,489 cycles a frame does not pay for one (`devtools/slotbench`, `make
  bench-slots`, `.claude/notes/tech/fixed_slot_sprites.md`). Answered in its
  thread; it closes it.
- Found by the same bench, **open, nothing fixed**: the dynamic engine's NMI
  hook has no budget and loses OAM at 7 frames in a VBlank, and a 17th sprite
  reads past its tables (`silent_defects_log.md`). The fix changes
  `MAXSPRTRF` or the hook's order, so it needs its own lot and a Class B
  validation; no one has asked for it yet, the first game does not use the
  dynamic engine.

## Waiting

- luna shipped the profile fold the same day (`a38ff57` on its `develop`:
  a ROM PC takes its mirror's label, never a RAM label; replayed here with
  its stable binary: `NmiEntry` 200, `oamQueueEntry` gone —
  `partners/luna/2026-10-30_to_luna_profile-mirror-fold_reply.md`). To
  replay at the pin bump that carries it. The kit's thread is closed.
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
