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

## Measured

(filled step by step)
