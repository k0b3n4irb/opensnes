# Where the work stands (resume note, 2026-11-12, night)

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
- **Done 2026-11-09 (develop `dfaa6eb6`)**: `audioStopVoices(mask)` and the KOFF hold cut from 582 to 132 SPC
  cycles (a partner port asked; `testing/fixtures/stress/koff_hold` red at 8 turns, green at 21; lesson kit
  `36c0921`); a doc-drift anchor 22 (headline = table), front page says what the CI does; audio oracle covers every
  audio example; `testing/torture` (gcc.c-torture/execute on luna, `make test-torture`, weekly `torture.yml`,
  `expected.txt`: 714 pass, 17 fail all explained, 18 no build, 952 refused; lesson kit `8b869a9`);
  SNESMOD upstream checked (nothing moved; next due 2026-11-16).
- **CI limit raised** (`cf248f0e`): the functional job (Linux) was cancelled at its 20-minute limit on 2026-11-09
  (x86_64: 10-12 min usually, 15-19 on others); now 35. CI green on `cf248f0e` (Build & Release 42601318649:
  x86_64 14 min, arm64 15 min; Lint 42601318912). A duration/limit report (alert at 70 %) is proposed to the CI watch, not written.
- **Torture subset in `make tests`** (`c5ea804b`): 26 tests, about a minute, any difference from `expected.txt`
  fails; shown to fail with `fdcda7c8` undone (10 pinned refusals change, two tests fail at run time) and with
  `ee5b8636` undone (`own/hex-escape-stops`, our own test: no gcc test exercises it). Needs the network the first
  time (fetches the gcc tests; skipped offline outside CI).
- **ROM hashes across platforms (measured 2026-11-09, run 42675444869)**: Linux x86_64 = Linux arm64 = a local aarch64
  build on all 65 examples; macOS and Windows each differ from Linux (and from each other) on all 65, by 41 to 51
  isolated operand bytes: 8 console-module RAM variables get other addresses. Cause: `_sections_sort` in the wla-dx
  fork's `wlalink/write.c` returns -1 for two sections of equal priority and size in BOTH directions (not a valid
  order); `qsort` then orders them as its implementation likes. Fix to make (Class A, fork + PINS.md): a stable sort
  (ties keep the original order); glibc already gives that order, so the 65 Linux ROMs must stay identical and the
  five hash lists of the CI must become equal. `rom_sha256` of `baselines.json` is stale for all 65 and read by nothing.
- **Torture refusals triaged** (`6434c224`): 14 `compiler-internal` (8 assertions in `emitdata`, one SIGSEGV...), mostly
  from `vector_size`/`mode` being ignored; the cproc lot below turns most into clean refusals.
- **luna v1.43.0 is published** (`--inject-nmi-at`/`--inject-irq-at`, `[checkpoint.dsp]`, S-DSP stepped per SPC cycle):
  next lot = bump the pin from v1.40.0 (audio hashes will move: prove with `luna diff --audio --align-onset`), then the
  interrupt gate (it must fail for certain on the defects of `7802f85b` and `76adcdb0`).
