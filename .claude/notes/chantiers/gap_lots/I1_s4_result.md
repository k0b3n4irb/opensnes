# I1 S4: the size rule

Step S4 of `I1_inline_plan.md` (sections 1, 4, 5 'S4', 'Revision after S2', 'Contest'). Branch `i1s4`, from `develop`
`79149566` (cproc `18eecca`, qbe `0f52456`, the S3 pins). Pair: Opus writes and measures (one agent, no sub-agent on this
step). Names neutral: the games. Nothing of the games is measured here: their gauge is run by the maintainer session.

## Prediction (written 2026-11-14 before any code of this step; committed alone)

The plan's S4 prediction, with the revision after S2 (unit budget 128 KB estimated, per caller 8 KB, per section 16 KB,
threshold 72 IR instructions, free copies at 2 instructions or fewer, factor 3.24 bytes per IR instruction), copied and
made precise where the plan left a range for this step to fix:

1. **Switch `QBE_NO_INLINE_SIZE=1`: byte-identical to S3 (`develop` `79149566`)** on every `.asm` and `.sfc` of the corpus
   and the four library object sets (1 087 files of the reference build, `cmp`). The default build without the switch
   changes the corpus.
2. **Corpus code**: +20 to +35 KB of code over all example ROMs (H10: +27 KB, S2's estimate 23.0 KB over 52 units, the
   library's 8.9 KB counted once there and once per linking ROM here), read from the `.sym` code sections; no example
   changes its ROM size or its number of banks; every `diff_corpus.py` MATCH (a boot offset allowed and explained).
3. **Budgets on the corpus and the library**: no unit reaches the 128 KB unit budget, no caller the 8 KB per-caller cap
   nor the 16 KB section cap (S2: nothing above 5.1 KB on the games, the corpus is smaller); the trace shows 0 refusals
   `unit budget`, `caller budget` and `section`. So H7 (priority by loop depth) is not settled by the corpus: said.
4. **gapbench**: `callst` and `chainst` faster by at least 30 % of the gap to the assembly twin, both speeds (SlowROM
   `callst` <= 53 923 and `chainst` <= 66 897 master cycles; FastROM `callst` <= 46 814 and `chainst` <= 58 754); `fptr`
   faster or equal (its `fp_index` is static with two sites); no row slower than `baseline.json`. `call` / `chain` (global
   callees) unchanged.
5. **The differential hunts**: 0 failures in 10 000 seeds each (`difftest.py --seeds 1-10000`, `difftest_stmt.py --seeds
   1-10000`), with the new seed class in which helpers are copied, and its half run under a tiny
   `CC_INLINE_CALLER_BUDGET` (partial copies).
6. **Checks**: `auto_inline` and `auto_inline_leaf_rule` fail as written on purpose (`twice`, two sites, is now copied):
   their lines are rewritten to pin the new rule, and the old rule is pinned under `QBE_NO_INLINE_SIZE=1`
   (`inline_size_off`). Every other check passes unchanged (S3 marked `noinline` where a static must stay).
7. **Injected faults**: `QBE_INLINE_FAULT=stale` stops the compile of the stale-site fixture on the `splice_fn` assertion;
   `QBE_INLINE_FAULT=standalone` fails the link of the two-unit fixture or gives values that differ from the model;
   `QBE_INLINE_FAULT=order` (walk 2 before walk 1) turns a `.checks` line red (a one-site callee after a copy in a caller
   built near the room).
8. **Baselines**: `luna_runner.py --compare` unchanged (the same pictures at the same frames; a boot offset is a
   `diff_corpus` matter, not a picture); `wram_regress.py` moves on the examples whose direct-page temporaries or stack
   contents change (re-captured with that reason); `audio.json` may move on audio examples whose SPC700 talk is
   re-timed (re-captured only with the `luna diff --audio` quote); `bench_baseline.json` moves only if a measured
   workload has a static of two sites or more (hypothesis: none, its workloads are global).
9. **Side effects**: the stack floor gate of `rom_coverage.py` green; `nmi_budget.py` under 12 000 mclk; `make
   test-sframe` green and `QBE_SFRAME_FAULT=noedge` still red; compile time of the corpus within +20 %.
10. **The games** (private, not measured by this step): the plan's revised prediction stands, **-1.23 % (game 1) and
    -1.26 % (game 2)** busy per step, range -0.9 to -1.6, equivalence identical. Private gauge: to run by the maintainer
    session.

## What was built (qbe `0f52456` -> `57101dd`, local branch `i1s4`; cproc unchanged)

- **The plan** (`inline_plan`, after the sites are counted and the addresses scanned, before any splice): the unit's
  direct calls with the loop depth of each site (QBE's `fillloop` on the parsed IR, 8^depth), Tarjan over them (a
  cycle is never a candidate), the candidates (static, address never taken, no section, not `noinline`, no `inline`
  keyword, two sites or more, a body the splicer takes, own size <= `CC_INLINE_SIZE` 72, a name not ending in
  `.b/.w/.l`), the final size of every function (own + what will be poured into it, minus the call sequences: `npar`
  `arg` and one `call`), the cost `(sites - 1) * final - sites * (npar + 1)` at `CC_INLINE_BPI` 3.24 bytes an IR
  instruction, `static inline` counted first and never refused, own size <= 2 free, the others admitted by weight /
  cost (ties by name) while the unit stays under `CC_INLINE_UNIT_BUDGET` 131 072. The decision is a set of callees.
- **Walk 2** (`inline_check2`), after walk 1 on the whole unit, with its own marks (`checked2`, `busy2`): each call to
  an admitted callee, re-found by name in the block as it stands; the callee's walk 2 first; refused for a caller with
  a section of its own (`ramcode caller`), today's tests with the site count left out (`tmp room`, ...), the caller's
  added bytes over `CC_INLINE_CALLER_BUDGET` 8 192 (`caller budget`), its size over `CC_INLINE_SECTION_BUDGET` 16 384
  (`section`). After a copy the walk goes on into the clone. A caller left without a call is a leaf.
- **`splice_fn`** asserts that the instruction at `call_idx` is a direct call to the callee it was handed (internal
  error naming the site otherwise).
- **`inline_settle`**: a function absorbed (one site), consumed (`static inline`) or admitted (the size rule) is not
  emitted only when no function that stays names it (calls, addresses, phis, jumps), to a fixed point; a `static`
  that something still names is emitted. A non-static `inline` keeps C99's rule. The trace prints one `settle` line
  per copied function (`sites=`, `copies=`, `left=`).
- **`-d`** collects the module like a compile and traces each function after the module's inlining (it used to run
  `inline_check` per function as parsed, before any site count, with the callees parsed earlier already freed).
