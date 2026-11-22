# G1 R3 — design note of the prototype (written before any code)

Written 2026-11-15 by the thinker of the pair (Opus), read-only on the code. Nothing here is measured by this note: every
figure is quoted from a file (named), every guess is written **hypothesis**. Inputs read: `G1_generator_prototype.md`
(Steps, the contest of revision 2, the conductor's answer on the free convention, R-1, R0's plan), `G1_r0_result.md`,
`G1_r1_result.md`, `G1_r2_result.md`, `G1_ir_probe.md`, `L4_frames_and_calls.md`, `L4_plan_decoupe.md`, `L4_s3_result.md`,
`I1_inline_plan.md`, `I1_s4_result.md`, `tech/our_asm_on_real_games.md`, `compiler/ABI.md`, `.claude/rules/compiler.md`,
`templates/crt0.asm` (NMI entry), the branches `g1-r0` / `g1-r1` / `g1-r2` (`devtools/g1bench/`, `testing/fixtures/g1_invariants/`).
Names: the game 1, the game 2, the other 65816 compiler. Nothing of R3 enters `develop`.

## 0. What R3 must answer, and the line it is judged on

R3 is the first stop of the owner's rule (`G1_generator_prototype.md` l.17-19, l.155-157): a generator that allocates A, X,
Y and the direct page as one bank and selects instructions by master-cycle cost reaches **1.1 of the hand on the two leaves**,
or says why; **not within 1.2: the prototype stops**. The 2026-11-13 refinements stand (l.65-70, l.178): master cycles per
perimeter, leaf B defined (routine alone, and with its caller), **two lines per leaf** ("allocation and selection only",
"with idioms"), a held-out set, a frozen reference commit, every figure naming its commit and switches (R-1, l.203).
The conductor's answer (l.197-199) adds the **two columns**: "ABI kept" / "free convention inside one file".

**Decision asked of the conductor before R3-1 (not after):** which line the 1.2 stop reads. Section 6 predicts that leaf B
alone reads **about 1.31 on the allocation-and-selection line** and **about 1.10 with idioms**: the choice of line decides the
verdict before any code. My recommendation, so that G1 is not credited with what any generator takes (R1 verdict point 1,
`G1_r1_result.md` l.161, l.116-118): the idioms that are IR rewrites (range test, `cond ? k : 0`) are written **once, as IR
passes behind a switch, feeding both emitters**; the stop reads the prototype's "with idioms" line, and the report gives
next to it today's generator with the same IR idioms, so that G1's own share is the difference of the two.

## 1. Input, output, and where the prototype sits in the chain

Chain today (`compiler.md` l.18): `.c -> cc -E -> cproc -> QBE IR -> qbe w65816 -> .asm -> wla-65816`.

- **Input: the QBE IR of one function as `emitfn` receives it** in `compiler/qbe/w65816/emit.c`, i.e. after every middle-end
  pass the target runs: inlining (`splice_fn`), `ivar` (counter as scaled index, `compiler.md` l.55), `reassoc` /
  `testbound` (l.70), `gvn`, `gcm`, `sinkload` (l.65), `cmplast` (l.60), `sweepdead` (l.61), `threadjnz` (l.44). The target
  skips `spill` / `rega` (`T.skiprega`, l.61), so the IR is still **SSA with phis**: the natural input of an allocator.
  Settled by the contest (`G1_generator_prototype.md` l.81): allocate on the IR, before the text rules. Consequence read in
  R0: the IR families `loops` (`IVAR REASSOC_LOOP SINK_LOAD`) and `REASSOC` are inputs the prototype keeps for free; what it
  must earn back is the emitter's own work (section 2.3).
- **Output: the WLA-DX text of that one function**, written into the same temporary file `emitfn` writes (`relax_branches`,
  `compiler.md` l.59), in place of the current emitter's text: the section header and label, `.ACCU` / `.INDEX` after every
  `rep` / `sep` (`CLAUDE.md`, WLA tracking), and the **static-frame side table** lines (`; sframe fn`, `; sframe call`;
  `compiler.md` l.73: `opensnes-rom check --objects` refuses a link whose instruction lines and table disagree, so a function
  the prototype emits must carry its table line even as a leaf).
