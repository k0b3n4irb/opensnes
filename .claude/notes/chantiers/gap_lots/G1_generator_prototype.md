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
   looked at (the other 65816 compiler's measure, private, is the first: a private folder outside the repository).
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

## Revision 2 after the retrospective (2026-11-14): the step plan to be contested before any code

Sources: `reviews/2026-11-14_retrospective_generator.md` (the three readings and the eight neighbours' answers) and the owner's direction
of the same day (one comparative bench for every solution, the best of each world technique by technique, the target is the SNES, do not
scatter; he asked to be challenged). Nothing of G1 enters `develop`; this runs in a worktree started from `develop`.

### What the retrospective changes

1. **The unit of cost is the master cycle of an access, by region** (6 / 8 / 12; code size is time: a byte of instruction is a read).
   The bench counts master cycles per window, slow and fast ROM apart. Instruction counts alone are not accepted as a figure.
2. **The bench takes several natural formulations of the same C** (structures, one array per field, pointers, grid): formulation moves
   the cost by 1.33 to 1.58, more than most advances (1 to 2 %). Otherwise G1 fits our workarounds.
3. **The arbiter is the rebuilt game replayed at each stop**, not a snapshot nor twelve routines. The gauge's own gap (predicted -1.72 %,
   measured -0.08 % on one game, cause open) is settled first or its limit written in every figure.
4. **Correctness before gain**: the invariants of the games are tests that must go red under an injected fault: order and number of
   console register accesses; no growth or move of the VBlank path; layout of an array the DMA sends as it is; bank and convention of a
   function called from assembly or the NMI; no datum above `$1FFF` read as bank `$00`; no inlining or renaming of a probe entry.
5. **A technique table first, a generator second.** Each technique reproduced by hand or by a small pass and measured alone: direct page on
   the entity (measured by hand by a port: 5 080 against 5 217, -2.6 %, NMI contract confirmed; two conditions: structure in bank 0 below
   `$2000`, no C direct-page variable accessed while D is moved), far pointer as `[dp]` with a 16-bit step, first argument in A, hardware
   multiplier as a typed operation with its 8 / 16 cycle slack, `mvn` stub, signed 16 by unsigned 16 to 32 multiply, word composition
   of neighbouring byte writes in FAR, stable inlining that reports a flip. Techniques of other compilers enter the same way (their
   documents captured privately with the licence written; their binaries run only where the licence allows a private bench).
6. **A gain is confirmed on a cartridge before it is declared**: the emulator is a faithful model, not a console capture.

### Steps (each the size of one sub-agent, a prediction written before, a stop rule, the lesson to the kit)

- **R0, the bench and its truth (2 to 3 days)**: one harness: the twelve gapbench rows plus the neighbours' hot shapes in several
  formulations, in master cycles by window on both ROM speeds, for today's generator, the other 65816 compiler at the same state, the
  hand yardstick; equivalence proven at every row; the gauge gap explained or bounded. Prediction: the table reproduces 2.20 / 2.56 / 1.0
  to within 3 %. Stop: if it does not reproduce, nothing after R0 is read.
- **R1, the technique table (3 to 5 days)**: each technique of point 5 alone, on the rows that name it; the table "technique x measured
  gain x who does it" with a column "ours already". Prediction per technique before the run; expected: no technique alone above 3 % of a
  game's busy time except the call boundary. Stop: a technique under its prediction by half is dropped from G1's scope with its figure.
- **R2, the invariants (1 to 2 days, parallel to R1)**: the six invariants as fixtures with injected faults, running against today's
  generator first (they must pass) so that any generator is judged by them. The mutation score of the equivalence harness (50 to 100
  one-instruction faults, 90 % caught) is part of R2.
- **R3, the prototype on the hot loops of the leaves (the first stop of the owner's rule)**: allocation of A, X, Y and direct-page
  slots as one bank, selection by master-cycle cost, the SDK ABI at the boundary, built from the winning techniques of R1; within 1.1 of the
  hand on the two leaves or a written reason. Stop: not within 1.2, the prototype stops and says so.
- **R4, 6 to 8 functions drawn by profile weight (the second stop)**: the functions that call, the games rebuilt and replayed; the line
  is 10 % of game 2's busy time; the figure goes to the conductor with the prediction written before.
- **R5, closure**: the table, the lessons, a cartridge confirmation of the claimed gains, and the decision that belongs to the owner.

### Cost, honest range

R0 to R2: 6 to 10 days of the pair; R3: 3 to 4; R4: 2 to 3; total 11 to 17 days (the earlier 7 to 11 did not include the bench of formulations,
the technique table nor the invariants; hypothesis until R0 is measured). Fable is called only to contest this plan, to review one
diff before a whole-corpus change, and on one named diagnosis, one task at a time.

### Contest of revision 2 (the strongest model, 2026-11-14, one task: where the revised step plan is wrong, costs more or cannot be tested), checked, and what is taken

Instruction given (journal, rule of 2026-11-14): read-only; the question above; pieces: this file, the retrospective, `G1_ir_probe.md` (Measured, Two defects), `L4_s3_result.md` (Result); out of field: redesign, inlining, the owner's authority, style; form: at most ten objections with piece, consequence and fix, two short paragraphs (the bench direction, R0 to R2 before code). Checked by me against the pieces before keeping (2.15 in `G1_ir_probe.md` l.89 and 190; "S4 is therefore what makes the static frames pay" in `L4_s3_result.md` l.68; `tilemap` +7.5 % l.98); the objection on the lost cost targets is half wrong (points 5 to 10 of the 2026-11-13 contest still stand, this revision only did not repeat them: they are in force).

Taken, in this order:
1. **The 10 % line (R4) is predicted lost by our own sources**: since the X/Y colour belongs to the baseline, the generator that keeps today's ABI at the boundary adds 1 to 2 %. The 2026-11-13 contest's point 2 (the scope contradiction) was asked of the conductor and is unanswered. **Before R3: the prediction of condition 2 is rewritten on the post-X/Y baseline, and the question "free convention inside one file, yes or no" goes to the conductor** (it is L4's territory: a "no" lowers the line, a "yes" widens the lot).
2. **R1 does not measure alone what pays only together** (L4: correct and slower until S4; the colour made `tilemap` +7.5 % by cutting a loop rule). No technique is dropped on an isolated measure: it is dropped only after a combined measure with the technique it depends on, named in advance. Each technique is written "isolated pass" or "exists only inside an allocator" (direct page on the entity moves D, the register bank itself; `[dp]` and the first argument in A are allocation decisions; the multiplier's slack is scheduling); the second kind is costed inside R3.
3. **The starting debt is a step of its own**: in R0, the leaves and the R4 sample are measured with the old text rules cut (`QBE_NO_*`), and R3 is re-estimated from it; otherwise the first stop measures the loss of the rules, not the allocator.
4. **The gauge's gap (-1.72 predicted, -0.08 measured) is a step before R0** with a cost (1 to 2 days) and a stop: if the cause stays open, R4 is read on the rebuilt game with an error bar written (about 1.6 points), and its 9.0 to 10.7 % margin is read against it.
5. **R0's prediction is wrong by construction**: the first twelve rows read 2.15 today (not 2.20), and the other compiler's 2.56 is a static cost weighted by block counters, not master cycles per window. R0 replays the old method at the frozen commit, then publishes the new unit as its own base, with no inherited target.
6. **"Within 1.1 of the hand on the two leaves"**: the 2026-11-13 targets stand (master cycles per perimeter, leaf B defined, two lines per leaf, held-out set, frozen reference commit); leaf B is no longer a leaf once inlined, leaf A is at 0.0 % from the probe (the gap is the allocation of A and the 32-bit accumulators): both written again in R0.
7. **The differential proof is a gate of R3 and R4**, not only R2's mutation score: 20 000 seeds through the prototype with the fallback rate, the paranoid build, the `jsl` / `rtl` contract on the luna trace; their cost is in the steps.
8. **R1 re-measures what is measured**: the column "already measured" is filled first from the notes (first argument in A 0.90 / 0.97 %, far pointer in the direct page 0.25 %, `mvn` and the multiplier delivered); only the unknown is measured, on the rebuilt game (a bench gain does not transfer: -2.26 against -0.08 %).
9. **R0 is narrower**: the twelve rows and two neighbour forms (the entity loop, the byte table), not the whole matrix; the other forms after, one at a time.
10. **R5**: the cartridge confirmation has a defined observable (late frames on a fixed scenario) and a tolerance, or the gains are declared "model" and say so.

On the owner's direction (one bench for all, the best of each world): its cost is the matrix times each third-party compiler (licences, emulation, different measuring methods); its biggest risk is that gains are not additive (L4, `tilemap`), and "the better of both per row = 1.99" is a calculation, not a generator. **The narrow first step: the twelve rows, two compilers, one unit, to settle whether 0.34 on `unpack` and 0.41 on `call` hold in master cycles.** R0 and R2 before code: yes; R1 as written hides the generator's cost: it is split as item 2 says.

### The plan after the contest

- **R-1 (1 to 2 days)**: the gauge's gap, with its stop (error bar if the cause stays open).
- **R0 (2 to 3 days)**: the twelve rows and two forms, three generators (today's, the other compiler, the hand), both ROM speeds, master cycles per window, the old method replayed at the frozen commit, the starting debt (old rules cut). Prediction: the new base reads 2.15 on the first twelve rows within 3 % of the old method re-run; the debt is written as a number.
- **R1 (3 to 5 days)**: the table; the "already measured" column first; isolated passes measured alone, allocator-only techniques only listed and costed for R3; no drop without a combined measure.
- **R2 (1 to 2 days)**: the six invariants and the mutation score, against today's generator first.
- **Conductor's answer on the free convention** (before R3): decides the line of condition 2.
- **R3 (4 to 6 days)**, **R4 (2 to 3 days)**: as in revision 2, with the differential gates (20 000 seeds, paranoid build, `jsl` / `rtl` contract) and the rewritten prediction.
- **R5**: closure, the observable and its tolerance, the owner's decision.
Total 13 to 19 days of the pair (hypothesis until R-1 and R0 are measured; the contest adds R-1, the debt and the gates).

### Conductor's answer on the free convention (2026-11-14)

Yes: the free convention between `static` functions of one file is in the scope, as an option of the prototype measured apart, and the line stays at 10 %. Conditions: **two columns everywhere** (every figure of R3 and R4 is given twice, "ABI kept" and "free convention"); **the public boundary does not move** (a function visible from another file, or called from assembly or the NMI, keeps the ABI: the games' invariant, tested under an injected fault); **L4's finished work is reused**, not redone; two additions enter the plan: an **integration report compared from one build to the next**, signalling when an inlining decision flips, and the library's share of hand assembly noted as a library lever outside G1. The owner is asked only if R3 or R4 shows the line out of reach even with the free convention. Order confirmed: I1 S4, then R-1 (the gauge's gap), then R0.

### R-1 closed (2026-11-15, 00:00)

The gauge's gap was a reference that was not the one named: `xylot-ref` was the compiler with the X/Y colour off (`QBE_NO_XY_COLOUR=1`); the parent of the lot already coloured. The lot's own part is -0.35 % on game 1, the whole colour -1.65 % (the gauge's -1.72 % was the whole colour). No error bar is needed from this cause; **the rule that remains**: every figure names its reference by commit and by switches. R-1 costs 0 days: the plan starts at R0 (with that rule in its harness); total 11 to 17 days.

### Plan update (2026-11-15, conductor's go) and R0, predicted before any measure

Updated: R-1 is closed (above). The case "an inlining decision was lost since 2026-11-04, about 3 %" is **withdrawn** (a game's own correction: never inlined since its loop was written, the room rule refuses it, forcing it gains 0.04 %): it supports nothing in this plan; the integration report stays a practice without a proof case.

**R0 (2 to 3 days; a pair on a worktree branch `g1-r0`; nothing enters `develop`)**: one bench, in master cycles per window on both ROM speeds, naming every reference by commit and switches (the lesson of R-1).
1. **Establish the method that gave 2.56 for the other 65816 compiler** (a measure of its built ROM on the emulator, or a static weighted cost: read its script), re-run it at the present state, and never put two units in one column.
2. **The twelve rows** of `gapbench` and **two neighbour forms written neutral**: the entity loop in three formulations of the same C (an array of structures, one array per word field, a pointer walk) with its hand yardstick, and the byte table (four bytes written to four arrays from an index) with its hand yardstick. Equivalence by checksum, as the existing rows.
3. **Three generators**: today's (`develop` `3139a9ef`), the other 65816 compiler at the same state (private, script in the private folder), the hand.
4. **The starting debt**: the same rows with the generator's switches off one family at a time (`QBE_NO_PEEP*`, `QBE_NO_X_CACHE`, `QBE_NO_SLOT_COLOR`, `QBE_NO_XY_COLOUR`, `QBE_NO_DP_*`, ...) and all at once, the weighted mean of the twelve rows for each: what today's generator owes to its rules, and what a new allocator would have to earn back first.

Predictions (hypotheses, written before the run): (a) the new base reads **2.15 within 3 %** on the first twelve rows (the probe's figure, `G1_ir_probe.md`); (b) the other compiler's figure in the same unit is **within 10 % of 2.56**, the sign of its six better rows unchanged (`unpack`, `call`); (c) the structure form and the field-array form of the entity loop differ by **1.3 to 1.6** (a port measured 1.33 and 1.61 at N = 20); (d) switching off every text rule at once raises the twelve-row mean from 2.15 to **2.6 to 3.4**, and no single family accounts for more than a third of the rise. Stop: if (a) is missed by more than 5 % nothing after item 2 is read until the cause is named.
