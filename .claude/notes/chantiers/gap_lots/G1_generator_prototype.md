# G1 — a bounded, throwaway prototype of a code generator designed for the 65816

Opened 2026-11-13 by the owner's answer "A" (the conductor's decision log, same day), after the short lot and alongside L4.
Written before any code: cost, rule, plan. Nothing of the prototype enters `develop`; the current generator stays the yardstick.

## Question
Do the compiler's retouch axes (all counted, 6 to 9 % at most for the games) leave a gap that a generator designed for the
65816 closes? "Designed" means: allocation of A, X, Y and direct-page slots as one register bank, static frames and the calling
convention decided at the start, instruction widths chosen by real cost.

## What is measured, not guessed
Four perimeters rewritten by hand with equivalence proven at every game step (`tech/our_asm_on_real_games.md`): two leaves (1.35
and 1.75 over today's C) and two functions that call (1.23 and 1.66 with their callees and a free convention; 1.10 to 1.13 body
alone under today's ABI). The perimeters, the traces and the proofs exist; the prototype reuses them as they are.

## Engagement rule (written before the count, as given by the conductor)
The prototype commits the project to a generator only if, on the same traces and with equivalence proven, **both**:
1. it is within **1.1 of the hand** on the two leaves; and
2. it saves **at least 10 % of game 2's busy time** over the current generator.
Otherwise: retouch the current one (L3/L4 lots) and stop. The figures go to the conductor; any commitment beyond is the owner's.

## Scope (what the prototype is, and is not)
- In: a stand-alone pass that takes the QBE IR of ONE function after the existing optimisations and emits 65816 text for it with
  its own allocator (graph colouring or linear scan over a bank of A, X, Y and direct-page words, chosen by measurement on the
  two leaves first), own instruction selection by width, and the SDK's ABI at the function boundary (so it links in place of the
  function the current generator would emit). Functions it cannot handle fall back to the current generator, and the fallback
  count is reported.
- Out: a new frontend, a new IR, the library, the assembler, any change to `develop`. Static frames are L4's; the prototype
  consumes L4's convention when it lands, it does not duplicate it (hypothesis: the two overlap on the same slots).

## Cost (estimate, hypothesis until the first step is measured)
A pair Opus / Opus, in days of the pair: (1) harness, equivalence proof on the four perimeters, the two hand versions as targets:
1 to 2; (2) allocator and selection on the two leaves, within 1.1 or a written reason: 3 to 4; (3) the two functions that call,
with the ABI at the boundary: 2 to 3; (4) the traces of both games, the whole-program count against the 10 % line: 1 to 2.
Total **7 to 11 days**, with a stop after (2): if the leaves are not within 1.2 after step 2, the prototype stops there and says
so (an early verdict costs 4 to 6 days, not 11).

## Order and gates
1. Contest this plan (Fable, written, before step 1): what is missing, which cost is underestimated, which yardstick is not
   looked at (the other 65816 compiler's measure, private, is the first: `dumps/…/vbcc`).
2. Step 1 harness; step 2 stop/go on the leaves; step 3; step 4 verdict to the conductor with the table.
3. Each step: state identical, the prediction written before, the lesson in the kit.

## Open
- Whether the allocator should work on the IR before or after the existing peephole rules (hypothesis: before; the rules become
  its cleanup).
- How the prototype and L4's static frames share the direct page (to settle once L4's window is measured).

## Contest (Fable, 2026-11-13, written) and what is taken