- **After the prototype**: only branch shortening (`SHORT_BRANCH`, +67 % when cut alone, R0 l.177: kept by the contest's
  point 9, confirmed by R0's verdict point 2) and `jt_rewrite` (unused: functions with a jump table are refused, section 7).
  Every other text rule is **not applied** to the prototype's text but **run in count mode as the control**: the number of
  lines each rule would change on the prototype's output is reported (expected zero, contest point 9, l.81); a non-zero count
  names a trick the prototype missed.
- **Hook**: `emitfn` asks `g1_wanted(fn)` (env `QBE_G1=1` all accepted functions, or `QBE_G1_ONLY=name,...`); the prototype
  (`w65816/g1.c`, its own data structures) returns *refused + reason* or the text. Refused functions are emitted by today's
  generator, unchanged. `QBE_G1` unset: output byte-identical to the reference (gate). `QBE_G1_TRACE=1` lists acceptances,
  refusals with reason, and the allocation of each value.
- **Reused, not rewritten** (the conductor's "reuse finished work", l.199): the read-only predicates of today's emitter for
  addressing questions (`datdef_has`, `idx_nonneg`, `sym_idx_form`, `is_far`, `addrtaken`; `compiler.md` l.45-47, l.75), the
  runtime helper calls (`tcc_*`, `__*`) as they are, the side-table printer `sframe_dump`, the branch pass, the free / public
  distinction of I4 (`addrtaken`, export).

## 2. The allocator: one bank

### 2.1 The resources
One bank of locations, each with its own legal-operation table and cost:

| location | what it may hold | constraints (each from a file) |
|---|---|---|
| **A** (16 bits) | the value an ALU chain is working on; a value held across X/Y-only instructions | every `adc sbc cmp and ora eor`, shift and load-op needs A (`G1_ir_probe.md` l.110: "a value that is an operand of `adc`... lives in A or in memory, never in X or Y"); a store with A in 8 bits copies the low byte only |
| **X**, **Y** (16 bits) | indices, counters, values only moved / compared / stepped | no `long,y` (`G1_ir_probe.md` l.19); `sym,x` adds an unsigned 16-bit index (`compiler.md` l.45); X is written by every indexed access whose index is another value (the "X of service", 30.7 / 14.1 % of the colour's refusals, `G1_ir_probe.md` l.105-108) |
| **direct-page slots** | everything else | in a leaf: `tcc__lf` (`$80-$A7`, 20 words) and the `tcc__r*` words outside any helper call's live range (helpers use `tcc__r0..r3`, `r9`, `r10`, `compiler.md` l.48); never `$40-$7F` (SNESMOD); every slot inside the 168 bytes the NMI page mirrors (`nmi_audit.md`, `ABI.md` "Direct page layout") |
| **D** | the base of the direct page, moved onto an entity (T1) | entity in bank 0 below `$2000` (R1 l.104 and the retrospective's two conditions, `G1_generator_prototype.md` l.136-137); while D is moved no `tcc__lf` / `tcc__r*` access through `.b`, no helper call; D off a page boundary costs one CPU cycle a `.b` access (R1 l.86-91) |
| **DBR** | the data bank of a run of absolute accesses (ROM table read `abs` in place of `long`) | restored before return (contest point 8); while moved, an absolute access to plain RAM is wrong unless DBR is a bank that mirrors it |

The NMI question asked by contest point 8 (l.79-80) is answered by reading `templates/crt0.asm` l.1100-1137: the handler
pushes D and DBR (`phd`, `phb`), sets `D = tcc__nmi_registers` and `DBR = $00` itself, and restores both on exit. **So a
main-thread function that moves D or DBR is safe against the NMI by construction**; what is not checked by this reading:
IRQ handlers (assembly, user-written) that might assume D = 0 (hypothesis: none in the corpus; R3-0 greps the corpus).

### 2.2 The algorithm (hypothesis until R3-0's paper allocation is measured)
1. **Hot loops first.** Loops are taken innermost first, by measured block frequency (section 3). Inside one loop body (tens
   of IR instructions in the twins), the assignment of each SSA value to {A, X, Y, slot} is searched **exactly** (branch and
   bound over the values live in the loop, at most a few dozen; edge costs are the moves between locations), with the
   instruction selection done inside the cost function (an assignment is priced by the text it forces). The plan asked
   "graph colouring or linear scan, chosen by measurement on the two leaves" (l.24-25): neither prices the asymmetry of A, X, Y;
   SSA interference graphs are chordal, so the colouring itself is easy, the cost is the problem. The search is the measure.
2. **A as a chain, not a colour.** A value is "in A" for a stretch of its life: from its definition to a use reached through
   instructions that do not write A (X/Y moves and steps, stores, branches that keep A). The allocator chooses the order of
   independent IR instructions inside a block (a local schedule) to lengthen those stretches. This is the item no X/Y colour can
   supply (`G1_ir_probe.md` l.117-118) and the larger part of the hand's gain (values 12 of 31 cycles on leaf A, 44 of 156 on
   leaf B, `our_asm_on_real_games.md` l.93).
3. **Outside the hot loops**: a linear scan over the rest of the function, with the loop's decisions fixed at the loop's
   edges (moves on the edges, as the parallel phi moves today, `compiler.md` l.41).
4. **32-bit values**: a pair of direct-page words, never split across register and memory; the high half is a location of its
   own (narrow temps keep one word, as `compiler.md` l.50).
5. **No value is held across a helper call** in A, X, Y, `tcc__r*`.

### 2.3 What exists only inside an allocator (R1's list, `G1_r1_result.md` l.19-35 and l.119, contest item 2 l.174)
| technique | R1's status | what R3 does with it |
|---|---|---|
| T1 direct page on the entity | under prediction alone (-7.8 / -2.5 / -0.2 %), **dependency named**: the function's temps leave the page (R1 l.94-98) | D is a location of the bank; the measure R1 asked is made in R3-5: `mobfp` and `mobptr` with D on the entity and no `tcc__lf` in the body |
| T2 far pointer as `[dp]`, 16-bit step | delivered as a pass (E1) | the prototype keeps a far pointer's three bytes in a slot and reads `[slot],y`; E1's step rule (`farptr_step`) reused as a predicate |
| T3 first argument in A | stopped at 0.90 / 0.97 % (A1 S0) | only in the free-convention column (section 4) |
| T9 X/Y colour | delivered (lot XY), in the baseline | subsumed: X and Y are locations of the bank; the colour's proof by emission (`compiler.md` l.74) is the model for R3's paranoid build |
| T10 free convention between statics of a file | L4 S3 stopped (+0.54 / -1.06 %) | the free-convention column; L4's side table and `place` / `check` reused, not redone |
| T4's slack (scheduling) | isolated pass measured, the slack is scheduling | **out** of R3 (T4 itself is out, section 7); the local schedule of 2.2 could use it later |

Two more that only an allocator can do, read in the hand rewrites (`our_asm_on_real_games.md` l.97, l.102-104): **DBR set
once** for a ROM table (a location of the bank, above) and **the 32-bit composition** `high << 16 | low` without extension
temporaries (selection with the pair allocation).

### 2.4 The debt it has to earn back first (R0, `G1_r0_result.md` l.159-181)
Every switch off reads 6.07 against 2.09 on the first twelve rows; alone, `SLOT_COLOR` +50.7 %, `DP_FRAME` +22.8 %,
`PHI_SHARE` +16.6 %, `NARROW_KL` +13.0 %, `BYTE_WORD` +7.9 %, `ROTATE` +4.0 %, near family +44.6 %; the families do not
add (125 %). The prototype is designed to make each of these decisions natively: slot sharing and the leaf frame are the
allocator's colouring; near / long addressing and the byte-as-word read are selection; loop rotation is the block order. It
keeps `SHORT_BRANCH`. **What it may miss** is the cross-block knowledge of the text rules (`peephole_xflow`, `peephole_entry`,
`slotdead`): L4 S3 lost exactly that (`L4_s3_result.md` l.63-69). The control count of section 1 and the held-out rows of
section 5 are the two places it shows.

## 3. Selection by master-cycle cost

- **Unit**: master cycles of each instruction, from its bytes and its accesses by region (retrospective point 1,
  `G1_generator_prototype.md` l.126): ROM byte 8 (SlowROM) / 6 (FastROM); WRAM and the direct page and the stack 8; internal
  cycle 6; I/O 6. Penalties modelled: D low byte not zero +1 CPU cycle a `.b` access (R1 l.86-89); `abs,x` / `abs,y` with
  16-bit index and a page cross, and the write forms, +1; `long,x` one byte more than `abs,x` (R0 l.95-97 reads the cost on
  `mobptr`). Two tables, slow and fast; every figure of R3 is given on both (R0's harness does).
- **Block weights: measured, not estimated.** A static estimate by loop depth was tried by lot XY and dropped: it chose wrong
  on `box` and `slots` (`G1_ir_probe.md` l.208-211). R3 weights blocks by the luna block counts of a run of today's build of
  the same source (the twins in the bench; the leaves in the games' study copies, by their sessions). **This is a
  prototype's crutch**: a shippable generator has no profile. So every leaf figure is also given with the weights a static
  estimate would have chosen (the best of R3-2's static rules), and the difference is reported (weak point 4).
- **Choice**: a candidate allocation is priced by the text it emits, with the weights; the loop search of 2.2 keeps the
  cheapest. Width: A and X/Y stay 16 bits (contest point 7's cut, l.73: no 8-bit width regions; bytes read as word and mask,
  as `BYTE_WORD` does, `compiler.md` l.67; a byte store keeps `sep` / `rep`).

## 4. ABI at the boundary: two columns

The public boundary does not move (`G1_generator_prototype.md` l.199): a function **visible from another file, whose address
is taken, or called from assembly or the NMI** keeps the ABI. The test of "free" is today's (`addrtaken`, not exported:
`compiler.md` l.47), the one I4 already pins (`G1_r2_result.md` l.35, l.92: the FREE change stays green, the public ones go
red).

| item | ABI kept (every function the prototype emits in this column, and every public one in both) | free convention inside one file (`static`, address never taken, every caller emitted by the prototype) |
|---|---|---|
| entry width | `rep #$20` at entry (caller may arrive with A in 8 bits, X/Y in 16: `ABI.md`, R2 l.173-177) | none: the caller is known to be in 16 bits (as today for non-exported functions) |
| arguments | pushed left to right, read at `framesize + 4 + ...` (`ABI.md` "Callee side") | in A, X, Y or direct-page slots chosen by the callee's allocation; the callers comply |
| return | A (16 bits); a 32-bit value's high half in `tcc__retval_hi` | A, X, Y or a slot, chosen by the callee |
| caller-saved | A, X, Y, P, every `tcc__r*`, `tcc__lf` (`ABI.md` quick reference) | the callee's **actual** clobber set, published to its callers: a value may survive a call in a location the callee does not touch |
| call instruction | `jsl` / `rtl` | `jsr` / `rts` when caller and callee are placed in one section (one bank); else `jsl` |
| D, DBR | unchanged at return | unchanged at return (a moved D or DBR is restored before `rts` as before `rtl`) |
| stack | balanced at return, arguments popped by the caller | balanced; no arguments on the stack |
| frame | `tcc__lf` for a leaf, as today | a leaf: `tcc__lf`; a framed static: L4's static frame (`sf.<fn>.f`, its placer and checks, `L4_s3_result.md` l.95-113) — reused only when L4's window is on the branch R3 builds on, else stack |
| side table | `; sframe fn` / `; sframe call` lines | the same lines; a `jsr` is a table line too (`compiler.md` l.73) |
| who checks | R2's boundary contract (`bench_abi`: A, X 16 bits after `rtl`, S, D = 0, DBR = 0) and I4 | I4's FREE case; the paranoid build; the differential tests with static helpers (`difftest_stmt` odd seeds, `compiler.md` l.53) |

For R3's two leaves the columns differ only where a leaf is read **with its caller** (leaf A alone has no boundary in the
measure; R1 l.135). Section 6 predicts the difference is small (weak point 5).

## 5. Proof: what a figure of R3 needs before it is read

Contest item 7 (l.179): the differential proof is a gate of R3, not only R2's score. Each gate is run on the prototype's
output, with `QBE_G1=1` on every function it accepts.

1. **R2's equivalence harness** (`devtools/gapbench/run.py` with `bench_abi`, guard zones, edge hooks, poisoned A/X/Y and
   direct page; `g1-r2` `98c3703d`): every row's checksum and boundary contract, both speeds. Its score on today's output is
   **96.2 %, 77 of 80, the three survivors equivalent** (`G1_r2_result.md` l.160-164). Re-run `mutate.py --seed 1` (80
   faults) on the **prototype's** text of the same functions: at least 90 % detected and every non-equivalent mutant caught,
   or the prototype's figures are not read.
2. **The six invariants** (`testing/fixtures/g1_invariants/run.py`, `G1_r2_result.md` l.84-95): 6 / 6 green and every
   listed fault red with the prototype on; I4's FREE change green.
3. **20 000 seeds of each differential test** (`difftest.py`, `difftest_stmt.py`) through the prototype, with the **fallback
   rate**: share of functions accepted, share of IR instructions accepted, and the reasons. The hot loops of the two leaves
   and the twins must be accepted whole (0 fallback), or no figure.
4. **The paranoid build** (`QBE_G1_ASSERT=1`): every value also stored to a shadow slot at its definition and compared with
   its location at every use (`stp` on a difference); every register the allocator declares dead is written with `$A55A`
   at that point. Lot XY's assertion build is the model (`compiler.md` l.74) and its lesson: the assertion build itself had
   three defects before it could be trusted, found on its first 100 seeds (`G1_ir_probe.md` l.219-222). Run under
   `difftest_stmt` (at least 300 seeds clean) and with provoked faults.
5. **Provoked faults of the prototype**, each must go red: `QBE_G1_FAULT=interf` (one interference ignored),
   `=width` (an immediate emitted at the wrong width after a `rep`), `=dbr` (DBR not restored), `=dmove` (a `tcc__lf .b`
   access left while D is moved), `=clobber` (a free callee's clobber set published one register short).
6. **Width-flow check** on the emitted text: M and X tracked through every path; each immediate and each `.ACCU` agrees.
7. **The `jsl` / `rtl` contract on the luna trace** of the games' study copies at every entry and return of a prototype
   function: S balanced, D and DBR as at entry.
8. **Byte-identical off**: `QBE_G1` unset, the 65 ROMs and both games' ROMs equal the reference build.
9. **The games' state identical** at every gauge step (341 and 700 steps, `G1_ir_probe.md` l.78-80), by their sessions.
10. **The control count** of section 1 (text rules in count mode on the prototype's output).

## 6. Prediction, written before any code

### 6.1 The starting points (each quoted)
- **Leaf A** (game 2, no call): today **16 367 410** master cycles, hand **12 707 660**: **1.288** (`G1_ir_probe.md` l.84,
  lot XY l.193: the colour took 0.0 %). Its 2026-11-11 decomposition (`our_asm_on_real_games.md` l.84-104): C 17 153 702,
  gap 4 446 582; 53 % of the gain in the loop (per sub-step of 31 cycles: values 12, counter 11, data bank 4, stack 2, `clc`
  2), 47 % outside (32-bit composition, an index re-read from the stack, the frame).
- **Leaf B** (game 1): routine alone today **8 828 590**, hand **6 107 176**: **1.446**; with its caller **13 214 632** /
  **10 232 184** after the probe, **22.8 %** of the gap after lot XY (`G1_ir_probe.md` l.85-86, l.193), so about
  13 115 394: **1.282** (calculation). Decomposition (`our_asm_on_real_games.md` l.84-99): C 10 704 344, gap 4 601 198; per
  moving actor of 156: values 44, index in X 32, counter 28 (allocation, 104), signed range test 34, data bank and second
  index 9, `cond ? k : 0` 9 (idioms, 52).
- **The debt** (R0): 6.07 all switches off against 2.09 (section 2.4). **The twins** today (R0 table l.63-64): `soa` (leaf
  B's shape) 1.46 SlowROM, `glide` (game 2's sprite loop) 1.38. Leaf A has **no neutral twin yet** (R3-0 writes one).

### 6.2 What is left of each gap (calculation; the attribution is a hypothesis)
- Leaf A: 786 292 cycles were gained since the hand rewrite (17 153 702 -> 16 367 410). **Hypothesis**: that is the counter
  item (11 / 31 x 53 % x 4 446 582 = 836 000; the probe says its counter already sat in a register, `G1_ir_probe.md` l.114).
  Left: loop 20 / 31 x 53 % = 1 520 000 (values 912 000, data bank 304 000, stack 152 000, `clc` 152 000) and outside the
  loop 2 090 000; sum 3 610 000 against 3 659 750 measured (1.3 % apart: the decomposition is consistent).
- Leaf B alone: 1 875 754 gained since (10 704 344 -> 8 828 590). **Hypothesis**: all of it on allocation items (index and
  counter: the loop rules and the X/Y colour), none on the idioms. Left: idioms 52 / 156 x 4 601 198 = 1 534 000, allocation
  2 721 414 - 1 534 000 = 1 187 000, about the "values" item (44 / 156 x 4 601 198 = 1 298 000): what the colour could not
  hold (`G1_ir_probe.md` l.43, l.111).

### 6.3 What the prototype takes (hypotheses, the take rates are the design's bet)
| item | take, range (point) | why |
|---|---|---|
| values in A (both leaves) | 50 to 90 % (70-75 %) | the chain of 2.2; the hand's local order may use moves a generator does not make (weak point 3) |
| leaf A: stack re-read | 100 % | a leaf with all its values in the bank has no stack slot |
| leaf A: outside the loop (32-bit composition, frame) | 40 to 70 % (55 %) | pair allocation and selection |
| `clc` the range makes useless | 0 | range analysis, out |
| data bank set once | 80 to 100 %, "with idioms" line only | DBR as a location; it is selection, but counted on the idioms line to keep the first line strict |
| range test, `cond ? k : 0` (leaf B) | 70 to 100 % (85 %), "with idioms" line only | IR passes shared with today's generator (section 0) |

### 6.4 The prediction (SlowROM; FastROM expected within 0.02 of it, as every R0 row: hypothesis)
| leaf | today | R3, ABI kept: allocation and selection | R3, ABI kept: with idioms | R3, free convention: allocation and selection | R3, free convention: with idioms | today + the same IR idioms |
|---|--:|--:|--:|--:|--:|--:|
| leaf A (no call) | 1.288 | **1.15** (1.10 to 1.22) | **1.12** (1.07 to 1.19) | = ABI kept (no boundary in the measure) | = ABI kept | 1.288 (it has no IR idiom) |
| leaf B alone | 1.446 | **1.31** (1.28 to 1.35) | **1.10** (1.03 to 1.17) | = ABI kept (no boundary in the measure) | = ABI kept | 1.23 |
| leaf B with its caller | 1.282 | **1.20** (1.18 to 1.23) | **1.07** (1.03 to 1.12) | 1.20 minus at most 0.01 | 1.07 minus at most 0.01 | 1.15 |

The point figures are the calculation of 6.2 x 6.3 (leaf A: 14 382 000 cycles allocation-only, the range of the take rates
gives 13 931 000 to 14 832 000, plus a debt margin of up to +0.05 on the upper bound for what the prototype fails to earn
back, section 2.4); leaf B alone: 7 997 000 (7 819 000 to 8 235 000) and 6 694 000 with idioms. The free convention is
predicted worth **at most 0.01** on these leaves: **hypothesis** that leaf B is called a few times a frame, not once per
actor (it walks its 18 actors itself, `our_asm_on_real_games.md` l.82); a call costs about 226 master cycles
(`our_asm_on_real_games.md` l.37), so 0.01 of 10.2 M is about 450 calls in the window. **This contradicts R1's
prediction** (1.12 to 1.18 free against 1.20 to 1.25 kept, `G1_r1_result.md` l.137): R3-0 reads the call count in the
study copy and settles it before the column is built.

What it says, before any code:
1. **Allocation and selection alone do not reach 1.1 on either leaf**, and leaf B alone is **above the 1.2 stop** (1.31):
   if the stop reads that line, R3 is predicted to stop at R3-2 (as R1 predicted, l.139).
2. **With the idioms, both leaves are predicted within 1.1 to 1.12** (leaf A 1.12, leaf B alone 1.10, with caller 1.07):
   condition 1 is in reach only on that line, and G1's own share there is today + idioms (1.23 / 1.15 on leaf B) against the
   prototype (1.10 / 1.07).
3. Projection to the games (not a condition of R3; shares pre-XY, hypothesis): leaf A at 5.4 % of game 2's busy time
   (`our_asm_on_real_games.md` l.83) gives 5.4 x (1 - 1.15 / 1.288) = **0.6 %** of game 2; leaf B "most of 11.5 %" of
   game 1 gives 1.1 % allocation-only, 2.7 % with idioms. Condition 2 (10 % of game 2) stays R4's, and G1-M0 already showed
   the leaves alone cannot reach it (3.1 / 4.8 %, `G1_generator_prototype.md` l.97).
4. The twins (hypothesis: each reads like its leaf): `soa` 1.46 -> 1.31 allocation-only; `glide` 1.38 -> 1.22 (1.17 to
   1.28; no idiom); the held-out rows: none slower than today by more than 2 %.

## 7. Steps (each one sub-agent; each a prediction committed alone before its code, a stop rule, a lesson in the kit)

| step | what | stop rule | days |
|---|---|---|--:|
| **R3-0** paper and targets | frozen reference (commit and switches); a neutral twin of leaf A written for the bench (`path`: four 32-bit accumulators, sub-steps, a sine table in ROM) with its hand yardstick, checksum equal; the twins' block profiles; leaf B's call count in the study copy (by game 1's session); **a paper allocation**: the text the designed allocator would produce on the hot loops of `soa` and `path`, written by hand under its own rules only, measured in the bench; corpus grep for IRQ handlers using the direct page | paper `soa` and `path` both above 1.25 of the hand on the allocation-and-selection line: **stop before any code** (an early verdict for one day instead of six to nine) | 1 |
| **R3-1** skeleton | the hook, IR intake, selection by cost with every value in a slot (no register allocation yet), the side table, branch shortening, refusals with reasons, `QBE_G1` off byte-identical; gapbench checksums and `bench_abi` on the accepted rows; 2 000 seeds of each differential test; the fallback table. Prediction: the naive output on the accepted rows reads between R0's direct-page-off (3.76) and all-off (6.07) means: the prototype's own debt, written as a number | a gate red that the step cannot fix in its budget | 1.5 |
| **R3-2** allocator | the bank (A chains, X, Y, slots, DBR; D left for R3-5), the loop search, the cost model, the two lines on the twins and on the held-out rows (`copy fill col row box ents state probe sprites` held out; tuning on `soa glide path mobaos`) | a twin under half its predicted gain, or a held-out row slower than today by more than 2 % and not explained; and the stop of the owner's rule on the twins' line chosen in section 0 (above 1.2) | 2 |
| **R3-3** proof at scale | section 5 items 1 to 6 and 10: mutation on the prototype's text, six invariants, 20 000 seeds x 2 with the fallback rate, the paranoid build and its faults, width flow, control count | any gate red: no figure of R3-2 is read | 1 |
| **R3-4** the leaves, ABI kept | the games' study copies, by their sessions: the two leaves (leaf B alone and with its caller), both lines, both weightings (profile and static), state identical at every step, `jsl` / `rtl` contract on the trace | above 1.2 on the line chosen: stop, R3-6 says so | 1 |
| **R3-5** free convention and T1 combined | the free-convention column on leaf B with its caller (caller and callee both emitted by the prototype: the caller calls, so this step accepts calls for that one function); T1 combined as R1 named it (`mobfp`, `mobptr`, D on the entity, no `tcc__lf` in the body; R1 predicted 3 to 5 % on `mobptr` once the temps leave, l.97-98) | the column is reported whatever it reads; T1 dropped from G1 if under half its prediction combined | 1 |
| **R3-6** verdict | the table (two columns, two lines, both weightings, held-out, fallback), predicted against measured, to the conductor; the lesson to the kit | — | 0.5 |

**Cost: 8 days (range 6 to 9)**, against the plan's 4 to 6 (`G1_generator_prototype.md` l.193). The difference, said: the
paper step (1), the free-convention column, which needs the prototype to emit one function that calls (about 1), and T1's
combined measure that R1 sent to R3 (about 0.5). An early stop at R3-0 costs 1 day; at R3-2, 4.5 days.

**Risks** (each with where it shows): the cross-block knowledge of the text rules not re-earned (control count, held-out
rows; the L4 S3 precedent); compile time (lot XY measured +116 % before its fix, `G1_ir_probe.md` l.215-217: the loop search
is bounded by a value count and refuses beyond); the assertion build being wrong itself (lot XY's three defects); the study
copies depend on the games' sessions (two days of their time, R3-0 and R3-4); a moved D seen by an IRQ handler (R3-0 grep).

**Out of scope**: functions that call (R4), except leaf B's caller in R3-5; 8-bit width regions (T7), typed multiply (T4),
byte-store composition (T6), the constant-offset fold and the bank-carry candidates (R1 l.197-203): isolated passes of
today's generator; jump tables, volatile accesses, `__ramcode`, alloc / address-taken locals (the function is refused and
falls back); HiROM, SA-1, Super FX builds (LoROM SlowROM and FastROM only); inlining decisions; the library; anything on
`develop`; the cartridge confirmation (R5); the 10 % line (R4).

## 8. Five points where this design can be wrong (for the contradictor)

1. **The line decides the verdict, and the idioms may be credited to the wrong side.** Leaf B reads 1.31 without idioms and
   1.10 with (6.4): the stop is decided by the choice of section 0, not by the prototype. If the IR idioms are not written
   once for both emitters, the "with idioms" line credits G1 with 0.13 to 0.21 that today's generator would also take
   (`G1_r1_result.md` l.116-118 states the rule).
2. **The decompositions are dated and the attribution of what was gained since is a guess.** 6.2 assumes the 0.79 M (leaf A)
   and 1.88 M (leaf B) gained since 2026-11-11 were all counter / index work and none idiom. If a lot took part of the range
   test or the data-bank item, the "with idioms" line has less room and leaf B lands above 1.1. And the twins are not the
   leaves: leaf A has no twin yet, and R0 showed formulation moves cost unevenly (prediction (c) missed, R0 l.193).
3. **The A-chain bet.** 70-75 % of the "values" item rests on holding values in A by local reordering. The hand may have
   used reorderings across memory accesses or algebra a generator must not do; at 30 % taken, leaf A reads about 1.21 and
   the stop fires there too. R3-0's paper allocation is the cheap test of this point, and it is only as honest as the rules
   it is written under.
4. **The debt and the profile crutch.** The prototype starts near R0's 6.07 and must re-earn slot sharing, near addressing,
   rotation, byte-as-word; L4 S3 lost its gain to the text rules' cross-block knowledge it displaced. And the cost model
   uses measured block weights, which no shippable generator has: lot XY's static estimator chose wrong. The prototype's
   figure may be an upper bound of a real generator's by more than the gap between the lines.
5. **The free-convention column may measure nothing on the leaves.** 6.4 predicts at most 0.01 (few calls a frame), against
   R1's 0.07; if that holds, R3's two columns are equal and R3 says nothing about the convention, which G1-M0 and the hand
   rewrites (1.23 / 1.66, `our_asm_on_real_games.md` l.171-173) place in the functions that call, i.e. R4. Conversely, if
   leaf B is called once per actor, the column matters and the prediction of 6.4 is wrong on it.

## Contest (the strongest model, 2026-11-15, one task: where is this note wrong, too optimistic or untestable) and verdict

Call made under the 2026-11-14 rule: read-only, one question, four sources, ten objections at most. The answer was checked:
the arithmetic of objections 1 and 4 was replayed (leaf B, allocation fully taken: (8 828 590 - 1 187 000) / 6 107 176 =
1.251; leaf B with its caller: 1.2005; leaf A allocation-only: 14 382 000 / 12 707 660 = 1.132, not the 1.15 of the table in
6.4), and the quoted lines of 6.2, 6.3, 6.4 and of `G1_r0_result.md` (verdict 1 to 4: the plan keeps the text rules the
allocator does not subsume) were read. Verdict: **the note is not a base for R3-0 as it stands.** To correct before any code:

1. **Fix the cell the 1.2 stop reads, in writing, before R3-1**: leaf, perimeter (alone / with caller), line, static or
   profile weight, speed. Reading "allocation and selection" on leaf B alone cannot pass even with the whole allocation item
   taken (1.251); the contest recommends "with idioms", leaves A and B alone, static weight, with "today + the same IR idioms"
   beside it. The figure that judges G1 is the gap between those two columns, not the threshold. The decision is the
   conductor's: asked by thread.
2. Recompute 6.4 from 6.2 x 6.3 (leaf A 1.13 / 1.11, upper bound 1.174, 30 % of the values 1.164, none of "outside the loop"
   taken 1.222: that item, 57 % of what is left, is the real bet, taken at 40-70 % without a source).
3. The attribution of the gains since 2026-11-11 is a hypothesis that R3-0 must measure (per-item decomposition of both
   leaves at the frozen commit; is leaf B still a function in the reference build); `soa` reads 35 813 after the probe and
   36 377 in R0: explain.
4. The debt margin (+0.05 on A only, none on B) has no source; measure the loss per family on `soa` and `glide` alone.
5. The data bank is classed three ways; decide the side before R3-1 (today + idioms 1.23 becomes 1.269 without it).
6. The doors can pass without proving the prototype: re-run the mutation score on 3 to 5 other seeds, report the
   acceptance rate per mutated function, write a floor for the 20 000 seeds.
7. R3-0's stop is not testable (threshold 1.25 without a source, paper rules written by the same author the same day): freeze
   the paper rules before the text, apply them by a second agent.
8. Compare the cost model with luna on the 31 R0 rows, both speeds, tolerance written, before the search uses it.
9. NMI: a fixture where a prototype function runs in the main loop and in the NMI with D moved, plus a fault that must go red.
10. T1: write a per-row prediction (committed alone); the 3 to 5 % was "hypothesis, not measured".

Next: the conductor's answer on item 1, then a revision 2 of this note taking 2 to 10, then R3-0.

## Conductor's decision (2026-11-15, thread of the same date) and the margin committed before R3-1

1. **The cell the 1.2 stop reads**: the "with idioms" line, leaves A and B **alone**, **static weight**, at the ROM speed the two games build (the other speed reported beside it). "With its caller" and the profile weight are reported, they do not decide.
2. **No credit to G1 for what is not its own**: IR-rewrite idioms (range test, `cond ? k : 0`) are written **once**, as passes behind a switch, feeding both emitters; the column "today's generator + the same IR idioms" is measured on the same cell. An idiom that serves one emitter only is named and justified here.
3. **The verdict reads two figures**: (a) the cell under 1.2 (else the stop); (b) the gap between G1 and "today + idioms" on the same cell.
4. **Minimal margin for (b), committed before R3-1: 0.05 of ratio to the hand on each leaf.** Under it, G1 below 1.2 does not justify a new generator: the idioms go to today's generator and the report says so to the owner. Check against 6.4 (hypotheses): leaf B 1.10 vs 1.23 (1.269 without the data bank) is a gap of 0.13; leaf A has no IR idiom, today 1.288 vs 1.12 is 0.17. The margin is met by the prediction with room; it is the figure R3 must measure, not a target to tune. Unchanged by the contest; to revisit only on a fact.
5. Data bank (contest point 5): decided in revision 2 before R3-1. Revision 2 takes points 2 to 10, then R3-0 with a stop that is testable (paper rules frozen before the text, applied by a second agent). Sonnet executes, Opus reviews.

## Revision 2 (2026-11-15; Sonnet writes, Opus reviews): contest points 2 to 10

Supersedes sections 6.4 and 7 (R3-0, R3-2, R3-3, R3-5 rows) and the stop of R3-0. Everything else of the note stands. Still
hypotheses: nothing here is measured; the measurements are R3-0's, written below as tasks with their pass rule.

### R2.1 (point 2) 6.4 recomputed from 6.2 x 6.3, arithmetic replayed
Leaf A (today 16 367 410, hand 12 707 660): values 912 000 x 72.5 %, stack re-read 152 000 x 100 %, outside the loop
2 090 000 x 55 %, data bank 304 000 x 90 % (G1's own, R2.4). Allocation and selection (no data bank): **1.134** (low takes
1.174, high 1.096). With the data bank: **1.112** (low 1.153, high 1.075). Leaf A has no IR idiom, so "with idioms" equals
this line. Leaf A's real bet is the **outside-loop item** (57 % of what is left, 55 % taken, no source): with none of it
taken the line reads 1.224, with 30 % of the values only 1.164. R3-0's paper covers whole functions, not only hot loops, to
test exactly this item.
Leaf B alone (today 8 828 590, hand 6 107 176): allocation item 1 187 000 x 72.5 %, idioms 1 534 000 x 85 %, data bank
(9 / 156 of 4 601 198 = 265 000) x 90 %. Allocation and selection **1.266** (the stop would fire if the cell read this
line, as the contest said); with the data bank and the idioms **1.052** (low takes 1.138, high 0.98: read as "at the
hand"); today + the same IR idioms **1.232**. Gap (b): leaf A 0.18 (1.112 against 1.288), leaf B 0.18 (1.052 against 1.232);
margin 0.05 met with room. The earlier 1.31 / 1.10 / 1.23 of 6.4 were wrong by the arithmetic of the data bank (R2.4).
Debt margin: see R2.3. Free-convention column and "with caller": unchanged until R3-0 reads the call count (6.4's text).

### R2.2 (point 3) R3-0 measures the attribution, it does not assume it
At the frozen reference (commit and switches written in the first line of the result): per-item decomposition of both leaves
(values, counter / index, data bank, stack re-read, `clc`, range test, `cond ? k : 0`, outside the loop), by luna counts
per instruction, by the two games' sessions on their study copies. Questions it answers: is leaf B still one function in the
reference build (inlined or not: its perimeter changes); why `soa` reads 35 813 after the probe and 36 377 in R0 (same
source, same switches? name the difference: code, pin or harness). Pass rule: the sum of the items is within 3 % of the
measured gap; else R2.1's take rates are not read and the table is redone from the measured items.

### R2.3 (point 4) the debt margin gets a source
R3-0 measures the loss per family **on `soa` and `glide` alone** (R0's switch rows already exist: run them again at the
frozen commit, both speeds). Until then the margin is a stress case, not a point: +0.05 on **both** leaves (A 1.16, B 1.10
on the "with idioms" line), still under 1.2; if the measured loss on the leaves' shapes exceeds 0.05, the cell is redone
with the measured loss before R3-1.

### R2.4 (point 5) the data bank is G1's own, decided
DBR is a location of the bank, set by the allocator: it is not an IR rewrite and today's generator cannot take it as a
pass. It counts on the allocation-and-selection line of G1 (both lines) and **never** in "today + the same IR idioms".
Written before R3-1; revisit only on a fact (a pass that does it on today's generator, measured).

### R2.5 (point 6) the doors must be able to fail on the prototype
- Mutation: `mutate.py` re-run on **five seeds (1 to 5)**, on the prototype's text; each seed at least 90 % detected, and every
  surviving mutant read by hand and classed (equivalent, or a hole: a hole stops the step). Report per mutated function the
  share the prototype **accepted** (a refused function is mutated on today's text and proves nothing about G1).
- Floor for the 20 000 seeds: the count is of seeds whose functions the prototype **accepted**, not generated: 20 000
  accepted per differential test; and acceptance at least 50 % of the generated functions and 100 % of the hot loops of the
  twins and leaves. Under the floor, the 20 000 prove today's generator and no figure is read.

### R2.6 (point 7) R3-0's stop is testable
1. Before any paper text, the **paper rules are frozen in a file** (the allocation algorithm of 2.2, the cost unit of 3,
   the refusals), committed alone. 2. The paper text of `soa` and `path` is written by a **second agent** that gets only the
   file and the IR, not this note's predictions. 3. It is measured in the bench (same harness, both speeds), whole
   functions, with the IR idioms applied as the IR pass would. 4. **Stop before any code if the paper reads above 1.2 of the
   hand on the cell** (the owner's stop, same cell: "with idioms", alone, static weight); a paper above 1.12 is reported as
   a warning (the paper is a floor of what the prototype reaches: it cannot do better than its own rules). The 1.25 of the
   first draft had no source and is withdrawn.

### R2.7 (point 8) the cost model is checked against luna before the search uses it
R3-0 prices the 31 R0 rows with the model of section 3 and compares with luna, both speeds. Tolerance written before the
comparison, committed alone (hypothesis, no source): per row within 3 %, mean absolute error under 1 %, and the rank of the
rows' variants (switch on / off) the same. A row outside it: the model is corrected and the comparison re-run; the search
(R3-2) starts only after.

### R2.8 (point 9) NMI with D and DBR moved
Fixture `testing/fixtures/g1_nmi_dmove` (to write in R3-1): a main-loop function the prototype accepts, moving D and DBR
inside its loop, while an NMI handler increments a canary and writes plain RAM through its own direct page; checksum and
canary equal to the reference build. `templates/crt0.asm` NMI entry (checked 2026-11-15: `phd`, `phb`, D set to
`tcc__nmi_registers`, DBR to `$00` by `pea $0000 / plb / plb`) makes it safe by construction; the fixture checks that it
stays so. A fault that must go red: `QBE_G1_FAULT=dbr_nmi` (DBR left moved across the `wai` in the wait loop and an
absolute read of an NMI-written variable in the loop). R3-0 also greps the corpus for IRQ handlers using the direct page.

### R2.9 (point 10) T1 predicted per row, before R3-5
At R3-1's end, committed alone: a prediction per row (`mobfp`, `mobptr` and each other row with an entity in bank 0 below
`$2000`): saving in percent of the row's master cycles, and which temps leave the page. The 3 to 5 % of R1 stays a
hypothesis; T1 is dropped if the measure is under half the per-row prediction summed.

### R2.10 Steps after revision 2
R3-0 is now: R2.2, R2.3, R2.6 (rules frozen, second agent), R2.7, corpus grep of R2.8, the neutral twin of leaf A, the call
count of leaf B. R3-3 takes R2.5. R3-1 adds the NMI fixture; R3-5 reads R2.9. Cost: R3-0 1.5 days (was 1), the rest as
before, **about 8.5 days**. Next, in a fresh conversation: R3-0 (Sonnet executes, Opus reviews), starting with the paper
rules file and the tolerance file, each committed alone, then the measurements.
