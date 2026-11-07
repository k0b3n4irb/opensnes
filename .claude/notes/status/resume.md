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

## Done 2026-11-04 (rag-partner's two doc findings)

- `212ba5dc`: OAMADD is a word address (sprite N = `N * 2`) in `OAM.md` /
  `REGISTERS.md`; `hdma.h` count byte `$01-$80` no repeat, `$81-$FF` repeat.
  Answered in the thread; `make tests` green, `make lint-docs` OK.
- `04bec2d1`: doc-drift anchor 21 (`devtools/check_doc_drift.py`) refuses `OAMADD… = x * 4` and the old HDMA count wording. Before: `make lint-docs` passed with `N * 4` put back; after: exit 1 on both.
- Rest-time proposal (thread `au-repos-…`): go for step 1 only, reading with
  a partner port what its sound engine needs; code only if measured and the
  two-games rule holds. Not started.

## Done 2026-11-05 (budget past 64 banks)

- `668dc2a4`: `opensnes-rom budget` takes a section's bank from its ROM offset
  (the `$C0` fold made linker bank 64 look like bank 0 on a FastROM LoROM of 80+
  banks). Golden on a 128-bank image: bank 0 32 784 -> 16 bytes. CI green.
  Asked by a partner port; thread answered and closed.

## Done 2026-11-06 (region gate)

- `ea446cf8`: fixtures `libtests_region` / `libtests_region_pal` + three luna manifests
  (`libtest_region_*.toml`): header byte `$FFD9` per `ROM_REGION`, `isPAL()` per console.
  CI green. `isPAL()` itself was already covered both regions by `test_libtest.py` (`make test-pal`).
- Waiting on the owner: a possible `pace` module (`PaceClock`, `paceFrames`, 6/5 units per
  VBlank) for two partner ports. Nothing written; one port's card 0009 (30 ticks/s NTSC) stands.
- luna `1cce8de`: `luna state --dsp-events notes.jsonl` (one line per KON) and a `frame` column
  in `--dsp-trace`; usable for SNESMOD note-by-note checks at the next pin bump.

## Done 2026-11-06 (lzssDecodeRam)

- `f67b6abb`: `lzssDecodeRam(source, FAR dest)`, LZ10 into RAM of any bank, screen on, no `sei`.
  163 master cycles/byte on a 2 bpp tile block (139 FastROM), 4.3x faster than `lzssDecodeVram`;
  376 bytes of ROM only in a ROM that calls it. Example `examples/memory/lzss_ram`. CI green.
  Asked by a partner port; resumable slices (`lzssDecodeRamWithin`) wait for their real-block figure.
  Lesson: after a fast-forward of a lib change into this tree, `make lib` before the
  clean-export pre-push check, or the new symbol is unknown.

## Done 2026-11-06 (4 MB LoROM, sample swap)

- `d5005098`: `ROM_BANKS` up to 128 on plain LoROM. Code is linked from `$80` since 2026-10-26, so banks
  126/127 are `$FE:`/`$FF:`; the old cap of 126 dated from `$00` addressing. 128 banks: same frame hash and
  audio as 8 banks; fixture `libtests_bigrom` reads a const table in bank 127. At 126 banks the linker's
  checksum (`$6013`) differs from luna's recursive-mirror one (`$08FE`): the rules agree only when the part
  above the largest power of two is itself a power of two. Not fixed in the linker; build.md says which counts to prefer.
- `7ac47544`: `audioLoadSampleAt` / `audioLoadSampleBeginAt` (a partner port). `audioStopVoice` alone is not enough before
  overwriting: wait 8 ms (two VBlanks). `wdm_expected` key in `luna_runner.py` / `manifest.toml`.
  Open: fil to luna for a DSP-register read (ENVX) in a manifest checkpoint.
- Waiting: a partner port's real-game figures for `lzssDecodeRam` (resumable slices only if needed) and for `audioLoadSampleAt`;
  whether their ROM in 128 banks passes `luna cart-check`. `pace` module (6/5 units per VBlank): nothing written, owner's call.

## Waiting (2026-11-07)

- luna `281485c` (`[checkpoint.dsp]`) is not under a tag yet (pin `v1.40.0`): at the tag that carries it, bump the pin,
  replay the corpus golden queries, add the ENVX-after-KOFF manifest (`at_symbol` on `audioUpdate`).

## In progress

Nothing between turns. OpenSNES works reactive (owner, 2026-10-29): it
answers the games, luna and the porting tools, and starts no chantier of
its own.
