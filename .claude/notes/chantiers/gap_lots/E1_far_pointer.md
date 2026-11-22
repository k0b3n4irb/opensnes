# E1: a far pointer read where it lives, stepped on 16 bits

First short lot taken from the comparison with another 65816 compiler (a
private measure of the gapbench C compiled by it; only neutral figures leave
it). Written before the code; the measured column is filled when the lot ends.

## The axis

On `unpack` (two far pointers walked a byte at a time) the other compiler
spends 0.34 of what we spend. Its literal loop is nine instructions:
`sep / lda [p] / sta [d] / rep / inc p / inc d / dex / txa / bne`. Ours is
twenty-six, for two reasons:

1. **Every access copies the pointer into `tcc__r9`** (`lda.b lf+N / sta.b
   tcc__r9 / lda.b lf+N+2 / sta.b tcc__r9+2`, 16 CPU cycles) before
   `lda [tcc__r9]`. The pointer's own direct-page slot (`tcc__lf+N`, in
   bank $00's direct page like `tcc__r9`) is already a valid `[dp]`
   operand. `tcc__r9` is a cache of ONE pointer, so a loop that alternates
   two pointers stages at every access (`peephole_r9_loop` keeps one
   pointer there for a whole loop, never two).
2. **Every `p++` is a 32-bit addition** (`lda / clc / adc #1 / sta / lda /
   adc #0 / sta`, 24 cycles). The other compiler documents that a far
   object never crosses a bank, so it adds on the low 16 bits only and the
   bank byte never moves (`inc dp`, 7 cycles).

Part 1 is always exact: `[tcc__lf+N]` reads the same three bytes the staging
copied, as long as nothing writes them in between and the direct page does
not move. Part 2 is exact only under a guarantee, established below.

## Safety of the 16-bit step (established before the code)

Facts, each checked on this tree:

- **A C object never crosses a bank.** The backend puts each C object in its
  own section (`compiler/qbe/emit.c`, `wla_emitdat`): `.far.N` RAMSECTION
  `BANK $7E SLOT 2`, `.bss.N` / `.data.N` `BANK 0 SLOT 1`, `.rodata.N`
  SEMISUPERFREE in the asset banks. Every memory map the SDK ships
  (`templates/memmap*.inc`) gives SLOT 2 = `$2000-$FFFF`, SLOT 1 =
  `$0000-$1FFF`, a ROM slot of one bank. Measured with wlalink: a `FAR`
  object of `$E001` bytes is refused ("No room for RAMSECTION ... in RAM
  bank 126"), two objects that do not fit together are refused (never
  spilled into `$7F`), a const object of `$8001` bytes in LoROM is refused
  ("a section cannot span banks").
- **But an object may END at the last byte of a bank**: measured, `FAR u8
  other[16]` after a `$DFF0`-byte object sits at `$7E:FFF0-$FFFF`. Its
  one-past-the-end pointer, valid in C, is `$7F:0000`; stepped on 16 bits it
  becomes `$7E:0000`. Dereferencing it is undefined, but comparing it is not:
  `for (p = other; p < other + 16; p++)` would never end. So a stepped
  pointer must never be compared, stored, passed, returned or converted:
  only dereferenced, or stepped again.
- **A pointer from elsewhere carries no guarantee.** An argument, a call's
  result, a pointer loaded from memory, an `extern` object (assembly may
  define a buffer across `$7E:FFFF`, or in a bank of its own map): refused.

The rule, therefore (to be implemented exactly):

- A **root** is the address of a data object DEFINED in this translation
  unit (`$sym`, `$sym + k`): the backend placed it, in one bank.
- A **member** is a 32-bit temporary defined by a copy or a phi of members
  and roots only, or by `add` / `sub` of exactly one member or root and an
  integer (a constant, a 16-bit value, its extension, a product of it by a
  constant).
- Every use of a member is the ADDRESS of a load or a store, or the pointer
  operand of another member's definition. Anything else (a compare, a store
  of the pointer itself, an argument, a return, a truncation) removes it,
  and the removal propagates (greatest fixed point).
- Then the value of every member is its true address modulo 2^16 with the
  bank of its root, which is the true address wherever C allows a
  dereference. Only a member defined by `add` / `sub` whose pointer operand
  is a member TEMPORARY takes the 16-bit form; a root plus an index keeps
  the full sum.

What is refused, written: pointers from arguments, calls, memory, `extern`
objects, function addresses; any pointer that is compared, tested against
null, stored, passed or returned; an access outside the object the pointer
came from (undefined in C) that crosses a bank wraps inside the bank where it
used to reach the next one.