Verdict of the contest: do not start step 1 as written. Taken, in order:
1. **G1-M0 first (one day, before any code)**: the share of game 2's busy time by function after inlining, leaves against
   functions that call, and the prediction of condition 2 written before the code. Reason (calculation on the notes' figures,
   not a measurement): 10 % of busy time at a factor of 1.35 on the leaves needs about 41 % of the C's time covered; at 1.2 it
   needs 64 %; the hot time of game 2 is in functions that call, where the ABI keeps values on the stack (`ABI.md`: A, X, Y,
   `tcc__r*`, `tcc__lf` are all the callee's to clobber). If the reachable share stays under 10 %, G1 does not open as scoped.
2. **The scope contradiction**: "designed for the 65816" includes the convention and static frames, and the scope leaves both out.
   Either the scope takes the free convention between `static` functions of one file, or the question is rewritten as
   "allocation and selection inside the function" and condition 2 is lowered, with the conductor's agreement. Asked of the
   conductor.
3. **The other 65816 compiler measured first** on the two leaves (compile to assembly, static cost weighted by the traces' block
   counters; private; way 2 is under way): if it is at 1.4 on leaf B, the bar of 1.1 is past the state of the art.
4. **An IR-side probe, one day, as the comparison arm**: a third colour "lives in X/Y" in `color_slots`, asked by L3's contest and
   postponed. If it takes most of what the new generator would, the generator is not needed.
5. **The rule, refined without replacing it** (the headline stays: within 1.1 of the hand on the two leaves AND 10 % of game 2's
   busy time): targets in master cycles per perimeter with leaf B defined (routine alone 1.75 or with its caller 1.47); two
   lines per leaf, "allocation and selection only" and "with idioms" (52 of B's 156 cycles are range test, data bank and
   `cond ? k : 0`, which any generator can take); a held-out set (`gapbench` rows); a frozen reference commit; the comparison
   also to the ceiling of the retouch axes; game 1 no worse and code size; the coverage and extrapolation rule written before;
   what is kept of an intermediate result.
6. **Stops moved**: after G1-M0; after the hot loops of the two leaves alone; after a sample of 6 to 8 functions of game 2 drawn
   by profile weight, outside the four perimeters. The whole-program count is dropped.
7. **Cost re-estimated**: 15 to 25 days as written, 8 to 12 with the cuts (no 8-bit width, no step 3 under the pure ABI, a sample
   instead of the whole program); the starting debt measured by switching the existing analyses off (`QBE_NO_*`) on the leaves.
8. **Proof as a condition of any figure**: a mutation score of the equivalence harness (50 to 100 one-instruction faults injected
   in the output; under 90 % detected, the proof is not enough for a generator); a paranoid build (each definition also stored,
   each use compared) with poisoning of registers declared dead; both differential tests through the prototype, 20 000 seeds
   each, with the fallback rate; a width-flow check on the emitted text; the boundary contract checked on the luna trace at
   every `jsl` / `rtl` (stack balanced, D and the data bank unchanged); the NMI handler's data-bank assumption read in
   `crt0.asm` before the prototype sets the bank.
9. **Open 1 settled**: allocate on the IR before the rules; the old text rules are cut except branch shortening, and the count
   of lines they would change on the prototype's output is the control (expected zero).
10. **Open 2 settled**: L4 ships now its link-time half (annex table, placement, overlap check, window); its emitter half waits
    for G1's first stop; a static slot becomes a colour class of the allocator.

Recorded, not taken: the pair stays Opus / Opus by the owner's rule of 2026-11-12 (Fable only for contest and the review before
close).

## G1-M0 and the other compiler (2026-11-13), the two figures asked before step 2

**G1-M0** (private traces of the gauge, reproduced by a script; game 1 / game 2, % of busy time): leaves (no call executed or
emitted) 21.4 / 18.6; functions that call 58.9 / 74.4; not C (library assembly, NMI) 19.7 / 7.0. Half of the busy time is in 5
functions (game 2) and 7 (game 1); 90 % in 27 and 23. Functions that call and hold a value across a call (a stack frame): 57.0 /
67.7, of which the `n,S` accesses alone 8.7 / 17.5. Leaf B of the hand rewrite is no longer a leaf once the compiler inlines it
(it sits in the hottest function of game 1); only leaf A remains one.
Prediction of condition 2 on game 2, written before the prototype (gain = class share x (1 - 1 / factor)):
leaves alone at 1.2 / 1.35: 3.1 / 4.8 % — **out of reach** (it would need 2.17 on every leaf); leaves plus functions that call at
1.10: 9.9 / 11.6 %, at 1.13: 11.7 / 13.4 %. Correction (hypothesis, from the notes): the 1.13 was measured before the jump table,
which was most of it, so the remaining body factor is about 1.086 and the range falls to **9.0 to 10.7 %**: the line is reached
only if the prototype also wins about 1.09 to 1.10 on the functions that call, under today's ABI. With a free convention inside a
file (1.23) the estimate is about 18.7 %: that is L4's territory, outside the narrow scope.

**The other 65816 compiler** (vbcc65816 Release 2 under emulation, private; its own Dhrystone binary reproduced byte for byte):
geometric mean over the twelve bench routines 2.56 times the hand against 2.20 for ours (20 routines: 2.60 against 2.40). It is
not a target: the hand stays the yardstick. It is clearly better on six routines (`unpack` 0.34 of ours, `call` 0.41 or 0.71
without inlining, `chain` 0.68, `slots` 0.66, `fixed` 0.80, `ents` 0.94); taking the better of both on each row would give 1.99
instead of 2.20 (a calculation on the table). Ideas ours lacks, from reading both assemblies (hypotheses until coded): a far
pointer kept in a direct-page slot and used as `[dp]` with a 16-bit increment (`unpack`), the first argument in A and the
arguments popped once after several calls, small same-file functions inlined, a 32-bit `>> 8` as a byte move, constant-count
loops unrolled. Not to copy: its unrolling without the index in X.

**What this says about G1**: the narrow scope cannot reach the 10 % line by allocation inside leaves; it can only if the
functions that call also gain, and most of what they lose is the stack traffic that L4 and the call convention address. So the
cheap order is: L4's counting step (S0) and the IR-side probe first (about two days together), the vbcc ideas as short lots
(they are our own backend's), and the prototype's remaining cost is 7 to 11 days of the pair after the probe (hypothesis; the
figure was 8 to 12 before the probe and the contest's cuts), with its first stop at the hot loops of the leaves.