- Trace: `plan` lines (candidates, cost, weight, admitted or the refusal), `decline ... (pass=3 ...)` for walk 2,
  `absorbed ... (size rule)`, `final2` (walk 2's `final`; `final` stays walk 1's, which `i1_calib.py` reads),
  `settle`. Faults: `QBE_INLINE_FAULT=stale`, `=standalone`, `=order`.
- Tests: checks `inline_size`, `inline_size_off`, `inline_size_budget`, `inline_walks`, `inline_order` (generated once:
  269 lines of arithmetic put `big` at 1 690 temporaries), `inline_dtrace`; `run.py` gained `stderr` (the case
  compiled with `CC_TRACE_INLINE=1`) and `dtrace <func>`; fixture `testing/fixtures/compiler/inline_size`; a case of the
  `jump_table` fixture; two golden cases of `opensnes-rom`; `difftest_stmt.py` seeds 8k+2 and `difftest.py` odd seeds.
  `auto_inline`'s line "two call sites: stays a function" is rewritten (the old rule is pinned under the switch by
  `inline_size_off`).

## The contest's eight requirements, in their order

1. **Stale planned site.** The plan keeps no (block, index): walk 2 re-finds each call by the callee's name in the block
   as it stands, and `splice_fn` asserts. Fixture `inline_size`, `stale_block()`: a one-site `single` then `pa`, `pb`
   (same arity, two sites each) in one block; values against the host model. Under `QBE_INLINE_FAULT=stale` (walk 2
   hands `idx + 1`) the build stops: `inline.c: dying: internal error: inlining pa.main into stale_block: the
   instruction at inl1_single.main.ret[6] is not a call to it (stale site)` (the block named is the one walk 1 made:
   the rewrite the contest feared, caught).