- **cproc attribute lot: DONE and pushed (2026-11-10)**, fork `82e1c1d`, develop `011d7645`, CI green; worktree removed. (Was: my own gates are green, the A/B proof and the 2000-seed hunts must be replayed before the fork and OpenSNES are pushed. It makes cproc REFUSE the GNU attributes `mode` and `vector_size`
  (and whichever others it can justify) — a Class A lot (fork commit, PINS.md, negative cases, torture
  `expected.txt`: 20020108-1 and scal-to-vec2 go from FAIL to REFUSED). I re-validate every gate before merging.
- **Finding to fix (Class A, cproc fork + PINS.md)**: the GNU attributes `mode` and `vector_size` are accepted and
  ignored (`sizeof` 2 where GNU C gives 1 and 8); recorded in `KNOWN_LIMITATIONS.md`; refusing them is a lot of its own.
- **Still mine from an independent review of 2026-11-09**: a request to luna to force an NMI/IRQ before an
  address (thread open, prototype `wait_race`), then a gate that sweeps the critical sections; journal for
  `wram_regress.py --update` (a reason required, the count of drifted examples, the `diff_corpus` result); a
  hold-out set of workloads never cited by a `perf` commit; measure whether the 65 ROM hashes agree across
  Linux, macOS and Windows; triage of the K&R and "other" refusals of the torture run; check that reverting
  the string-literal fixes makes a torture test fail; the audio follow-ups of the partner port (non-blocking
  "command taken?" test, a note on calls from an `nmiSetLag` callback with one sender, a note start in two
  handshakes, `audio_queue`); a one-page note on TAD against SNESMOD. The second-opinion emulator job waits
  for the scope the conductor sets; then the doc-drift rule 11 gets a `# second-opinion:` marker.
- The A3 C3 check (a plain pointer that travels between files) is still only documented.
- Open threads: `ordres de musique datés` (a partner port; answered once, a prototype proposed: dated music orders in the SPC
  driver, a moyen request); the five ci-rouge threads wait for the cache lesson.
- Waits for the author: the `ppuReset` COLDATA question (Class A); discardable const data of linked modules
  (Class A). Waits for luna: a tag carrying `281485c` and `de03dd8` (then bump the pin, replay the corpus
  queries, the DMA/HDMA timing tests and the ENVX-after-KOFF manifest).
- Machine: the btrfs `/home` has its metadata nearly full (1.67 of 2.01 GiB): writes fail with ENOSPC
  while `df` shows free space, when several heavy jobs run; reported to the conductor. An orphan
  `.git/index.lock` (0 bytes, no git process) was removed after checking.

## In progress

**Where the chantier stands (2026-11-12, night; this session runs on Opus since this evening, owner's decision to save
quota; a Fable sub-agent may be called by this session alone when needed, and the journal says when and why).**
1. **Lot L2 (jump table) is DONE and pushed** (`develop` `825a39ab`, QBE fork `a0c9619`). Both games returned their figures
   on the advance-4 thread: game 1 -0.072 % a step (30/30 tests), game 2 -1.86 % (713 steps, 16 tables of 147 sites). The
   thread closes with the CI run and the lesson.
2. **L7 examined, not opened** (`chantiers/gap_lots/L7_library_hot_routines.md`): about -1.5 % of game 1's busy time and
   -0.4 % of game 2's at best; ~4 of the plan's 18.7 points are DMA bytes, not code; `(dp),y` costs the same as `[dp],y`
   with 16-bit index registers. Kept from it: the fixture `testing/fixtures/stress/world_place` (in `make tests`). The
   `oamPlaceWorld` unrolling waits as a short lot for a quiet day.
3. **Next**: (a) the private measurement through the other 65816 C compiler is BLOCKED on its linker (vlink 0.18, sources
   not reachable at their origin on 2026-11-12; private note `dumps/opensnes-etalons/vbcc/LISEZMOI.md`); three ways asked of the
   conductor (thread `…opensnes-en-fable-5-2-reduire`, message 034). (b) **L3 is designed, contested and counted** (`chantiers/gap_lots/L3_machine_optimiser.md`): M0 (2026-11-13) gives
   values in X/Y 1.1 and 2.2 % of busy time (high bound 2.7 / 6.6), under the 3 % rule written before the count: S3 does not
   open as written. **L4 counted** (`gap_lots/L4_frames_and_calls.md`, M1 2026-11-13): 5.0 and 7.1 % of busy time at most, L4 opens
   (static frames 4.1 / 6.2, first argument in A 0.6; near calls out, 0.3). L4's design is written and contested (Fable, 2026-11-13: go with changes, all five taken: overlap criterion not cycles,
   placement at link time, arguments in the callee's static frame, window sized from the measurement, refusal's remedy).
   **M2 done (2026-11-13)**: F1 alone 2.4 / 2.7 %, F1 + change 3 2.9 / 3.3 %, deepest chain 136 / 104 bytes (window above 96), refusals 1 of 143 / 233; by the rule L4 as written does not open. The largest class is temps crossing no call that sit on the stack (3.4 / 7.2 %): **M3 done (2026-11-13)**: `tcc__lf` axis ceiling 0.66 / 1.66 % of busy (4 extra words: 0.64 / 1.14), M2's 3.4 / 7.2 % was whole cost, not saving; under the 3 % rule too (`gap_lots/L4_frames_and_calls.md`, section M3). **M5 and M4 counted (2026-11-13)**: M5 0.58 / 0.21 % of busy, long-indexed reads 2.4 / 0.2 % at most (FAR RAM, not const data): no axis of the L3/L4 family reaches the 3 % rule alone; the ranked list (not additive, conductor's estimate 5 to 8 % on game 2) is at the end of the L3 note. Next: the conductor decides what, if anything, opens from the set. Was: **Decision written (L3 note, end): next lot S4 / L5, count M5 first (rule written); M4 only if M5 fails. Was: M4**, the top-cost instruction shapes of the traces, whole program, for a class the plan has not named; report to the conductor first. Tooling of the count:
   `dumps/opensnes-etalons/l3m0/` (private; `trace.py TAG`, `m0.py TAG sb|x2`). R1 stays in the plan.
