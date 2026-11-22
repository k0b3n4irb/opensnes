# L3: the machine-level optimiser

Lot of `../c_asm_gap_plan.md` (items 1 and 5, the small items of
temporaries). Written before the lot starts: the design, cut into steps, each
with its proof obligation, its predicted gain and its cost. Contested in
writing before the first step (section "Contested", below). The measured
column of each step is filled when it ends. Draft of 2026-11-12.

## The axis

Register allocation is three quarters of what a hand gains over our C on two
real leaves (`tech/our_asm_on_real_games.md`: values kept in a register 12 and
44 cycles, the index held in X 32, the counter in a register 11 and 28, of 31
and 156 a turn). The backend has no allocator: every SSA value has a memory
slot, A is a cache, X and Y are taken by text rules in the shapes they know.
Those rules are now about twenty passes over the emitted text
(`relax_branches` in `compiler/qbe/w65816/emit.c`), each with its own reading of
labels, branches, widths and liveness (`pp_nz_dead_after`, `pp_slot_live_at`,
`pp_a_is_slot_before`, `pp_writes_x`...), and each new one is checked against
every other by hand ("two rules right and wrong together", `compiler.md`).

The lot gives the text one reading: instructions with what each reads and
writes, a control-flow graph, liveness of every register, flag and slot; then
uses it to keep values in X and Y over regions, and moves the existing rules
onto it.

Target of the plan: the two rewritten leaves within 1.1 of the hand (today 1.35
and 1.75), bench weighted mean under 1.6 (today 1.99).

## What stays as it is

- The IR, the middle end, instruction selection, the emitter's output. L3 reads
  what the emitter prints, as the text rules do; no new IR.
- The jump table's contract: while the passes run, the dispatch is
  `asl a / tax / bvs @t1 / ... / jmp @tk`; `jt_rewrite` runs after them.
- The branch pass (`relax_branches` proper) and `line_bytes`: L3 runs before
  them; a wrong length still fails the link, never the program.
- Every `QBE_NO_*` switch of the existing rules keeps working until the rule is
  retired (step 5), so that an A/B is one variable.

## The steps