2. **`absorbed` only when no reference remains.** `inline_settle` scans the final IR. Fixture: `util.c` and
   `sub/util.c`, each a `static mix` emitted `mix.util`; `util.c`'s is copied into `util_rom` and declined in the RAM
   function `util_ram` (kept), `sub/util.c`'s copied at both sites (dropped). Under `QBE_INLINE_FAULT=standalone`:
   `util.c.o: util.c.wrap.asm:63: FIX_REFERENCES: Reference to an unknown label "mix.util".` (the link fails). With the
   switch both units keep their `mix` and the link refuses the second (`Label "mix.util" was defined more than once`):
   the binding to a namesake cannot happen silently today; the test's switch-off build renames the second. A candidate
   whose name ends in `.b/.w/.l` is refused (`unit name`), and the back end already refuses such a static.
3. **`checked` / `busy` across the walks.** Walk 1 unchanged; walk 2 has `checked2` / `busy2`; it goes on into a clone.
   Check `inline_walks` (`f` before `g` before `h`, `g` of two sites calling `h` of two sites): no call in `f`, neither
   emitted, `settle g: sites=2 copies=2 left=0`, `settle h: sites=2 copies=2 left=0`, one `final2` for `g`.
4. **`__ramcode` through transitivity.** Decided: no copy into a caller with a section of its own, and walk 1 absorbing
   before walk 2 copies means a body poured whole into a RAM caller holds no copy either. Check `inline_size`: `mid`
   (one site) is poured into `ram1` with its call to `helper`, which stays `jsl helper` there while `rom_user` holds the
   copy; `ram2` keeps both calls to `small`, a ROM function. The window refuses an overflow at link: the fixture built
   with `RAM_CODE_SIZE=16` stops on `Section ".ram_code" (17 bytes) overflows from ROM bank 1.`
5. **Stack depth.** Fixture `inline_size`: `rec()` 24 deep, two copies of `mixs` a level; luna's deepest S over 20
   frames, here and in the same sources built with `QBE_NO_INLINE_SIZE=1`: **$1F43 with the copies, $1EDF without**:
   100 bytes shallower (hypothesis, not traced: the return address and arguments of the call to `mixs` at the deepest
   level are gone, and the copies' temporaries take direct-page slots, not rec's stack frame). Ceiling written in the
   test: the copies may deepen it by 96 bytes at most (4 a level).
6. **Call table multiplicity.** Decided not compared, and pinned: golden case "check: two calls to one callee, one
   table line" accepted (the placement needs reachability, a second call to the same callee adds none), and its
   reverse "a table line with no call left" refused (`the table of f.a lists a call to h.a that its text does not
   make`). `make test-sframe`: 65/65, the `noedge` fault still refused.
7. **New seed class.** Taken from the even (global) seeds, not from 4k+1: seeds 8k+2, every helper `static` and called
   from two to four places (the first in a loop), a helper calling another, a dense switch helper one time in four;
   seeds 16k+10 under `CC_INLINE_CALLER_BUDGET=48`. Seed 10 under the budget: 112 `caller budget` and 18 `tmp room`
   refusals, the helpers kept for the calls left; seed 2 at the default budget: every admitted call copied.
   `difftest.py`: odd seeds' `f` is `static`, called twice, the second value checked against the first.
8. **`-d` trace.** Check `inline_dtrace` (`dtrace user`: the calls of the last IR `qbe -d G` prints for `user` equal the
   `jsl`/`jml` of its text). S3's `bin/qbe -d G` on the same IR printed, for `user`, calls to `once` and `twice` (a
   one-site callee and a callee the size rule copies) beside `ext` and `kept`: the comparison fails on it; it passes
   now.

## Found by the hunt: a defect of the back end, fixed (qbe `c3b1f3f`)

`difftest_stmt.py` seed 9922 (class 8k+2, default budget) stopped the build: `internal compiler error: %inl.4693 has
no stack slot and is an operand read from the stack in t9.main (temp_noslot)`. Reduced (line-wise, the error kept) to
two statics copied into a loop, `h5(-128)` (a dense switch) feeding `h3`, whose `!x0 ? 1L : (x1 >= 1024L)` the
optimizer turns into `%inl.54 =w ceqw %.12, 0` used once as arg[1] of `%.18 =l xor %.4.7, %inl.54`. The same reduced
program with both helpers `static inline` fails with S3's `bin/qbe` (develop): the defect predates this step, the size
rule only made the shape reachable from plain statics. Cause: `mark_dead_stores` case 3 drops the store of a temp used
once as arg[1] of the next commutative operation, counting on the 16-bit path's swap (the operand taken from the
A-cache); the 32-bit pair path of `xor` (and `add`, `and`, `or`) loads r0 and reads r1 from its slot. Fix: case 3 only
when the operation is not Kl. Loud before (the slot check refused the read), never a wrong value. Proof: seed 9922
clean; pinned as `p16` in `difftest_stmt.py`, red without the fix (the same internal error), green with it; the corpus
and the library byte-identical with the size rule on before and after the fix, and with the switch against develop.
Found and fixed in the first attempt.

