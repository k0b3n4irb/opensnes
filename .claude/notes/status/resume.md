# Where the work stands (resume note, 2026-11-03)

The note a new conversation starts from. Keep it short and current: what is
in progress, the next step, what waits for whom. History is in the git log
and the CHANGELOG.

## State

- **v0.54.0 is released** (tag on `main`, changelog dated 2026-11-01). `develop`
  is clean and equal to `origin/develop` at `01d22764`.
- The 0.53.0 worktree `../opensnes-requests` / `wip/requests` is merged and
  can go.

## Open, nothing fixed

- The dynamic engine's NMI hook has no budget and loses OAM at 7 frames in a
  VBlank, and a 17th sprite reads past its tables (`silent_defects_log.md`).
  Needs its own lot and a Class B validation; nobody has asked for it.
- A11 of `STRUCTURAL_DEFECTS.md`: the backend never indexes with Y.

## Waiting

- luna's profile fold (`a38ff57`): replay at the pin bump that carries it
  (`partners/luna/2026-10-30_to_luna_profile-mirror-fold_reply.md`).
- The first game: remove its fade workaround, replay its sound test.
- A thread waits on the owner's decision (`fils liste --auteur`); none is
  in this session's hands.

## In progress

Nothing between turns. OpenSNES works reactive (owner, 2026-10-29): it
answers the games, luna and the porting tools, and starts no chantier of
its own.