| Step | What | Code changes? | Proof obligation | Predicted gain | Cost |
|---|---|---|---|---|---|
| **S0** | the instruction model: every line the emitter prints parsed into a record (mnemonic, addressing mode, operand, width of A and of X/Y in force, what it reads and writes among A low / A high / X / Y / N / Z / C / V / D / the stack pointer / memory by class: a direct-page slot by byte, a stack slot by byte, a named global, through a pointer, an I/O register), and printed back | none: the text that leaves is the text that came in | (a) the 65 example ROMs, the compiler checks and the torture subset byte-identical; (b) a strict mode (`QBE_L3_STRICT=1`) where a line the model does not know is an internal compiler error, run on the corpus, `make test-torture` and 20 000 programs of each differential test: zero unknown line; (c) the read / write sets of every mnemonic and mode checked once against the WDC data sheet table, line by line, in the note | 0 | M: a table of the instruction set as the emitter uses it, about 600 lines |
| **S1** | the analyses: basic blocks and successors from the text (labels, the branch forms the emitter and the rules write, `jmp`, returns, the jump table's `bvs` group), then backward liveness of each register, flag and direct-page byte, and forward knowledge of the widths in force | none: nothing consumes it yet | **dead-value poisoning**: a test build writes garbage wherever the analysis says a thing is dead (`ldx #$DEAD`, `ldy #$BEEF`, `lda #$5A5A` for A, `sep #$C3` or `rep #$C3` alternately for N/V/Z/C, a store of `$A5A5` into a dead direct-page slot), placed after every instruction; the compiler fixtures with a host model, the torture subset and 20 000 programs of each differential test must keep their results (not the examples' frame hashes: the poisoned code is slower and larger, so a frame that depends on timing may legitimately move; the examples are run poisoned for liveness and crashes only). A liveness that is wrong in the dangerous direction (dead when live) changes a result; one that is only conservative changes nothing. Plus a negative control: the same build with one deliberate error (C never read by `adc`) must fail | 0 | M: about 500 lines; the poisoning build is a switch of the same pass |
| **S2** | the first consumers: the direct-page dead store (`peephole_slotdead`) and the dead-flags question (`pp_nz_dead_after`) answered by S1; the old predicates kept and compared in a check mode | yes, only removals the old rules could not prove | the 65 ROMs differ only by removed instructions (a diff that adds or moves a line is an error), poisoning still passes, 20 000 seeds of each differential test, `make tests`; in check mode every place where the old predicate says dead and S1 says live is listed and explained (an old rule that was wrong, or S1 too conservative) | small: the count of extra removals measured on the bench and the two games before the step is closed | S |
| **S3** | **values in X and Y over a region**: a direct-page temporary (a `tcc__lf` slot, then a stack slot of a function that calls) whose live range is a region where X (or Y) is dead and every access can be rewritten: `lda T` -> `txa`, `sta T` -> `tax` (when N/Z are dead after), `ldx T` -> nothing, `inc T` -> `inx`, `lda sym,x` with X = T kept as is; an access that cannot be rewritten (an ALU operand, a byte access, a call in the range) excludes the temporary, or splits the region where a spill and a reload cost less than they save. Chosen by a weight: the accesses saved times their loop depth, from the costs of the data sheet, against the moves added | yes | poisoning on the new code; 20 000 seeds of each differential test per rule; the 65 ROMs against those before, frame for frame; both games' state identical at every step; a counting mode (`QBE_L3_COUNT=1`) that prints, per function, the candidates and their weight without changing the code, run FIRST on the bench and the two games to make the prediction | **predicted by the counting mode before any rewrite**; bounded above by the nameable share of item 1, 11.6 and 10.8 % of the two games' busy time, and by the hand's 32 + 12 cycles of 156 on the leaf of game 1 | L: about 800 lines; the largest step, cut again if the counting mode shows two kinds of candidate |
| **S4** | values in A over more than a straight line: the A-cache across blocks (today `peephole_xflow`'s A half) re-expressed on S1, and the reload of a value A still holds after a branch | yes | as S3 | to be counted at the end of S3 | M |
| **S5** | the old text rules retired one by one when S2 to S4 do their work: `peephole` (reload, dead store, dead load, X reuse), `peephole_xflow`, `peephole_slotdead`, `peephole_entry`, the X and Y loop rules; each retirement is its own commit | yes: each retirement must leave the 65 ROMs identical or smaller | the ROMs of the corpus identical or with fewer instructions, poisoning, 20 000 seeds; a retirement that makes one example larger is not done until the case is understood | maintenance: one reading of the text instead of twenty | M, spread |

No step starts before the previous one is measured (plan, section 5). S0 and
S1 change no byte of any ROM; they can be merged as soon as their proof holds,
and they are what the later steps stand on.

## Why poisoning is the proof of S1

A peephole mistake is silent: the assembler accepts the text, the program runs,
the value is wrong once in 100 000 programs. The rules so far are each proven
by a hunt of 10 000 to 20 000 seeds, which tests the rule's output but not the
reasoning behind it. Poisoning tests the reasoning itself, everywhere at once:
if the analysis calls X dead at a point where something will read it, the
garbage reaches a result on the first program that goes through that point.
It needs no oracle beyond the ones we have (the differential tests' models, the
fixtures' host models, the examples' frame hashes). It is the analysis's
contract made executable; every later step inherits it (a transformation that
consumes a wrong fact fails under poisoning before it ships).

What it does not prove: that the analysis is precise (a conservative answer
passes), timing (the poisoned build is slower: what depends on the frame is not
its oracle), and paths no test executes. The second is the differential tests'
coverage, the same limit as today; S1 adds a report of the blocks poisoned and
never executed by any test, read before S3 is written.

## Risks

- **Width.** The 65816 instruction means another thing in 8 and in 16 bits;
  the model must know the width in force at each line, and a `rep`/`sep` the
  analysis cannot place (after a call, at a label with two widths in) must end
  what the model claims. S0's strict mode refuses a line whose width it cannot
  establish.
- **What the emitter writes in strings the rules splice** (`beq + / jmp @B / +`
  written as one line by `peephole_dec_loop`): S0 parses after the split.
- **The jump table**: the `bvs` group must read as a multi-way branch, and S3
  must not take X across it (the `tax` belongs to the dispatch).
- **Interaction with the rules that stay until S5**: S2 to S4 run where in the
  sequence of `relax_branches`? Proposed: after every existing rule but
  `peephole_entry` and `jt_rewrite`, so that each S-step sees the text the old
  rules leave, and `peephole_entry`, which removes a load another rule may
  count on for its flags, still runs last. Each S-step re-runs the poisoning.
- **Compile time**: one parse and a fixed-point liveness per function; the
  largest function of the games has a few thousand lines. To be measured in S1
  (budget: the corpus build no more than 10 % slower).

## Contested

(written by the thinker before S0 starts. Pair for this lot: Opus writes and validates; a Fable
sub-agent contests the plan, 2026-11-12, the one use of Fable the owner allows: the written contest of a
compiler lot's plan.)

**Verdict: go with changes.** The axis is right; the plan over-states S3's ceiling by about a factor of two, its proof is blind to
the errors S3 is most exposed to, and it spends two steps before any figure says whether the lot pays. (Static counts on the 75
`*.c.asm` of the examples as built 2026-11-12; cycle figures from the timing rules, not measured; other compilers from memory.)

**(a) What would make it not pay.**
- The 11.6 / 10.8 bound is the sum of six rows of `our_asm_on_real_games.md`; only three are X/Y promotion: temporaries and reloads
  across a branch 3.4 / 3.9 (S4, A-cache); index fetched from the stack 1.2 / 2.1 (L4: the value crosses a call, X and Y are
  caller-saved, `compiler/ABI.md`); operand passed through a temporary 1.6 / 0.8 (operand fusion); loop counter, doubled index, X
  shuttling 5.4 / 4.0 (**S3**). S3's bound is 5.4 / 4.0, overlaps included, counted before lot 1 put the counter in Y.
- A promoted access is cheaper, not free: `lda.b T` (16 bits) 32 mclk, `txa` 14: 18 of 32 saved; only `ldx T` -> nothing saves all.
  The direct-page share of item 1 is 18.0 / 19.0 % (27.2 - 9.2, 37.1 - 18.1).
- Of 610 (function, slot) pairs with 4 449 accesses, 411 (1 915 accesses, 43 %) have only `lda/sta/stz/ldx/ldy/inc/dec`; the rest
  have an ALU operand (`adc` 101 slots, `sbc` 82, `cmp` 39). The two heaviest candidates per function: 1 526 accesses (34 %).
  **Hypothesis: 18 % x 0.34 x 0.56, about 3.5 % of busy time for S3, not 11.**
- S3's list is poorer than the rule it would retire: `pp_alu_of_slot` already turns `lda P / clc / adc T` into
  `txa / clc / adc P`. Missing: `lda T / cmp #k` -> `cpx #k`, `lda T / sta G` -> `stx G`, `lda T / pha` -> `phx`,
  `lda G / sta T` -> `ldx G`, `stz T` -> `ldx #0`, `dec T` -> `dex`, `txy` / `tyx`. Width: `tax` with an 8-bit A copies the
  hidden byte, so a `sta.b T` in a `sep #$20` region is not a `tax`.
- The hand's 32 cycles of "index held in X" need X held for the whole object while A computes, the second index in Y: only if the
  emitter's own scratch use of X (the `tax` of every indexed access) leaves the region, which is instruction selection.

**(b) What it could break.**
1. Poisoning proves backward facts only: a missing WRITE in the model (`inx` absent) makes liveness conservative and passes, yet
   falsifies "X still holds T", which S3 and S4 consume (the `pp_writes_x` lesson). S3 needs a second executable contract: an
   assertion build that keeps the shadow store and emits `cpx T / beq + / brk` wherever the analysis claims X == T.
2. `sep #$C3` / `rep #$C3` only where N, V, Z and C are all dead: a mask per flag. `lda #$5A5A` and `ldx #$DEAD` set N and Z
   themselves: `php / plp` around them when N/Z are live.
3. The high half of A is untested unless an 8-bit region gets `xba / lda #$5A / xba`.
4. Stack slots move: `N,s` names another slot after each `pha` / `pea`; the stack depth at every line and every merge is needed.
5. The text does not carry the return type: `rtl` reads A and `tcc__retval_hi`; a `jsl` writes A, X, Y, P, every `tcc__r*` and
   `tcc__lf`, and reads the `tcc__r*` inputs of the runtime helpers.
6. The shared direct page is sound (caller-saved `tcc__lf`, the NMI's mirror); hand assembly breaking the ABI would surface.
7. Poison goes in last: a line inside the `asl a / tax / bvs` group makes `jt_rewrite` stop (refused, not silent); insert after
   it, the model knowing `jmp (@jtt.N,x)`; it also changes what `peephole_entry` and `peephole_r9_loop` match.
8. S0's byte-identical round trip proves nothing about the model (printing the stored string satisfies it); strict mode proves
   coverage, not truth; the hand-checked table is the weak link. Use luna as the oracle: on a trace, a register or flag that
   changes across an instruction and is not in its write set is a model error.
9. One negative control is too few: one per resource (A low, A high, X, Y, each flag, a direct-page byte, a stack byte).

**(c) Ordering and cut.** Count before S0 (a script over the `.c.asm` weighted by the games' traces is enough to decide). Placement
after the old rules is doubtful: `peephole_loop_x` has taken the hot loops, so S3 would measure leftovers; count with the loop rules
off and on, and treat S3 as replacing `loop_x` in the same commit. Move S2's check mode (old predicates against new over the corpus)
into S1: a free second proof (`pp_nz_dead_after` looks at 8 lines only).

**(d) State of the art.** llvm-mos allocates before emission (imaginary zero-page registers as a register class, A/X/Y constrained
classes, a real allocator); vbcc and Calypsi use zero-page pseudo-registers; cc65 has `register` variables in zero page. None
recovers allocation from text. Here `color_slots` already has liveness, interference, `cross[]` and a loop-weighted cost, and
`SOFF()` / `emit_stack_load/store/op` are the single choke point: a third colour, "lives in X / Y", for a temp whose every use is
index, copy, step or compare with a constant, inherits types, widths and aliasing from the IR, where S0 must re-derive them from
text in about 1 100 lines. Its difficulty is knowing where the emitter clobbers X (a dry-run emission can record it). S0 + S1 then
remain as the verifier, which is their real value.

**(e) Changes, ranked.** 1. Take the count first; restate S3's bound as 5.4 / 4.0 at most. 2. A one-day spike of the IR-side X/Y
colour on gapbench `col` / `row` and leaf B before committing to S3 in text. 3. Fix the poisoning specification (b2-b5, b7, b9) and
add the forward assertion build. 4. Validate the model against luna traces, not the data sheet by hand. 5. Extend S3's forms
(commutative ALU, `cpx` / `stx` / `phx` first). 6. Decide L3 against L4 on the count: the stack is 9.2 / 18.1 % and S3 cannot
touch it. **The one measurement before S0**: on today's compiler and both games' traces, the master cycles of the accesses to
direct-page slots whose every access in their live range is rewritable while X or Y is free, split by form, with
`peephole_loop_x` on and off. **Under about 3 % of busy time on both games, S3 does not open as written and L4 goes first.**


### The plan after the contest (accepted, 2026-11-12)

All six changes are taken. The lot now starts with a measurement, not with S0:

1. **M0, the count** (before any compiler code): on today's compiler, weighted by both games' traces, the master cycles of
   accesses to direct-page slots whose every access in their live range is rewritable while X or Y is free (the extended list
   of forms of change 5), split by form, with the loop rules (`peephole_loop_x`, `QBE_NO_PEEP_LOOPY`) on and off; and the same
   count for stack slots, which only L4 reaches. A script over the `.c.asm` and the traces, outside the compiler.
   **Decision rule: under about 3 % of busy time on both games, S3 does not open as written and L4 goes first.**
2. If M0 passes: a one-day spike of an X/Y colour in `color_slots` (the IR side) on gapbench `col` / `row` and leaf B, measured
   against the counted gain; the text-side S3 only if the spike shows the IR side cannot see where the emitter takes X.
3. S0 and S1 become the verifier of whichever side is chosen: the model checked against luna traces (a register or flag that
   changes across an instruction outside its write set is a model error), the old predicates compared with S1 in S1 itself,
   poisoning per flag with `php / plp` where N/Z are live, the high half of A poisoned in 8-bit regions, the stack depth
   tracked at every line and merge, `jsl` / `rtl` with their full read and write sets, poison inserted after `jt_rewrite`,
   one negative control per resource, and a forward assertion build (`cpx T / beq + / brk` where X == T is claimed).
4. S3's bound restated: **5.4 / 4.0 % at most, about 3.5 % as hypothesis**, not 11.6 / 10.8.

## Measured

### M0, the count (2026-11-13)

Method (private tooling outside the repository, the gauge's builds: frozen game sources, today's compiler `develop`
`0a9b9fa0`): luna traces of 4 x 40 frames of each game's loaded scene (the windows of the 2026-11-11 count); every executed
instruction of compiled C that reads or writes a direct-page temporary (`tcc__lf+N`) attributed to its (function, slot),
with its measured cost. Statically, from the `.c.asm`, a slot is **rewritable** when all its accesses are in the contest's
extended list (`lda` -> `txa`, `sta` -> `tax`, `stz` -> `ldx #0`, `ldx` -> nothing, `ldy` -> `tyx`/`txy`, `inc`/`dec` ->
`inx`/`dex`, `lda T / cmp #k` -> `cpx #k`, `adc`/`and`/`ora`/`eor` when the operation commutes after `lda P`), and excluded
otherwise (a `cmp T` or `sbc T`, a byte access, an indirection). The saving of an access is its measured cost minus the
replacement's (12 mclk for a transfer or a step, 18 for `ldx #`; all of it for a removed `ldx T` or the `lda` of
`lda T / cmp #k`). **High** bound: every rewritable slot, the register assumed free. **Middle**: X (else Y) free over the
slot's textual span (first to last access in the function; optimistic for loops, pessimistic where a slot hosts several
temporaries), slots of one register not overlapping, greedy by saving.

| % of busy time | game 1 | game 2 |
|---|--:|--:|
| accesses to direct-page temporaries in compiled C (whole cost) | 19.9 | 22.8 |
| S3 high bound (register assumed free) | 2.73 | 6.63 |
| **S3 middle (register free over the span)** | **1.11** | **2.21** |
| the same with the loop rules off (`QBE_NO_PEEP_LOOPX=1 QBE_NO_PEEP_LOOPY=1`) | 1.54 | 2.20 |
| excluded slots read by `cmp T` / `sbc T` (whole cost of their accesses) | 7.5 / 7.0 | 3.9 / 5.7 |
| **stack slots `n,S` in compiled C (whole cost; L4's reach)** | **9.8** | **19.2** |

Checks: the direct-page share agrees with the contest's independent estimate (18.0 / 19.0); the busy time per frame of the
traces is within 5 % of the gauge's on its wider window; with the loop rules off both games' state is identical to the
reference at every step (gauge, 341 and 700 steps), and game 1 is 2.6 % busier (what the loop rules already give).

**Decision, by the rule written before the count: S3 does not open as written; L4 goes first.** The middle figure is under
3 % on both games, with the loop rules on or off. By form, the saving is mostly `sta`/`lda` pairs (0.4 + 0.35 and 0.75 +
0.77 of the middle), not the index held in X the hand used: that needs X freed of the emitter's own use (the `tax` of every
indexed access), which is instruction selection; the IR-side spike of the contest (change 2) is the way to measure it, after
L4. What the count also shows:
- the slots read by a non-commutative operation (`cmp T`, `sbc T`) weigh more than all rewritable ones (whole cost 14.5 and
  9.6 % against 2.7 and 6.6 high): an operand that could stay in A, or a reversed compare, is the larger axis (S4's, and
  L5's for the compares);
- the stack is 9.8 and 19.2 % of busy time, the largest single class of temporaries. That is L4's whole cost, not its
  saving: L4 gets its own count first (accesses that a static direct-page frame would make cheaper, and what they enable:
  `inc`, `ldx` from the slot, no frame setup), with a decision rule written before it.

S0 and S1 (the instruction model and the proven liveness) are kept in the plan as the verifier of any later rewriting, not
started now.

## Decision after M0, L4 M2 and M3 (2026-11-13), and the next lot

By the 3 % rule written before the counts: **L4 does not open as written** (static frames F1 2.4 / 2.7 %; with change 3, 2.9 / 3.3 %,
on a 136-byte window the direct page does not have). **The `tcc__lf` axis does not open** (ceiling 0.7 / 1.7 %; M2's 3.4 / 7.2 was
whole cost, not saving). **S3 does not open** (1.1 / 2.2 %). L4's cheap part stays on the shelf: change 3 alone is not counted apart.
**Next lot: S4 / L5, the operand kept in A and the compares**: the slots read by `cmp T` / `sbc T` weigh 7.5 / 7.0 and 3.9 / 5.7 %
of busy time as whole cost, more than any rewritable class counted so far.

Rule for this lot, written before its count (M5): its saving is counted as instructions removed or made cheaper, not as the
whole cost of the slots. It opens if the executed saving on accepted sites is **3 % or more of busy time in one game** and
the other is not worse. Count: in the traces, each `sta T` ... `cmp/sbc T` pair where A is dead after the compare or T can be
replaced by the reversed operation; and each compare that is a signed range test with a provable range (L5). Report by shape.
If it fails the rule, the conductor gets M4 (top-cost instruction shapes, whole program) instead of a lot.

### M5 and M4 (2026-11-13), and the ranked list the conductor asked for

**M5** (private `m5.py`, same traces): `lda SRC / sta T ... cmp|sbc|adc|ora T`, T in `tcc__lf` dead afterwards on the executed
path, SRC an immediate, direct-page or absolute operand untouched in between: the `lda` and `sta` go, the compare takes SRC.
2 809 sites, **0.58 %** of busy (game 1); 1 311 sites, **0.21 %** (game 2). By the rule written before the count (3 %), the narrow
form does not open. It counts only the adjacent pair; the wider A-cache across blocks (M0 bound 3.4 / 3.9) is not measured by it.
**M4** (`m4.py`, executed cost by instruction shape, compiled C = 93 % of busy): game 2 spends 11.8 % in `lda n,S`, **10.9 % in
`lda long,X`**, 9.9 % in `sta dp`, 7.2 % in `lda dp`, 5.9 % in `sta n,S`; the pair `tax ; lda long,X` alone is 7.1 %. Game 1 has
`lda abs,X` at 9.0 % where game 2 has the long form. Hypothesis, not yet checked: a long-indexed read costs a cycle more than the
absolute-indexed one; 96 187 executions x 6 mclk = **1.6 % of busy** in game 2 if the bank could be a data-bank register
(why the bank-$00 rule makes C const reads far, and what a DB switch would cost, is the first thing to read).

**Ranked list (gain / cost, overlaps flagged; none opens alone under the 3 % rule, the set is what the conductor asked to keep):**
| axis | gain, game 1 / 2 (% busy) | cost | overlap |
|---|--:|---|---|
| `tcc__lf` +4 words (M3) | 0.64 / 1.14 | 8 bytes of direct page, one constant, S | with static frames (same slots) |
| long-indexed reads to absolute (M4, hypothesis) | ? / 1.6 | DB handling, to be read | none known |
| static frames F1 + change 3 (M2) | 2.9 / 3.3 | 136-byte window, link-time placement, L | with `tcc__lf` axis, X/Y values |
| values in X/Y (L3 M0 middle) | 1.1 / 2.2 | instruction model (S0/S1), L | with static frames' temps |
| operand in place, adjacent (M5) | 0.58 / 0.21 | local rule, S | with X/Y values |
Not additive; the conductor's estimate for the set is 5 to 8 % on game 2. The next count that serves the list is the
long-indexed one (game 1 first: why it uses `abs,X`), then the union of the sites of the others, to give the overlap a number.

### Long-indexed reads counted (2026-11-13, same traces)

Targets of the executed `op $BBHHHH,X` by bank (trace operands). Game 2: 143 061 executions, of which 75 637 `lda` and 34 872 `sta`
in bank **$7E** (FAR RAM: the object table), then `cmp`/`adc`/`sbc` $7E, and 7 092 `lda` in a ROM bank. Game 1: 12 714
executions (`lda` from asset banks $9E, $87, $A5; 785 `sta` $7E). So the long form is not the const-read rule of bank $00: it
is FAR RAM, which an absolute-indexed form cannot reach with DB = $00. Ceiling if every one cost a cycle less (6 mclk each):
143 061 x 6 / 35 687 410 = **2.4 %** of game 2's busy time, ~0.2 % for game 1 (hypothesis of one cycle per long form is the
65816 timing table, not re-measured by luna here). A DB switch to $7E per function would also break I/O ($21xx) and ROM reads
in the same function: cost out of proportion to a gain under the 3 % rule. **The axis does not open.** The list above stands
with this line: 1.6 -> at most 2.4 / 0.2, not additive with the others, no lot.

## Lot LF+M5 (opened by the conductor, 2026-11-13): `tcc__lf` +4 words and the adjacent operand in place

The conductor opens one short lot from the ranked list, two axes that do not overlap in code: (a) `tcc__lf` grows by four
words (16 -> 20 words, 32 -> 40 bytes of direct page; the NMI mirror 160 -> 168 bytes); (b) M5's shape, `lda SRC / sta T /
[ldx|ldy ...] / lda Y / [clc|sec] / op T` with T a `tcc__lf` slot dead after the operation and op one of cmp, adc, sbc, and,
ora, eor: the `lda` and the `sta` go, the operation takes SRC (`peephole_fuse_indexed`, which already does it for SRC =
`SYM,x`). Pair: Opus writes and validates (no sub-agent for this lot, the conductor's instruction).

**Prediction, written before the code** (from M3 and M5, the same traces): (a) 0.64 / 1.14 % of busy time (game 1 / game 2),
(b) 0.58 / 0.21 %. Overlap: (a) moves stack temps into `tcc__lf`, which can only add sites to (b) (a T that was `n,S` becomes a
candidate) and never removes one; (b) removes accesses to `tcc__lf` slots that (a) did not count (M3 counted stack accesses
only). So the two are additive or slightly better: **about 1.2 % (game 1) and 1.35 % (game 2) together**, at most the sum of
the ceilings 0.66 + 0.58 = 1.24 and 1.66 + 0.21 = 1.87. M5 counted SRC as immediate, direct page or absolute; the rule also
takes a stack slot `n,s` (same proof, nothing moves the stack between), so (b) may come out a little above 0.58 / 0.21.
Limits stated before: M3 classified on the executed path of one run (the compiler decides statically, so the real (a) is at
most its figure); M5's deadness was read on the executed path too (the rule asks `pp_slot_live_at` over all paths, so it
accepts at most what M5 counted, plus the stack sources).

### Measured (2026-11-13), against the prediction

Gauge (busy master cycles per game step, the gauge's window; both games' state identical to the reference at every step,
341 and 700 steps; builds of this worktree before the change, `lfm5-avant`, and after), each axis alone by its switch
(`QBE_LF_WORDS=16` keeps the old 16 words, `QBE_NO_PEEP_FUSE_SRC=1` turns the new sources off):

| % of busy time saved | game 1 | game 2 |
|---|--:|--:|
| (a) `tcc__lf` +4 words alone, predicted | 0.64 | 1.14 |
| (a) measured | **0.00** | **0.24** |
| (b) operand in place alone, predicted | 0.58 | 0.21 |
| (b) measured | **1.41** | **0.29** |
| together, predicted | 1.2 | 1.35 |
| **together, measured** | **1.42** | **0.53** |
| code size | -0.72 % | -0.58 % |

gapbench: no row moves (its kernels have neither shape and fit 16 words). The corpus: 65/65 MATCH at equal PPU frame
(`diff_corpus.py`), eleven examples' C and four library files change.

Why (a) gives so little: M3 called a stack value movable when none of its reads came after a call **on the executed path**;
`color_slots` decides statically, and a temp live after a call on any path is `cross` and stays on the stack. With (a) alone,
three cold functions of game 1 change and none of the hot ones; in game 2 seven change, one of them hot (-4.3 % on it).
M3's figure was a ceiling, not a prediction, and the ranked list read it as a gain. Why (b) gives more in game 1: M5 counted
the `lda` / `sta` pair only; once it goes, the base peephole removes the `sta T0 / lda T0` that fed it (554 lines removed for
148 rewritten in game 1); hypothesis that this chain is the whole gap, not separated by a measurement. Lesson: a count that
classifies values for a decision the compiler takes statically replays that decision, or its figure is written "ceiling".

Gates of the lot: `difftest.py` and `difftest_stmt.py` 20 000 seeds each, clean; `make test-compiler` 150/150 (checks
`fuse_src`, `fuse_src_off` new; `leaf_split`, `peephole`, `test_global_vars` follow the new shapes); `diff_corpus.py` 65/65.
Three oracles moved, each explained: the WRAM streams of every example (every variable after `tcc__lf` and the NMI page
moves by 8 bytes); `audio/sfx_from_wav` (the sound 2 samples later, `luna diff --audio --align-onset` MATCH, max delta
0.00 %); two manifests: `hdma_gradient_colors_indirect` reads RAM addresses (recomputed from `band_data`, +8) and
`state_rooftop_run` checked Kit's height at the frame he reaches the gap's edge: the build after has two lag frames fewer by
frame 450 (59 -> 57, `luna profile`, one at the respawn), so its game runs a frame ahead and Kit is one pixel into the fall
at 600 (`kit_x` one step ahead from the respawn on, the trajectories otherwise identical); the checkpoint moved to 560.