## Result

(Measured after the code above; the predictions were committed first, `8236abbf`.)

| # | point | predicted | measured | |
|---|---|---|---|---|
| 1 | switch `QBE_NO_INLINE_SIZE=1` | byte-identical to `79149566` | **1 087 files, 0 differ** (65 `.sfc`, the examples' `.asm`, the four library sets), the same file set; replayed after the fix (lib + examples rebuilt) | held |
| 2 | corpus code | +20 to +35 KB, no ROM size or bank change, every `diff_corpus` MATCH | **+29 227 bytes** of `.text`/RAM code over 65 ROMs (237 495 -> 266 722, +12.3 %), 54 ROMs changed, 41 larger, 13 smaller; no ROM file changes size; `diff_corpus` **64/65 MATCH, 1 DIFF** (`basics/ppu_reset` frame 100, explained below) | code held; one DIFF |
| 3 | budgets on corpus + library | 0 refusals `unit budget`, `caller budget`, `section` | 123 candidates, **123 admitted**, 450 copies, 0 `unit budget` / `caller budget` / `section`; largest unit planned cost 7 381 bytes; **19 `tmp room`** refusals (11 in `rooftop_run`, 8 in `lzss_ram`, both their `main`); 114 copied functions not emitted, 9 kept for a remaining call | held (H7 not settled: no budget binds) |
| 4 | gapbench `callst` / `chainst` >= 30 % of the gap | slow <= 53 923 / 66 897, fast <= 46 814 / 58 754 | `callst` **23 661** / **19 799** (below its assembly twin: 0.72 / 0.71), `chainst` **53 060** / **44 830** (1.43 / 1.41); `call` / `chain` unchanged | held, beyond |
| 4 | `fptr` faster or equal, no row slower | | `fptr` 52 486 -> 52 489 SlowROM, 45 111 FastROM unchanged (`fp_index` is copied, but outside the timed path); ten rows move by 2 or 3 master cycles either way (placement, as in S3) | not faster; no row slower beyond placement |
| 5 | hunts, 10 000 seeds each | 0 failures | see Gates (the first run found seed 9922, above) | |
| 6 | checks | `auto_inline` rewritten, the rest unchanged | `auto_inline`'s `twice` line rewritten; 6 new checks; `make test-compiler` 162 passed | held |
| 7 | faults | each named check red | `stale`: the fixture's compile stops on the assertion; `standalone`: the fixture's link fails (`unknown label "mix.util"`); `order`: `inline_order` fails (6 assertions) | held |
| 8 | baselines | `--compare` unchanged; WRAM moves; audio may move; `bench_baseline` unchanged | `--compare`: **one picture moved** (`basics/ppu_reset` frame 100); WRAM: 33 of 65 streams drifted; audio: 7 of 9 hashes, all `luna diff --audio` MATCH (max delta 0.65 %, shift 8 samples at most); `bench.py`: 1 215 cycles against 1 211 ("no regression", baseline kept) | `--compare` did not hold |
| 9 | side effects | stack floor green, NMI < 12 000, sframe green, compile time within +20 % | stack floor green (smallest margin 772 B); NMI 12/12 within budget, `audio/snesmod_music` 6 190 against 5 630 (+9.9 %, at the edge of the 10 % gate); `test-sframe` 65/65, `noedge` refused; **QBE time on the 104 corpus and library units x5.3** (0.68 -> 3.64 s of CPU) | compile time did **not** hold |
| 10 | games | -1.23 % / -1.26 % | not measured here | private gauge: to run by the maintainer session |

### What did not hold, said plainly

- **Compile time.** Predicted within +20 %; QBE's CPU time over the 104 IR units of the library and the corpus goes from
  0.68 to 3.64 s (x5.3), three runs each, interleaved (`build/s4/qbetime.py`, not kept). Per unit: `rooftop_run`
  0.17 -> 1.35 s, `breakout` 0.04 -> 0.75 s, `lzss_ram` 0.01 -> 0.55 s, `rpg` 0.03 -> 0.43 s. Their largest function
  (`main`) doubles in length (`lzss_ram` 896 -> 2 017 lines of text, `rooftop_run` 1 629 -> 3 199): the cost is
  superlinear in a function's length. callgrind on `lzss_ram`: 60 % `strncmp` and 19 % `pp_slot_live_at`, the
  per-query liveness of the text passes that searches each branch's label over the whole function
  (`pp_slot_live_at` and `pp_branch_target`, `emit.c`). The parallel build of the library and the examples: about 1 s
  more (2.5 -> 3.5 s wall, noisy). Not fixed here (the emitter is outside the step): a label index built once per pass
  would make each query linear, byte-identical by construction. **The games' build time is a risk to measure** (their
  copies make functions of the same lengths, more of them).