3b. **Decisions of 2026-11-13 (conductor, recorded in the conductor's decision log)**: (1) short lot `tcc__lf` +4 words + operand in place (Opus agent in a worktree, prediction written first; nothing on `develop` before this session replays every gate); (2) then L4 F1 + change 3 (plan contested by Fable first); (3) the owner said A: a bounded, throwaway prototype of a code generator designed for the 65816 opens after the short lot and during L4. Before starting it: cost in days, plan contested by Fable, engagement rule as written (within 1.1 of the hand on the two leaves AND at least 10 % of game 2's busy time), four hand-rewritten perimeters, same traces, equivalence proven; the current generator stays the yardstick, nothing of the prototype enters `develop`; the figures go to the conductor, the commitment beyond is the owner's.
3c. **Way 2 opened (owner installed the x86 user-mode emulator, 2026-11-13)**: official x64 binaries of the other 65816 C compiler under emulation, private (`dumps/opensnes-etalons/vbcc/`). Order: after the short lot, alongside the prototype. First proof of install: its own Dhrystone binary reproduced byte for byte; then the private measurement of the bench routines, nothing published. If the gap is flagrant: assembly of both compilers compared routine by routine (2026-11-12 decision).
3d. **luna delivered point 10 (2026-11-13)**: profile mirrors follow the cartridge mapper, on its `develop` `075c2f8` (not tagged; pin stays `v1.43.0` until its next tag). To replay when the short lot is closed, with that build: the two gauge ROMs (`develop-a7`, `a6a-lot1`); expected: the function keeps about 422 050 mclk under its name in both, nothing left under `$C2:A140`, `$C3:ECA6`, `$C3:FEEA`. Then tell luna.
3e. **Short lot LF+M5 is DONE and pushed (2026-11-13, `develop` `561ded10`, QBE fork `650a80e`, CI green after a macOS-runner re-run: capacity, not code)**: measured 1.42 / 0.53 % of busy time against a prediction of 1.2 / 1.35 (the `tcc__lf` axis alone 0.00 / 0.24, the in-place operand 1.41 / 0.29: M3's figure was a ceiling on the executed path, the compiler keeps on the stack every temporary live after a call on any path). Gates replayed by this session: A/B 65/65, 2000+2000 fresh seeds, `make test-compiler` 150/0, `make tests`. Lesson in the kit. The games returned their figures (the first game -0.69 %, the second game -1.35 / -1.30 % NTSC / PAL, state identical in all); thread closed. Next, in this order: L4 F1 + change 3 (plan cut up and contested by Fable first), the prototype generator (cost in days + contested plan + the rule written as given), way 2 (private), luna point 10 replay (two gauge ROMs, luna `075c2f8`), an ATTRIBUTION lot (5 PNG, 2 palettes, `.tmj` editor, `wavetable.bin` licence). Trap met: a `commit -a` after `make clean` commits the deleted tracked generated files; stage by path.
3f. **luna 2026-11-14 (`a029ef7`, develop, not tagged)**: the sprite time-over is computed per sliver (a line over 34 slivers keeps the left slivers of the overflowing sprite; a sprite at x=256 pays all its slivers). No effect while the pin stays `v1.43.0`; at the pin bump that carries it, replay the framebuffer-hash baselines of the corpus scenes loaded with sprites (`python3 testing/luna_runner.py --compare`) and any manifest asserting an OAM layer, and re-capture only after reading each diff.
3g. **Calls to the strongest model (journal, rule `.claude/rules/fable_calls.md` since 2026-11-14)**: 2026-11-13, contest of the generator prototype's plan (one task: what is missing, cost, rule, architecture, yardsticks, false certainty; files: the plan, the L3/L4 notes, `emit.c`; read-only; answer: ranked critiques and an ordered list of changes), ten changes taken in `G1_generator_prototype.md`; 2026-11-13, contest of the L4 plan cut into steps (seven named points, read-only, same form), eleven changes taken in `L4_plan_decoupe.md`. Both answers were checked against the code or a count before being taken (the 92 % refusals figure was recounted by S0).
3h. **The games' own figures disagree with the frozen gauge (2026-11-14)**: the X/Y colour lot measured -1.72 / -1.69 % on the private gauge (frozen snapshot of each game's sources, game 1 frozen on 2026-11-11); the games measured their current sources: game 1 **-0.08 %** (and -0.53 % on its PAL 3/4 scene), game 2 **-2.26 %**; lot E1 (far pointer): game 1 0 / -0.07 %. State identical everywhere. So the lot's gain is real on game 2 and almost null on game 1's current scene: a frozen gauge drifts from a living game. To do at the start of the next compiler lot: `jauge.py --geler` then rebuild the reference (`depart`) with the SDK of that day; quote every gauge figure with its snapshot date. Also found by game 1: three commits of the X/Y lot on `develop` named a submodule commit that `compiler/PINS.md` did not yet carry (`make verify-toolchain` refuses a clone built at one of them): `check_submodule_pins.py --coherent` now runs in the pre-push hook on every pushed range.
4. **Threads waiting for others**: advance 2 (the second game's return on the map engine lot); the capacities thread to
   luna (its point 10, the mirror fold per mapper, also explains the false slowdowns game 2 saw in its own profile list).
5. **Models (owner, 2026-11-12)**: no Fable sub-agent without need; the complex pair is Opus / Opus; Fable only for the written
   contest of a compiler lot's plan and the review before closing a lot that touches the whole corpus. Short lots, no wide
   re-reading.
6. **Housekeeping**: the worktrees of the finished lots (`agent-a878…`, `agent-a8c6…`, `agent-a90e…`, `agent-a912…`) are merged
   and can be removed (their branches are not ancestors of `develop`, checked 2026-11-12: compare by content first); the tests marker is per working tree and a `make tests` run in the background with its output
   redirected does not set it (touch this tree's marker after reading the green log, as the hook's message says).

**Priority lot since 2026-11-11 (owner): bring C closer to hand-written assembly, then an 8-direction scrolling brick.**
The plan and its state: `.claude/notes/chantiers/c_asm_gap.md` (approaches A1 to A9, starting factor per routine, order).
- Done: `devtools/gapbench` (`make bench-gap`, `--check` in `make tests`): twelve routines, C against assembly by hand,
  geometric mean 2.55, from 1.57 to 9.01. The plan is written and sent to the conductor for review.
- The plan is reviewed and granted (2026-11-11): target a geometric mean under 1.5, no row above 2.0. Two lots run, each
  written by an Opus agent in its own worktree (`a6a-lot1`: five backend rules; `a7-string`: `memcpy` / `memset`). Nothing of
  theirs is on `develop` until this session has re-run every gate (Class A: A/B corpus proof, both differential hunts,
  `make tests`). The live state is the last section of the plan.
- **The project is no longer reactive (owner, 2026-11-11)**: it proposes its own chantiers. Due at the end of the A9
  analysis: a plan of approaches FOUND BY US, beyond A1 to A9 (each with its aim, expected gain, first measurement, and what
  kept us from thinking of it), handed to the conductor for the owner. At every lot the question is asked: what would the best
  compiler team do in our place, and which yardstick are we not looking at.
- **A9 is unlocked and running**: the commercial ROMs are on the machine (the emulator's local corpus, outside git); they are
  read in place, disassembled by execution trace (`luna state --trace-log`) into a folder outside every repository. A first
  game is being read (sprite list, object loop, main loop, NMI and transfers, scrolling, collisions, arithmetic,
  decompression); only patterns in our words and our own measurements come back here. Next: a game with 8-direction scrolling,
  before the scrolling brick is written.
- **The complete plan is `chantiers/c_asm_gap_plan.md`** (2026-11-12): every cost item with its share, cause, what shipped
  games do, axis, expected gain and owner; the small items with their lessons; what is missing and who is asked; the path lot
  by lot (L2 jump tables, L3 the machine-level optimiser, L4 values off the stack and the call, then L5 to L9, S1 the scrolling
  brick, S2). It is with the conductor for review: **no new lot before its answer.**
- **Done and pushed 2026-11-12**: `memcpy` / `memset` on the block move; the map engine, `oamHideRange`, `oamSetAttr`; the first
  compiler lot (five rules, QBE fork `70fa55c`). Bench weighted mean 2.24 -> 2.00; the two games -2.8 and -1.1 % of busy time,
  state identical at every game step. One thread per advance to the games and luna for their factual return.
- **The plan is accepted (conductor, 2026-11-12) and carried to the owner.** Order: L2 (jump table; its note
  `chantiers/gap_lots/L2_jump_table.md` holds the contest, the prediction and the cost-and-risk line, as every lot must before
  it starts, and the predicted-against-measured gap after), then examine L7 (the library's hot routines) BEFORE L3, then L3 cut
  into measurable steps, L4. The scrolling brick after L2. L2 starts when a complete CI run is green on the compiler lot.
- **Owner's decisions of 2026-11-12**: no console for several months (what only hardware can judge is written "not measured on
  a console"); after L2 and its validations, a PRIVATE measurement of the bench's routines through another 65816 C compiler
  (the non-commercial one with sources and a SNES target): nothing redistributed, nothing published, nothing written to its
  author; if the gap is flagrant, the two compilers' assembly compared routine by routine and every difference attributed to a
  cause in ours before any other decision. The two other commercial compilers are ruled out by the owner (2026-11-12, after their cost was read:
  one's price is not published and it is Windows-only, the other targets another machine): the non-commercial one stays, alone.
- **Public assembly projects under MIT or zlib as a reference column of the bench** (their routine as it is with its notice,
  our C twin on its data): recommended, with the owner. The invitation to outside programmers stays aside.
- **Two gauges at every compiler lot** (granted): the bench, and the two games' busy master cycles a frame, measured by the
  gauge tool that lives outside every repository (beside the yardstick dumps; its read-me says how), then confirmed by the
  games. The agreed proof of equivalence: each game's test suite, and the state at every game step against the reference.
- **Push order**: a fork first, then the commit that pins it (the pre-push hook now refuses the other order).
- **After each measured advance: one thread to luna and the two games** for their factual return on their real case (owner,
  2026-11-11: every other project is on hold and wakes only for that).
- Waiting: the conductor's review; the commercial ROMs of A9 (the owner provides them; disassembly happens outside this
  repository, only patterns in our words come back); the patterns of A2 from a partner port; the games' top-ten profiles (A4).
- After the gap: the 8-direction scrolling brick (tile ring, new row and column in grouped DMA, VBlank budget measured), from
  a partner port's real case, with an example, a luna test and its figure; that port tries it.
- **Weekly duty (owner, 2026-11-11)**: a thread to the conductor with three new approaches or outside yardsticks not tried
  yet, each with its expected gain, cost and what kept us from it. First one 2026-11-12, then every week.
- **Why the yardstick of commercial games did not come from us**: our yardsticks were PVSnesLib (`sdkbench`, `libbench`,
  `twinbench`) and ourselves (`cyclecount`); being 63 % ahead of PVSnesLib stood in for a measurement against hand assembly
  or a shipped game, and since 2026-10-29 the project worked reactive only. An outside yardstick is now part of every plan.

Reactive work goes on beside it (the games, luna, the porting tools):

- **Done and pushed 2026-11-10 (develop `5ab849a7`, then `213d50df`)**: cproc refusal of mode/vector_size and the
  other attributes that change the program (fork `82e1c1d`; lesson written in the porting kit); stable section sort in the wla-dx
  fork (`96e7860d`, 6 patches; Linux ROMs byte-identical, the CI hash lists must now be equal on macOS and Windows: READ
  THEM, run `devtools/compare_rom_hashes.py`); luna pin v1.43.0 with `audio.json` and `wram.json` recaptured (reasons in
  `WRAM_JOURNAL.md`); the interrupt gate (`testing/race_gate.py`, fixture `race_sweep`), SKIPPED until the pin carries
  `race-sweep` (first tag after v1.43.0: then bump, run, and confirm 353 addresses clean); `testing/mesen2_corpus.toml`
  (25 ROMs) for luna's second-opinion job.
- **Done 2026-11-10 evening**: the ROM-hash lists are read. The one ROM that differed between systems (`backgrounds/mode1`,
  Windows then macOS) came from the LZ77 encoder's uninitialised pad bytes (`eedca343`); run `42897584853` green, 65/65 equal on
  Linux, macOS and Windows; the temporary mode1 diagnostic is out (`de285b66`); lesson written in the porting kit.
- **Done 2026-11-11**: the character-ring brick: second port measured (753 unique characters at most, 303 in the worst window, per-level
  sheets 507-973, only one boss sheet at 1 165 solved by hand): below 1 024, so the need stays one port's; no module (two-games rule);
  the asker has the VRAM queue and an offered per-frame cost measurement on a stand-alone reproducer (waits for its N).
  Reopens with a second game above 1 024.
- **Done 2026-11-11**: (3) the >99 LSB sample differences reported to luna (note `partners/luna/2026-11-11_to_luna_v1.43.0-sample-diff-above-99-lsb.md`, `e87f18da`); waits for its answer.
- **To do next**: (4) the main tree's `compiler/` submodules are behind the pins: a
  `git submodule update` and a full rebuild are due, announced to the partners first.