The provoked faults that must turn the tests red (`QBE_FARPTR_FAULT`): (1)
an `extern` object accepted as a root, the object placed in assembly across
`$7E:FFFF`; (2) the use rule ignored, a pointer compared with the end of an
object ending at `$7E:FFFF`.

## Predicted, before measuring

Cycles counted on the emitted text (SlowROM, about 7.8 master cycles a CPU
cycle on these lines). Three earlier counts of this kind were too
optimistic: the ranges lean to the prudent side.

| Gauge | Now | Predicted | Why |
|---|--:|--:|---|
| `unpack` C (SlowROM, mclk a call) | 603 353 | 280 000 to 350 000 (about -49 %) | a literal or copied byte: 117 CPU cycles -> 47 (two stagings, two 32-bit steps, a reload gone); plus the token's two stagings |
| `unpack` C, FastROM | 517 898 | same ratio | the instructions executed, not where they are fetched |
| `slots` C | 27 765 | about 22 100 (-20 %) | the walked slot pointer's high half (3 instructions) gone in each of 64 turns |
| `ents` C | 28 253 | about 25 400 (-10 %) | the same, 32 turns of a longer body |
| `col` C | 16 078 | about 13 300 (-17 %) | the same, 32 turns, the pointer kept in `tcc__r9` |
| `fill`, `copy`, `glide`, the others | | unchanged (0 cycles) | no staging and no stepped pointer in their loops |
| bench, weighted mean | 1.91 | about 1.86 | slots 4, ents 6, col 2, unpack 1 of 87 |
| bench, geometric mean of 20 rows | 2.32 | about 2.19 | |
| game 1 and game 2, busy mclk a step | 368 399 / 610 292 | 0 to -0.2 % | their hottest functions (90 % of the C time) have no staging and no `adc.w #0`: the games walk near pointers and index |
| game 1 and game 2, bytes of code | 116 169 / 135 154 | -300 to -1 200 each | 192 and 329 stagings in their units, 20 and 32 high-half steps; perhaps half removable |

## Cost and risk

Part 1 is a text rule run LAST in `relax_branches` (no later rule reads
`[tcc__lf+N]`; every earlier rule assumes a direct-page slot is not reached
through a pointer, `pp_slot_live_at` says so in words), with its own backward
liveness of `tcc__r9`. Part 2 is an IR analysis before emission and a
shorter Kl `add` / `sub`. A mistake in either is silent: the differential
tests (2000 fresh seeds each, 52001-54000), the two fixtures and their
faults, the corpus at equal frame, the games' state at every step.
`QBE_NO_FARPTR_DP=1` turns both off.

## Measured (2026-11-14, same day)

Binôme: one session wrote and measured it alone (no sub-agent, as the lot asked).

| Gauge | Before | Predicted | Measured | Verdict |
|---|--:|--:|--:|---|
| `unpack` C SlowROM | 603 353 | 280 000-350 000 | **281 068 (-53.4 %)** | at the optimistic edge of the range |
| `unpack` C FastROM | 517 898 | same ratio | 246 234 (-52.5 %) | yes |
| `slots` | 27 765 | ~22 100 | 21 928 (-21.0 %) | yes |
| `ents` | 28 253 | ~25 400 | 25 320 (-10.4 %) | yes |
| `col` | 16 078 | ~13 300 | 13 178 (-18.0 %) | yes |
| the 20 other rows | | 0 | within ±3 mclk (refresh phase) | yes; all 34 checksums equal |
| bench, weighted mean | 1.91 | ~1.86 | 1.85 | yes |
| bench, geometric mean, 20 rows | 2.32 | ~2.19 | 2.18 | yes |
| game 1, busy mclk a step | 368 399 | 0 to -0.2 % | 368 101 (-0.08 %) | yes |
| game 2, busy mclk a step | 610 292 | 0 to -0.2 % | 608 740 (-0.25 %) | a little better than predicted |
| game 1 / game 2, bytes of code | 116 169 / 135 154 | -300 to -1 200 | -277 / -772 | game 1 just under the range |
| sdkbench `bytes` / `strings` | 2 630 718 / 1 410 602 | not predicted | 2 055 930 (-21.8 %) / 1 314 002 (-6.8 %) | the 21 other workloads unchanged (A/B with the switch) |

Both games: state identical to the reference at every game step (341 and 700
steps). The prediction counted the emitted text instruction by instruction,
not a replayed trace; it held this time.

