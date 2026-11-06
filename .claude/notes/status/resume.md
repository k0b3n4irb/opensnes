# Where the work stands (resume note, 2026-11-04)

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

## Done 2026-11-04 (lot « sons de jeu »)

- Step 1 closed: no new sound module (the game's rule is `audioPlaySampleOn` +
  `audioStopVoice` + `audioSetVoiceVolumePitch`).
- `examples/audio/sfx_voices` (`2d67f4c0`) + manifests `audio_sfx_voices*.toml`:
  pitch update 3 scanlines, stop+play in one frame 18 (works: DSP follows),
  192-byte load step 115, seven voices 29 (~39 500 mclk). Reply to the game's
  session: fil `2026-11-04-sons-de-jeu-…`.
- `rom_coverage.py` reports two never-executed functions (`oamSetBatch`,
  `vramQueuePushSpriteWithin`), not from this lot; not in `make tests`.

## Done 2026-11-04 (snes-rag's two doc findings)

- `212ba5dc`: OAMADD is a word address (sprite N = `N * 2`) in `OAM.md` /
  `REGISTERS.md`; `hdma.h` count byte `$01-$80` no repeat, `$81-$FF` repeat.
  Answered in the thread; `make tests` green, `make lint-docs` OK.
- `04bec2d1`: doc-drift anchor 21 (`devtools/check_doc_drift.py`) refuses `OAMADD… = x * 4` and the old HDMA count wording. Before: `make lint-docs` passed with `N * 4` put back; after: exit 1 on both.
- Rest-time proposal (thread `au-repos-…`): go for step 1 only, reading with
  xenon2 what its sound engine needs; code only if measured and the
  two-games rule holds. Not started.

## Done 2026-11-05 (budget past 64 banks)

- `668dc2a4`: `opensnes-rom budget` takes a section's bank from its ROM offset
  (the `$C0` fold made linker bank 64 look like bank 0 on a FastROM LoROM of 80+
  banks). Golden on a 128-bank image: bank 0 32 784 -> 16 bytes. CI green.
  Asked by xenon2; thread answered and closed.

## Done 2026-11-06 (region gate)

- `ea446cf8`: fixtures `libtests_region` / `libtests_region_pal` + three luna manifests
  (`libtest_region_*.toml`): header byte `$FFD9` per `ROM_REGION`, `isPAL()` per console.
  CI green. `isPAL()` itself was already covered both regions by `test_libtest.py` (`make test-pal`).
- Waiting on the owner: a possible `pace` module (`PaceClock`, `paceFrames`, 6/5 units per
  VBlank) for xenon2 / speedball2. Nothing written; speedball2's card 0009 (30 ticks/s NTSC) stands.
- luna `1cce8de`: `luna state --dsp-events notes.jsonl` (one line per KON) and a `frame` column
  in `--dsp-trace`; usable for SNESMOD note-by-note checks at the next pin bump.

## In progress

Nothing between turns. OpenSNES works reactive (owner, 2026-10-29): it
answers the games, luna and the porting tools, and starts no chantier of
its own.
