# G1 probe: a third colour "lives in X / Y" in `color_slots`

Opened 2026-11-13 by the owner's answer "go for B" (relayed by the conductor): the IR-side probe asked by L3's contest
(change 2, `L3_machine_optimiser.md`) and by G1's contest (taken item 4, `G1_generator_prototype.md`), never made. One
day, Opus alone (no sub-agent, the conductor's instruction). Nothing of it is pushed; it enters `develop` only if the
conductor says so.

## Question

Does an X / Y colour given by the existing slot colouring, on the IR, take most of what a new generator would take on the
hot loops (values, indices, counters in registers)? If yes, the G1 prototype is not needed. If it stops on the X the
emitter takes for itself (the `tax` of every indexed access), that is the evidence that justifies G1.

## What the probe is (written before the code)

- A temp (16 bits: a Kw, or a 32-bit temp whose high half nobody reads), or a web of them joined by a phi or a one-word
  copy, is coloured X or Y instead of a frame slot. Its definition stores with `tax` / `tay`, a read loads with `txa` /
  `tya`; as the index of a plain array (`sym,x`, near) it costs nothing in X and is `sym,y` in Y (abs,y exists for the
  loads, stores and ALU forms; there is no `long,y`, so a FAR or const array indexed by a Y value takes `tyx`).
- Candidates: costliest first (`fn->tmp[].cost`), never live across a call (`cross`), never interfering with another
  candidate of the same register (the colouring's own graph).
- **How it meets the emitter's X**: the function is emitted with the colour, and the emission is the proof. Each IR
  instruction's text is read back; a write of X (or Y) that is not the colour's own, while a value of that colour is live
  after it or read by it, refuses that value; so does any emit path that would read the value's slot (an ALU operand, a
  high half, a phi cycle), a store with an 8-bit accumulator, and a `tax` whose N / Z flags something reads. The function
  is emitted again without the refused values until nothing is refused (the set only shrinks). No splitting: a value is
  in its register for its whole life or not at all.
- The loop rules (`peephole_loop_x`, X and Y passes) do not run on a register the colour uses in that function: they
  assume the register free at the loop's edges.
- `QBE_NO_XY_COLOUR=1` turns it off; with it the output is byte-identical to today's.

## Prediction (before the code)

Leaves: the two hand rewrites (`tech/our_asm_on_real_games.md`) put three quarters of the gain in register allocation:
leaf A (game 2) 23 of 31 cycles a sub-step (values 12, counter 11), leaf B (game 1) 104 of 156 a moving actor (values
44, index in X 32, counter 28). The bench twins of the two hot loops are `glide` (the sprite pass of game 2, its heaviest
common loop) and `soa` (the actors of game 1 in parallel arrays, leaf B's shape).

| | predicted share of the C - hand gap taken | why |
|---|--:|---|
| `glide` (gap 31 134 mclk, SlowROM) | 8 to 11 % | the counter (i x 2) in Y indexes the word arrays as `sym,y`: the `tyx` before each access goes; the second index (`id`, step 4) cannot be X (the easing table's `tax`) and stays in memory |
| `soa` (gap 12 217) | 0 to 10 %, possibly negative | the doubled index in Y saves its reloads, but the counter then loses the register the loop rule gave it (the table read `frame_off` takes X): without splitting, the colour holds one of the hand's two indices |
| leaf B (game 1) | at most 20 % | part of the 32 cycles of the index; nothing of the 44 of values (ALU operands: `adc T`, `cmp T`); the counter is already the loop rule's |
| leaf A (game 2) | at most 5 % | its values are 32-bit accumulators (out of the colour), its counter already in a register |
| gapbench, the twelve routines | weighted mean 1.99 -> 1.95 to 1.99 | |

Games (% of busy time saved, game 1 / game 2): **0.2 / 0.2, range 0 to 0.6 each**. L3's M0 counted 1.1 / 2.2 (middle) for
direct-page slots rewritable while X or Y is free over their span, with the loop rules on. The colour is narrower than
that count on three sides: the emitter's own X (every indexed access whose index is not the coloured value writes X, and
game 2's long-indexed reads of FAR RAM, 10.9 % of its busy time as `lda long,X`, cannot take a Y index); no splitting;
ALU operands refused. And the lesson of lot LF+M5 applies: a count that classifies values for a decision the compiler
takes statically gives a ceiling (`tcc__lf` +4 words: predicted 0.64 / 1.14, measured 0.00 / 0.24).

**Rule for the verdict, written before the measurement.** The probe "takes most" if, on both leaves, it closes at least
half of the gap that register allocation explains (A: 74 % of the gap, B: 67 %), that is **at least 37 % of leaf A's gap
and 33 % of leaf B's** (on the bench twins when the real leaves cannot be rebuilt). Then the prototype is not needed. Under
that, and if the refusals are mostly the emitter's X, the probe is the evidence that justifies G1's allocator (which says
nothing yet of G1's 10 % line, G1-M0). Expected, from the table: **under the line on both; the reason the X of service**.

## Measured (2026-11-13)

What was built, beyond the plan above (each found by reading the probe's own output on gapbench, before any game figure):
the candidates are only webs with an index use or a phi (a value made and read once gains nothing in X and cost a `tax` /
`cmp #0`); a counter in X / Y is compared with `cpx` / `cpy #k` and stepped with `inx` / `dex` (x2 at most); the base
`fuse_indexed` text rule takes `lda.w sym,y` as a source like `sym,x`; a web whose index serves a long access (FAR or const
object: no `long,y`) does not try Y, one that serves `[tcc__r9],y` does not try X (each would copy into the other register
at every access); the registers are given again from scratch after each refusal, costliest web first. A function with a
jump table is coloured too: the table's captured moves count as anyone's write (refuse more, never less).

**Two defects of the first version, found by the gates, not by the measures** (the lesson in the porting kit):
`difftest_stmt.py` failed 59 of 2 000 fresh seeds (910001-912000), reduced to `i = 2; while (i--) {}` leaving 0: the check
"two members of a web interfere" skipped every pair whose union-find root had the higher number, so a counter and its next
value (both live at the test) took X together; and the emission's proof trusted the colouring for the colour's own writes.
Both fixed (every pair checked; an own write of X / Y while another value of that register lives is a refusal); the 2 000
seeds then pass. That first version had passed the gauge of both games (state identical at every step) and the bench's
checksums: they prove the games and the bench, not the compiler.

Builds: `develop` 650a80e (the reference, `g1probe-avant`) against branch `probe-xy`; gauge on the frozen game sources,
state IDENTICAL to the reference at the 341 and 700 steps; the two hand leaves measured on their study copies (C rebuilt
by each SDK, the hand version linked by its switch), the game's state identical in the three builds.

| | predicted | measured |
|---|--:|--:|
| leaf A (game 2), share of the C - hand gap | at most 5 % | **0.0 %** (16 366 830 -> 16 367 410 mclk; hand 12 707 660) |
| leaf B (game 1), routine alone | at most 20 % | **20.3 %** (9 522 620 -> 8 828 590; hand 6 107 176; factor 1.56 -> 1.45) |
| leaf B with its caller | | **20.1 %** (13 966 912 -> 13 214 632; hand 10 232 184; 1.37 -> 1.29) |
| `glide` (twin of game 2's sprite loop) | 8 to 11 % | **16.4 %** (98 814 -> 93 702; 1.46 -> 1.38) |
| `soa` (twin of leaf B) | 0 to 10 % | **10.4 %** (37 082 -> 35 813; 1.49 -> 1.44) |
| gapbench weighted mean / first twelve rows | 1.95 to 1.99 | **1.91 / 2.15** (from 1.99 / 2.20; `state` 1.35 -> 1.21; `oam` +0.24 %, the one row slower) |
| game 1, busy time saved | 0.2 (0 to 0.6) | **1.37 %** (lag frames 172 -> 159; code -1.21 %) |
| game 2, busy time saved | 0.2 (0 to 0.6) | **1.78 %** (lag frames 430 -> 417; code -0.99 %) |

The game figures are seven to nine times the prediction: the prediction leaned on the last count's failure and on the
emitter's X, and underrated two things the colour does that M0 did not count: the `tyx` before each access of a counter
the loop rule had put in Y (the colour indexes `sym,y` directly), and functions with a jump table (game 2's main loop -3.9 %,
its object dispatcher -0.8 %: only once the tables were allowed; without them game 2 was -1.05 %). Functions slower: game 1's
sprite update +2.7 %, a sprite routine of game 2 +1.1 % (not studied); and three of the measured workloads
(`devtools/sdkbench`, limit +2 %): `collide` +4.1 %, `bytes` +5.1 %, `tilemap` +7.5 %. Read on `tilemap`: the inner
counter (the hot one) is an operand of `eor` / `ora`, so the colour refuses it, while the text loop rule had kept it in Y
(it rewrites a commutative operand, `pp_alu_of_slot`); the colour then gives Y to the outer counter, and the loop rule,
which the colour switches off for the whole function on a register it uses, no longer keeps the inner one: the inner loop
goes back to memory. The colour's cost model does not see what the text rules would have done with the register, and a
register is held for a whole function, not a region.

**How the probe meets the emitter's X, and what stops it.** Weighted by the colouring's cost (use count x loop depth), the
candidates of both games' C end: coloured 60.3 % / 78.6 % (game 1 / 2); refused because the emitter writes the register
while the value lives (the `tax` of an indexed access whose index is another value, a call, a table's dispatch) 30.7 / 14.1
% alone and 7.2 / 5.1 % with a slot path too; refused only for a slot path (an ALU operand, a high half) 1.6 / 2.2 %. So among
the values the colour can hold (indices, counters), the X of service is nearly the whole refusal, and most of them are taken.
What the colour cannot hold is the larger part: **a value that is an operand of `adc`, `sbc`, `cmp`, `and` lives in A or in
memory on the 65816, never in X or Y**, and those are not even candidates. That is the hand's "values kept in a register"
(12 of leaf A's 31 cycles a sub-step, 44 of leaf B's 156), its signed-compare and data-bank idioms, and leaf A's 32-bit
accumulators. Leaf A shows it whole: its only index already sat in X through the text rules, nothing else qualified.

**Verdict, by the rule written before the code: the probe does not take the essential** (leaf A 0 % against 37 %, leaf B
20 % against 33 %). It stops on two walls: the emitter's X of service, for the indices it does try (a third to a fifth of
the candidates' weight); and, larger, the absence of any allocation of A, which no X / Y colour can supply because A is
the emitter's scratch for every operation. Both are what an allocator that chooses A, X, Y and the direct page together
with instruction selection would address: this is the evidence that justifies G1's prototype as an experiment. It says
nothing new of G1's 10 % line (G1-M0: out of reach on the leaves alone), except that the colour's 1.4 / 1.8 % now belong
to the baseline any prototype is measured against.

**The probe as a lot** (decision for the conductor): it gains 1.37 / 1.78 % of busy time and 0.08 on the weighted bench,
with the state identical, and its proof is executable (the emission refuses what its text does not prove; `QBE_XY_FAULT=1`
injects a `tax` too many that the proof is made to trust: `difftest_stmt` fails 41 seeds of 100; `QBE_XY_FAULT=2`, the same
`tax` undeclared: the proof refuses the value, 100 / 100 clean). Before `develop` it needs: the three slower workloads fixed (the loop
rule switched off only where a coloured value lives, or the colour weighed against it), the 17 compiler checks that pin
the old shapes moved or given `QBE_NO_XY_COLOUR=1`; the two example baselines that shift by one frame re-captured with
their reason (`basics/collision_demo`, `sprites/sprite_swarm`: `diff_corpus` SAME-SEQUENCE at offset -1, the code is
faster); a hunt of 20 000 seeds of each differential test (the rule for a new rule; 2 000 were run here); its paragraph in
`compiler.md`; and a reading of the two slower functions. Cost: about 900 lines in `emit.c`, compile time about +5 % on the
two largest files of game 2 (a function is emitted again after each refusal).

Gates of the probe (branch `probe-xy`): `QBE_NO_XY_COLOUR=1` gives a byte-identical kernel file and 150 / 150 compiler checks;
with the colour, 133 / 150 (the 17 shape pins above; no value check fails). Final run, branch head: `difftest.py` and
`difftest_stmt.py` 2 000 fresh seeds each (950001-952000) clean; the 2 000 seeds that caught the defect (910001-912000)
clean; `diff_corpus` 63 / 65 MATCH, the two DIFFs SAME-SEQUENCE at offset -1; `make tests` steps: liveness 65 / 65 (both
power-ons), compare 63 / 65 (the same two), manifests 175 / 176 (`movement_collision_demo`: every value passes, its frame
hash is the same one-frame shift), audio 4 / 9 hashes, of the five `luna diff --audio --align-onset` gives MATCH on four
(max delta 0.27 %) and DIFF on `chips/superfx_game_skeleton` (2.46 % in its last window, shift 6 samples, first sound at
the same sample: hypothesis, a game event a few cycles earlier; not listened to), WRAM oracle 15 / 65 (the streams move
with the code, not re-captured on a measurement branch); fixtures, ROM coverage, race gate, NMI budget, VRAM DMA, link of
the modules, bank spill, the user project, torture subset, `bench.py`, `libbench` all pass; `sdkbench` and `gapbench
--check` fail on the slower rows above. Nothing re-baselined.

## Lot XY (2026-11-14): the probe made a lot, prediction written before the code

Asked by the conductor and the owner (L4 frozen at S3): the colour as a mergeable lot. Pair: Opus alone, as for the probe.

**The three slower workloads.** Cause read on `tilemap` (above): the colour refuses the hot inner counter (an ALU operand),
gives Y to a colder value, and switches the loop rule off for the whole function on that register. Two changes, in this
order: (a) the loop rule is switched off only over the text where a coloured value of that register lives (the emission
already knows, chunk by chunk, which coloured values are live: the byte ranges of those chunks are passed to the rule,
which refuses a loop whose lines fall in one); (b) a function is emitted with and without the colour and the cheaper text
is kept, by a static cost (cycles of each line from the 65816 timing table, weighted by its block's loop weight), so that
the colour never loses to the text rules it displaces. (b) alone is enough for "never worse" by the estimator; (a) keeps
the loop rule's gain where both can coexist.
Prediction: `collide`, `bytes`, `tilemap` back within +-0.5 % of their baselines (each the colour's loss undone; any gain
left is (a)'s); the rest of `sdkbench` and `libbench` unchanged or better; gapbench weighted mean 1.91 (unchanged:
its rows gained already), `oam` back to its baseline.

**Net gain on the games** (busy time, today -1.37 / -1.78 %): the regressions of the probe in the hot functions of game 1
(its sprite update +2.7 %, 8.6 % of busy time: 0.23 points) and game 2 (a sprite routine +1.1 %, 4.8 %: 0.05 points) are
of the same family if (b) catches them (hypothesis): **game 1 -1.4 to -1.6 %, game 2 -1.8 to -1.9 %**. State identical
at every step. What could make it less: (b)'s estimator choosing the plain text where the colour was in fact faster
(it counts cycles, not the frequency a run gives); that would show as a game figure under -1.37 / -1.78.

**Compile time** (+5 % on the two largest files of game 2 for the probe): (b) adds one emission and one text pass per
function the colour touches. Target: no more than +5 % after (b), by cutting the attempts (a refused web is not tried
again in a register it was refused, already; the plain text emitted once and reused as the fallback).

**Gates.** The 17 compiler checks that pin the old shapes: each moved to the new shape where the check's subject is still
seen, or given `QBE_NO_XY_COLOUR=1` where it pins a mechanism the colour replaces (the loop rule's own shape); no value
check touched. The two examples one frame early re-captured after a frame-by-frame comparison (same pictures, offset 1);
`superfx_game_skeleton`'s audio compared and explained before its hash moves. A hunt of 20 000 seeds of each differential
test (30001-50000), the injected fault still caught, and an independent check of the lesson: an assertion build
(`QBE_XY_ASSERT=1`) that keeps every coloured value's slot as a shadow and compares the register with it at every use on
the emitted text (`brk` on a difference), run under `difftest_stmt`: a proof that does not rest on the interference graph.

### Lot XY measured (2026-11-14), against the prediction

Reference: the same tree built with `QBE_NO_XY_COLOUR=1` (`develop` fc63dc0a with the colour switched off; its gauge equals
the probe's reference to the cycle, and its kernel text equals `develop`'s but for the L4 side table's comments).

| | predicted | measured |
|---|--:|--:|
| `collide`, `bytes`, `tilemap` (sdkbench, limit +2 %) | within +-0.5 % | **-0.20 / -2.06 / 0.00 %** (from +4.1 / +5.1 / +7.5 %) |
| sdkbench, every workload | unchanged or better | **none slower** (`calls`, `copy`, `dist` +0.00 / +0.01 / +0.00 %; total -2.28 %; `crc` -6.2, `depot` -3.8, `farfill` -3.8 %) |
| libbench | unchanged | 23 / 23 within 10 % (`oambatch` +4 mclk) |
| gapbench weighted mean / first twelve rows | 1.91 | **1.91 / 2.15**; `oam` 1.87 -> 1.81, `state` 1.35 -> 1.21, no row slower than its baseline (`probe` gives back the probe's gain: 1.42 -> 1.40 against 1.35) |
| game 1, busy time saved | -1.4 to -1.6 % | **-1.72 %** (lag frames 172 -> 156, code -1.38 %) |
| game 2, busy time saved | -1.8 to -1.9 % | **-1.69 %** (lag frames 430 -> 417, code -1.04 %) |
| leaf A / leaf B / leaf B with its caller, share of the gap to the hand | (probe: 0 / 20 / 20 %) | **0.0 / 20.3 / 22.8 %** |
| compile time, the two largest C files of game 2 | +5 % at most | **+2.5 %** (best of five, user + system) |

State identical to the reference at every step of both games (341 and 700), and in the three builds of each leaf.

Game 2 lands under the prediction and under the probe's -1.78: the rules that keep the loop rule's registers (a web beside
a rule counter left in memory does not take its register; a lighter one beside a refused rule counter takes none) and the
`if`-merge rule give back some of the probe's colours there (not separated by a measurement). Game 1 lands above: its
sprite update went from +2.7 % to -1.1 %, the function that the probe's `if`-merge phi made slower (an absolute value in X
that evicted the index the text rules kept there) from +5.7 % to +1.7 %.

**How it was reached, and what was wrong on the way** (each found by a gate or a measure, not by reading):
- the three slower workloads had two causes: the colour switched the loop rule off for the whole function on a register
  it used (`tilemap`), and two counters joined into one web through a phi argument they shared (`i + 1` feeding `i` and
  `j = i + 1` in `collide`), which kept the inner one out of every register. Fixed by the region-aware loop rule, the
  joins that refuse interference, and the loop rule's counters learned first. A static cost estimator that chose between
  the coloured and the plain text was tried and dropped: weighting blocks by loop nesting counts a rare `hits++` as often
  as the loop's own step, and a frequency estimate from the CFG mis-weighted the blocks after a loop; it made `box` and
  `slots` lose their gain;
- the region ranges were merged as if they came in text order; they come from the last block to the first, so the merge
  kept one range and the loop rule converted a loop to Y where a coloured `j` lived: `difftest_stmt` seed 961160 (1 of
  200). `QBE_XY_FAULT=3` reproduces it on purpose;
- the probe's compile time "+5 %" compared the compiler with itself: the script passed `QBE_NO_XY_COLOUR=` empty, which
  switches the colour off as well (the switch tests presence). Measured right: +116 %; the plain emission's full text pass
  was the cost (clock per phase), it now stops after the loop rule;
- the assertion build itself had three defects before it could be trusted, each a false alarm found on its first 100
  seeds (8 stops): no shadow store for a phi whose argument already sat in the register, nor for a copy left out of the
  text, and an anonymous `+` label in its check that captured the `bxx +` of a branch whose moves it sat among (now a named
  label). After them: 100 / 100 seeds clean, and with `QBE_XY_FAULT=1` (a `tax` too many the proof trusts) 8 of 20 seeds
  stop on `stp`.

**Gates of the lot** (branch `probe-xy`, qbe 1888e97): `make clean && make` no new warning; `make test-compiler` 152 / 152
(with `QBE_NO_XY_COLOUR=1` everything but `xy_colour` passes: the 17 adapted checks pin the old shapes, with the switch);
`make tests` ALL CHECKS PASSED after the re-captures below; `diff_corpus` 63 / 65 MATCH, the two DIFFs the same pictures
a frame earlier (`collision_demo` 24 of 26, `sprite_swarm` 394 of 397, offset -1), and every C global of both equal one
frame earlier but the frame counters (the build has one lag frame fewer at boot): their compare baselines and the frame
hash of `movement_collision_demo` re-captured (every value of that manifest unchanged); the WRAM streams 50 of 65
(journal line); audio: `luna diff --audio --align-onset` before / after, four MATCH (max 0.01 %), `superfx_game_skeleton`
DIFF 2.46 % in its last window with a 6-sample shift, the first sound at the same sample, its WRAM identical outside the
stack at frames 100 and 299: the music commands reach the APU at other cycles within the same frames (measured, not
listened to; the author's ear decides). Hunts of 20 000 seeds (30001-50000): `difftest.py` 20 000 clean;
`difftest_stmt.py` 19 998 clean and 2 compiler stops (an assertion in `bsset`: a near dereference's stale base and index
entries, fixed in qbe 1888e97; the two seeds then clean, the 65 ROMs and both games' ROMs byte-identical before and
after the fix). Provoked faults: `QBE_XY_FAULT=1` 50 of 100 seeds red, `QBE_XY_FAULT=2` 100 / 100 clean (the proof
refuses), `QBE_XY_FAULT=3` seed 961160 red; the assertion build 300 / 300 clean (50001-50300), and with `QBE_XY_FAULT=1`
8 of 20 seeds stop on `stp`.