What the corpus does with it: 182 compiled units, 28 change (20 library units:
anim, asset, collision, panel, scene in the four builds; 8 examples). Stagings
into `tcc__r9`: 81 -> 65 in the examples, 300 -> 156 in the library (four
builds); `adc.w #0` / `sbc.w #0`: 13 -> 12 and 24 -> 24 (the corpus walks few
pointers by a counter from an object of its own unit); instruction lines 36 531 ->
36 456 (examples), 46 616 -> 46 024 (library). `QBE_NO_FARPTR_DP=1`: 182 of 182
units byte-identical to the compiler before the lot.

## The gates

- `make clean && make`: zero warnings. `make test-compiler`: 154 / 154 (ten
  checks that pinned `[tcc__r9]` accept `[tcc__lf+N]` for the access they
  name; `test_phi_acache`, which pins the staging itself, sets the switch).
- `make tests`: ALL CHECKS PASSED, after three explained re-captures:
  - audio, `chips/superfx_game_skeleton` (its own code and `asset` changed):
    `luna diff --audio --align-onset <before> <after> --until-frame 300` says
    DIFF, max delta 2.33 % (one window, 4000 ms; the others 0.00-0.57 %), the
    music starting 6 samples earlier; over 600 frames the windows go back to
    0.0x % with the same 6-sample shift. The picture is identical at every
    10th frame from 20 to 300. The C before `snesmodProcess` got faster: the
    phase of the music's processing, not the music.
  - WRAM, 13 ROMs: exactly the 13 that link a changed unit (their own
    `main.c`, or anim / asset / collision / panel / scene); the stagings no
    longer write `tcc__r9`. Journal line in `testing/baselines/WRAM_JOURNAL.md`.
  - manifest `string_nmi`: `r_cb_runs` (one NMI callback a frame while the
    main loop runs) 2 007 -> 1 808 with the bound at 2 000: the loop got
    faster, the bound measured its duration. Bound lowered to 1 500; the race
    counts (`r_cb_inside` 493, bad counts 0) unchanged.
- `diff_corpus` against the ROMs built before: 65 / 65 MATCH at equal frame.
- Differential tests, fresh seeds 52001-54000: `difftest` 2 000 / 2 000
  (48 000 expressions), `difftest_stmt` 2 000 / 2 000 (20 000 programs);
  `difftest_stmt` 52001-52300 under `QBE_FARPTR_ASSERT=1` clean.
- Fixture `testing/fixtures/compiler/farptr_dp` (in `make tests`): values
  against a host model, the placement it rests on read in the `.sym`, and its
  two provoked faults red (`QBE_FARPTR_FAULT=1`: `r_ext` reads `$7E:0000`;
  `=2`: `n_cmp` runs to its bound).
- The text rule's faults: `QBE_FARPTR_FAULT=3` (tcc__r9 taken for dead) fails
  22 of 30 `difftest_stmt` seeds; `QBE_FARPTR_FAULT=4` (a write of the slot
  ignored) changes no emitted text on the bench or the examples sampled and
  200 seeds stay clean: the emitter never writes a pointer's slot between its
  staging and its access today, the guard has no case to catch.
- The independent check, `QBE_FARPTR_ASSERT=1`: the staging stays and every
  rewritten access first compares the slot with `tcc__r9` (three bytes,
  `stp` on a difference): the rule's claim checked on every access executed.
  An assertion for the 16-bit step was not written: it could fire only where
  an object touches a bank edge (only the fixture places one), and there it
  would fire on valid code (a walk to one past the end).

## What failed, what is left

- Found on the way, older than the lot and loud: a pointer compared with the
  CONSTANT `obj + size` of an object ending at `$xx:FFFF` stops the link
  (`cmp.w #tail+16`, "out of 16-bit range"; its bank half `#:tail` would be
  wrong too). The fixture computes its end at run time.
- The games do not walk far pointers in their hot code (their hottest
  functions have no staging and no high-half step): the lot is worth a
  quarter of a percent there, as predicted. The bench's other idea from the
  same comparison (first argument in A, deferred popping: `call`, `chain`)
  is where the games' time is.
- The step is refused for a pointer compared with its end
  (`for (; p < end; p++)`): only a counted walk gains it. Widening that needs
  a proof that the end is not the one-past of an object ending at `$FFFF`
  (the linker knows; the compiler does not).
- `devtools/cyclecount/bench.py` reads 1 215 cycles against a baseline of
  1 211 with or without the lot (an earlier lot's drift, not refreshed here).
