# Where the work stands (resume note, 2026-11-08, evening)

The note a new conversation starts from. Keep it short and current: what is
in progress, the next step, what waits for whom. History is in the git log
and the CHANGELOG.

## State

- **v0.55.0 is released** (2026-11-08): `main` at `f21b3fc7` (merge of `develop` `6ee38284`), tag `v0.55.0` pushed, `release.yml` run 42472392201. It is published (5 assets). GitHub had an incident that morning (timeouts on release assets and archives): the Windows leg (MSYS2 download), Doxygen and cppcheck downloads answered 504/503 several times; failed jobs were re-run. v0.54.0 was 2026-11-01.
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

- luna `281485c` (`[checkpoint.dsp]`) is not under a tag yet (pin `v1.40.0`; checked 2026-11-07: v1.42.0 is `8fee4e6`, 28 commits BEFORE it, and the 1.42.0 binary rejects `[checkpoint.dsp]`; test with `git merge-base --is-ancestor 281485c <tag>`, not `git tag --contains`): at the first tag that carries it, bump the pin,
  replay the corpus golden queries, add the ENVX-after-KOFF manifest (`at_symbol` on `audioUpdate`).

## Done 2026-11-07 (ppuReset)

- `ff9e50d4` / `279f61d5`: `ppuReset()` (module `ppu_reset`, asm), example `basics/ppu_reset`, manifest
  `basics_ppu_reset.toml` (52 PPU registers + 33 lib variables equal to the boot state; 80 diffs without the call).
  283 bytes ROM, 167 832 master cycles. CI green (run 42361931119). Thread: `2026-11-07-detecter-a-la-volee-…`.
- Open: crt0 writes COLDATA = 0 (selects no plane, fixed colour stays); `ppuReset` writes `$E0`. Fixing crt0 is Class A, waits for the conductor.
- Measured for the author: a linked module keeps its SEMISUPERFREE const data uncalled (wlalink -d never discards it):
  math/fixed32 +577 B, sprite +512, math_ease/mode7 +256, hdma +64, text+dma +38; all in asset banks, none in bank $00.
  Fixing it for all = compiler emits discardable const (Class A, ~1.7 KB): not started, waits for the author.

## Audit of 2026-11-08: OpenSNES lots (the audit's synthesis and its report A3)

- **Q2 done** (`2a33b048`, `ccbb7e5a`): the private corpus name and its four tool names are gone from the tip
  (109 files; the French word for the physical cartridge is written Game Pak). `hardware_claims.md`
  now says the evidence sentences decide alone and uses the renamed verify verdicts.
- **L9 done** (`76adcdb0`, CI green: Build & Release 42489059646, Lint 42489059649): 32-bit mul/div/rem safe in
  an NMI/lag callback (software path in `mul32.asm`, result slots saved and restored by the NMI
  handler, `div32.asm` saves its working words); fixture `testing/fixtures/stress/rt32_nmi`
  (red on the old runtime: 208 / 121 / 278 / 378 / 521 wrong of 10 000, green after); +50 master cycles per
  `tcc_mul32`, +60 per divide; the WRAM map of every ROM changed (16 bytes), baseline renewed;
  `make test-bin-clean` in `make tests`. An `irqSet` handler stays unprotected (documented, 9 369 of
  10 000 products wrong when it multiplies per scanline). Lesson in the kit (`396f24c`). Neighbours told.
  A3 C3 is documentation only: the bank-blind read check itself still does not see a pointer that travels
  between files (a check that refuses it is not written).
- **L8 and the CI downloads done** (`0621c14c`, `2d020cca`; CI green: Build & Release 42501397641, Lint 42501397653):
  Doxygen and cppcheck are cached (cppcheck built into the runner's home), the Windows legs use the runner's
  MSYS2 (`release: false`), the remaining `curl`s retry; `devtools/check_ci_downloads.py` (in `make lint`) refuses
  a `curl`/`wget` without a retry or a `# retried:` note; every `opensnes*` tool prints
  `--version` as `name 1.0.0 (OpenSNES 0.55.0, commit 0621c14c[+dirty])`, checked by
  `devtools/check_tool_versions.py` in `make test-tools`. The six ci-rouge threads are closed with the lesson
  (kit `6336448`). A first run after a tool version change has a cold cache and can still fail if GitHub is degraded.
- **WaitForVBlank lost wake-up done** (`7802f85b`): a partner port found that an NMI between `sta vblank_flag` and
  `wai` makes the `wai` sleep one frame too long. Reproduced by `testing/fixtures/stress/wait_race` (1 call lost in
  9 600 phases before, 0 after), fixed in the NMI handler (it returns past the `wai` when it interrupted
  `vblank_wai`). About +80 master cycles per NMI; every example identical frame by frame (65/65); the audio, WRAM
  and `nmi_budget` references were re-captured with their evidence in the commit and the CHANGELOG. Lesson in
  the kit (`7f59492`). Waits for the partner port's own replay of its long PAL run, then the thread closes.
  Side effect found by the CI, not by `make tests`: the new label `vblank_wai` split `WaitForVBlank` and libbench
  read 357 226 cycles of "idle" (5 852 before); fixed (`6fdd06bc`: libbench counts `vblank_wai` as idle) and the three
  CI-only benches (`cyclecount/bench.py`, `sdkbench/run.py --check`, `libbench/run.py --check`, 20 s) now run in
  `make tests`. CI green on `6fdd06bc` (Lint 42523930952, Build & Release 42523931151).
- **Still mine from the plan**: L13 is only the rule (aligned); the vitrine of tags and release notes is the
  conductor's tool; A3 C3 (a check that sees a plain pointer that travels between files) is not written.
- Open threads: `ordres de musique datés` (a partner port; answered once, a prototype proposed: dated music orders in the SPC
  driver, a moyen request); the five ci-rouge threads wait for the cache lesson.
- Waits for the author: the `ppuReset` COLDATA question (Class A); discardable const data of linked modules
  (Class A). Waits for luna: a tag carrying `281485c` and `de03dd8` (then bump the pin, replay the corpus
  queries, the DMA/HDMA timing tests and the ENVX-after-KOFF manifest).
- Machine: the btrfs `/home` has its metadata nearly full (1.67 of 2.01 GiB): writes fail with ENOSPC
  while `df` shows free space, when several heavy jobs run; reported to the conductor. An orphan
  `.git/index.lock` (0 bytes, no git process) was removed after checking.

## In progress

Nothing between turns. OpenSNES works reactive (owner, 2026-10-29): it
answers the games, luna and the porting tools, and starts no chantier of
its own.