- **A picture moved.** `basics/ppu_reset` frame 100: the HDMA wave's table rebuild (`fillWaveTable` with `hdmaSin`,
  copied into `hdmaWaveUpdate`) now fits in two frames, three before (`hdma_wave_frame` at frame 100: `$3E` before,
  `$5E` after; between frames 60 and 100, 13 turns before, 20 after): the same wave at another phase (`luna diff
  --sequence`: SAME-SEQUENCE, 35 of 38 pictures in the same order; frame 200, screen B, MATCH). Re-captured with that
  reason.
- **`fptr` did not get faster** (its `fp_index` is copied, outside the timed loop).
- **libbench** (not in the prediction, read for the record): `worldc` 53 409 -> 41 318, `oamfast` 82 825 -> 73 278,
  but `world` 31 747 -> 33 186 (+4.5 %), `worldo` 36 309 -> 37 751 (+4.0 %), `idle` 5 852 -> 6 233 (+6.5 %): a
  `static` set-up helper of three sites (`world_setup`) is now copied into row functions that call the library, so
  its loop no longer runs as a leaf (the trade the leaf rule once refused). All 23 rows within the bench's 10 %.
- **The NMI of `audio/snesmod_music`** is +9.9 % of its reference (6 190 against 5 630 mclk, worst frame 83), inside
  the gate by 0.1 point; not analysed here.
- **The free copies** (own <= 2) are almost never met: the smallest real helper (`return x;`) counts 4 IR
  instructions (`par`, the copy, the extension, `ret`); a getter of a global counts 1.
- **The stack** went the other way from the contest's worry on the fixture (100 bytes shallower with the copies).

### Side effects (plan section 6)

| what | measured |
|---|---|
| ROM size per example | no ROM changes size or bank count; ten largest growths of code: `audio/sample_swap` +4 481, `games/rooftop_run` +3 883, `audio/soundboard` +3 382, `audio/sfx_voices` +3 105, `demos/rpg` +2 250, `memory/lzss_ram` +1 814, `audio/sfx_from_wav` +1 778, `games/breakout` +1 671, `games/blocks` +1 163, `hdma/hdma_wave` +631 bytes (the audio examples carry the library's `audio.c` copies) |
| Bank $00 | every link green (`opensnes-rom check`) |
| Library objects | the library's statics copied: `hdma.c` (`hdmaSin`, `runsEffect`, `fillWaveTable`), `audio.c` and others: see the trace (`make count-i1-calib` path, or `CC_TRACE_INLINE=1` on `lib/source/*.c`) |
| Compile time | above |
| Stack | `rom_coverage.py` floor green, 216 legs, smallest margin 772 bytes; fixture: 100 bytes shallower |
| NMI | `nmi_budget.py` 12/12, worst `games/rooftop_run` 8 732 mclk (73 %) |
| gapbench | above; `baseline.json` keeps `callst` / `chainst` (their four entries only) |
| Benchmark page | `bench.py` 1 215 cycles against 1 211, "no regression": `bench_baseline.json` and `docs/BENCHMARK.md` unchanged (anchors 13, 22 green) |
| Boot length | `diff_corpus`: every MATCH at offset +0 |

### Triage (what a reader can check by hand)

| example | why kept | what to look for |
|---|---|---|
| `basics/ppu_reset` | the one picture that moved (the HDMA wave twice as often per frame budget) | screen A waves smoothly, faster than before; at two seconds screen B, flat, no wave |
| `hdma/hdma_wave` | the library's `hdma.c` copies in its per-frame update | the wave travels without tearing; no line out of place |
| `audio/sample_swap` | the largest growth (+4.5 KB, `audio.c` copies) | the samples play and swap as before |
| `games/rooftop_run` | the largest `main`, 11 copies refused for the temporary room | the run plays, sprites and scrolling as before |
| `memory/lzss_ram` | eight tests copied into `main`, the other `tmp room` case | every line of the screen says the test passed |

## Gates (replayed on the final compiler, qbe `c3b1f3f`)

- `make clean && make`: `BUILD_EXIT=0`, the same two warning lines as the reference build (clang `-rdynamic`, CMake
  deprecation).
- **Switch byte-identical**: `QBE_NO_INLINE_SIZE=1` build of the library and the examples against the reference of
  `79149566` taken before any change: `files=1087 differ=0`, `same file set` (a full `make clean && make` before the
  fix, the library and the examples rebuilt after it). With the rule on, the corpus is byte-identical before and after
  the fix (`on-mode files differing after the fix: 0`).
- `make tests`: `ALL CHECKS PASSED (luna)` (`COMPARE: 65/65 ok` twice, coverage 63 OK / 2 INPUT-DEP / 0 DEAD / 0 FAIL
  both power-ons, `Audio regression: 9/9 match`, `WRAM regression: 65/65 ok`, `NMI VBlank budget: 12/12`, the
  `inline_size` fixture 7/7, `jump_table` 21/21 with `pickst() copied twice into twocopies(), a table in each copy`,
  `sframe corpus: 65/65`, gapbench `OK, nothing above baseline.json`).
- `make test-compiler`: `Compiler checks: 162 passed, 0 failed (+0 compile-only; 0/0 unchecked ratchet; 34/34
  refusals pinned)`; `QBE_INLINE_FAULT=order ... --only inline_order`: `0 passed, 1 failed`.
- `make lint`: exit 0. `make lint-docs`: `OK: no doc drift detected.` `make verify-toolchain`: `OK: toolchain pins
  match (3 submodule(s) verified)` (qbe `c3b1f3f`, 159 patches).
- Hunts, 10 000 seeds each, on the final compiler: `difftest: 10000/10000 seeds clean, 240000 expressions in 4
  shapes`; `difftest-stmt: 10000/10000 seeds clean, 100000 programs`. (The first run, before the fix:
  `difftest: 10000/10000`; `difftest-stmt: 9999/10000`, seed 9922, the section above.)
- `diff_corpus.py --ref <the examples built before>`: `DIFF CORPUS: 64/65 match, 1 diff, 0 missing` (`basics/ppu_reset`
  frame 100, explained above; every other example MATCH at offset +0).
- `make test-sanitizers`: `SANITIZERS: OK — cproc-qbe, qbe, wla-dx and the asset tools ran the fixtures, the lib, the
  goldens, the corpus and the upstream suites without an ASan/UBSan report` (its compiler checks 162 passed), then
  `restoring the normal binaries`.
- Faults on the final compiler: `stale` (`inline.c: dying: internal error: inlining pa.main into stale_block: the
  instruction at inl1_single.main.ret[6] is not a call to it (stale site)`), `standalone` (`Reference to an unknown
  label "mix.util"`), `order` (above), and the pin `p16` red without the fix.
- Baselines moved, each with its reason in its commit: `testing/baselines/baselines.json` + `basics_ppu_reset.png`
  (one picture), `wram.json` (33 streams, `WRAM_JOURNAL.md` line), `audio.json` (7 hashes, the `luna diff --audio`
  lines quoted), gapbench `baseline.json` (`callst` / `chainst` only). Not moved: `bench_baseline.json`, the other
  gapbench rows, libbench, sdkbench.
- Submodule commits local, branch `i1s4` of `compiler/qbe` (`57101dd`, `c3b1f3f`), not pushed; cproc unchanged.

## Private gauge: to run by the maintainer session

The games are not measured by this step. For each game, against this branch's `bin/` (`make clean && make` on
`i1s4`, qbe `c3b1f3f`), from the game's frozen reference:

```
# 1. the switch: byte-identical to the S3 build (gate)
QBE_NO_INLINE_SIZE=1 make -C <game> clean all   &&  cmp <game>.sfc <S3 reference .sfc>
# 2. the size rule: the gauge against the S3 build, equivalence at every step
make -C <game> clean all  &&  <gauge> --compare <S3 build> <this build>      # busy per step, late frames
# 3. budgets and refusals, per unit
CC_TRACE_INLINE=1 bin/qbe -t w65816 -o /dev/null <each kept IR unit> 2>&1 | grep -E 'plan unit|refused|pass=3'
# 4. the call table on the game's objects
bin/opensnes-rom check --objects <game>/linkfile <game>.sym
# 5. compile time of the largest unit, with and without the rule (the risk above)
time bin/qbe -t w65816 -o /dev/null <largest unit .ssa>;  time env QBE_NO_INLINE_SIZE=1 bin/qbe ...
# 6. code growth per unit and lowest stack pointer over the gauge windows
python3 devtools/sframe/i1_calib.py growth --before-sym <S3 .sym> --before-dir <S3 build> --after-sym <.sym> --after-dir <build>
```

Prediction to compare with: -1.23 % (game 1) and -1.26 % (game 2) busy per step, range -0.9 to -1.6; stop bar 1.02 %
on one game; code growth <= 1.2 x S0's (+57 KB / +175 KB) and <= 15 % of free ROM.

## Verdict of the maintainer session (2026-11-14, the private gauge replayed by me, the gates replayed by me)

Gates replayed on the worktree branch `i1s4` (qbe `c3b1f3f`): `make clean && make` 0, `make tests` ALL CHECKS PASSED, `make test-compiler` 162 passed / 0 failed, `make lint`, `make lint-docs`, `make verify-toolchain` OK; **the switch `QBE_NO_INLINE_SIZE=1` gives 65 ROMs byte-identical to `develop`** (65 / 65), and the default build differs from it on 54 of 65.

Compile time, replayed on one example game: 0.51 s with the switch, 1.77 s without (3.5 times).

**The gauge** (the same frozen sources for both builds, base = `develop` with S3, candidate = `i1s4`; state identical to the reference at every step, both games):

| | busy a step | late frames | code |
|---|---|---|---|
| game 1 | 368 101 -> 369 871 (**+0.48 %**) | 156 -> 169 | +30.1 % |
| game 2 | 609 299 -> 601 398 (**-1.30 %**) | 417 -> 408 | +51.6 % |

Prediction: -1.23 % and -1.26 % (range -0.9 to -1.6), stop bar 1.02 %. Game 2 held (-1.30 %). **Game 1 is slower**: the functions that absorbed copies got slower (`stepOther` +44.9 %, `stepPlayers` +22.2 %), the ones that disappeared into their callers gained little (`spritesUpdate` -2.0 %), net +0.48 %. Replayed with the games' live sources (`--sources-vivantes`): the busy time reads the same figures to the digit and the code a little different (game 1 +30.10 %); I cannot exclude that this flag changes the code and not the measured scenes, so the live-source replay is not an independent confirmation; it stands as "no contradiction".

**Reading.** The stop rule written before (a bar of 1.02 %, a consumer must not lose) is not met on game 1. Mechanism, hypothesis from the table (not yet read in the assembly): a caller that absorbs several copies grows past what the direct-page leaf frame and the temporaries hold, and its own cost rises by more than the calls it saved; the same shape as the bench's `world` +4.5 % and `idle` +6.5 % (a set-up helper copied into callers that call the library no longer runs as a leaf). The size rule counts bytes and the temporary room, not what the copy does to the caller's frame.

**Decision**: S4 does **not** land on `develop`. I1 is frozen after S4, as L4 was after S3: the code stays on the worktree branch `i1s4` (cproc unchanged, qbe commits `57101dd` and `c3b1f3f` local to the submodule). What is kept: (1) the measured table above; (2) the compile-time cost (3.5 times, in the back end's text passes, quadratic in function length: indexing the labels once per pass would fix it with output unchanged, a lot of its own); (3) the defect found by the hunt (`c3b1f3f`, a 16-bit second operand of a 32-bit commutative operation lost its stack slot, loud, pre-existing on `develop` with `static inline` helpers) which is worth landing alone; (4) the eight contest requirements, which any later inliner must meet. To reopen S4: a rule that prices the copy by its effect on the caller's frame (leaf kept, temporaries under the direct-page window), a prediction written first, the compile-time fix first.
