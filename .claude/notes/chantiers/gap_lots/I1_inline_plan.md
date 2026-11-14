# I1: a size criterion for inlining `static` functions with several call sites (cut plan)

Plan written 2026-11-14, before any code. It follows the owner's go for B (the two ideas drawn from the comparison with
another 65816 compiler) and the I1 S0 result (`I1_s0_count.md`): the games' own static callees of at most 256 bytes,
marked `static inline` on a private copy of the gauge and recompiled, gave **-1.80 % (jeu 1) and -1.71 % (jeu 2)** of busy
time per step, equivalence identical at every step, **+47 KB and +146 KB** of code against 885 KB and 1 581 KB free. The count
had said 2.55 / 2.35 %. It overstated by a quarter (measured / count = 0.73 on both games).

Read for it: `I1_s0_count.md`, `A1_s0_count.md`, `.claude/rules/compiler.md`, `testing.md`, `compiler/ABI.md`,
`compiler/qbe/inline.c`, `compiler/qbe/main.c` (`emit_collected`, `addrtaken`), `compiler/cproc/attr.c` and `qbe.c:1778`
(the `inline` linkage hint), `devtools/gapbench/` (`kernels.c` rows `call`, `chain`, `fptr`), `devtools/sframe/i1_count.py`,
`devtools/compiler-tests/cases/auto_inline*.{c,checks}`.

Nothing here is measured after a change. Every figure is a reading of today's tree, a measurement made for this plan on
today's compiler (marked **read 2026-11-14**), a prediction, or a hypothesis, and says which. The names are neutral:
jeu 1 / jeu 2, the games, another 65816 compiler.

Pair: this is a Class A change that touches what every unit emits. Complex: Opus writes each code step in its own worktree
(`git checkout -B work develop` first), Fable contests this plan once before S1 and reviews the closure. The session runs
every gate again before a merge. The games build on this tree: each step that changes `bin/` is announced before and after
(`partners.md`).

## 0. What decides inlining today (read 2026-11-14, `develop` `a1aee808`)

- `emit_collected` (`main.c`) counts the direct call sites of each function over the whole unit (`inline_count_sites`, before
  any inlining) and the names whose address is taken (`addrtaken_scan`). It then runs `inline_check` on every function in
  source order, runs the address scan again, and emits every function except the absorbed ones (`inline_absorbed`) and the
  fully consumed `inline` ones (`inline_fully_consumed`).
- `inline_check(fn)` walks the caller's calls in block order. For a callee that the small inliner does not take, it asks
  `auto_inline_ok`, runs `inline_check(callee)` first (bottom-up: the body cloned is final, nested to any depth), asks
  `auto_inline_ok` **again** (the callee may have grown), then calls `splice_fn`. A callee on the current chain (`busy`)
  is declined, so a cycle is never entered.
- `auto_inline_ok`: the callee is static, its address is not taken, it is not the caller, it has no section of its own, and
  **either `sites == 1` or (`inline` keyword and `ins_count <= 160`)**. It is not variadic, has no dynamic alloc, and holds
  no alloc, no struct parameter or return, no `Jretc/s/d`. Finally **`caller->ntmp + callee->ntmp < 1800`**.
  `rec->ins_count` is the size **before** the callee's own inlining (counted in `inline_record`, `Odbgloc` excluded).
- `splice_fn` names each cloned block `inl%d_%.22s.%d.%.40s` (splice number, callee name, block index, block name) and the
  continuation `inl%d_%.22s.ret`. A static function's name carries its unit (`name.unit`), and the block name of a block
  that is already a clone is the inner clone's label. So the 40-character cut can fall right after a `.b`, `.w` or `.l`.
  WLA then reads that as a size suffix of the operand and the link fails: S0 met it three levels deep.
- `__attribute__((noinline))` is **silently ignored** today: cproc skips any attribute that is not in its refused list
  (`attr.c`), and nothing reaches QBE. The repository has no use of it (one comment in
  `testing/fixtures/stress/race_sweep/main.c`).
- Eight compiler checks keep a static function as a function by giving it two call sites (`auto_inline`,
  `auto_inline_leaf_rule`, `loop_x_counter_home`, `mul_kl_const_narrow`, `peep_late`, `ramcode_not_inlined`,
  `test_acache_pha`, `test_variable_shift_bug`; also the fixture `d_quals`): `compiler.md` gives that as the way to do it.
  The size rule breaks the trick.
- gapbench: `k_call_f` and the `chain` callees (`ch_test`, `ch_clamp`, ...) are **global**: I1 as scoped does not move
  `call` or `chain` (`I1_s0_count.md`, variant W). `fp_index` (static, two sites, row `fptr`) and `sum` (static, many
  sites, the checksums) are the only static helpers with several sites.

### Calibration read for this plan (read 2026-11-14, today's `bin/`, nothing committed)

The library's 26 units and 78 corpus units were compiled into `/tmp` with `CC_TRACE_INLINE=1`. For each function that
absorbed nothing, the `ins=` of its `record` line (the compiler's own measure) was set against its emitted bytes (the
estimate of `i1_count.py`). Over 472 functions: **3.24 bytes per IR instruction** (median; quartiles 2.45 and 4.07). As
a separator of "at most 256 bytes", `ins <= N` gives:

| N | ≤ 256 B taken | > 256 B taken (wrong) | ≤ 256 B missed |
|--:|--:|--:|--:|
| 50 | 339 | 1 | 59 |
| 60 | 359 | 1 | 39 |
| 70 | 380 | 4 | 18 |
| **72** | **382** | **5** | **16** |
| 80 | 387 | 9 | 11 |

(The script was throwaway and is deleted; step S2 keeps a real one, `i1_calib.py`.) The same reading
estimates the corpus cost of the rule at N = 72, **one level, on today's text: about +27 KB over 123 callees**. The largest
units are `backgrounds/mode0` +3.6 KB, `windows/window_multi_hdma` +3.3 KB, the library's `audio.c` +3.2 KB (linked by
every audio example), `backgrounds/mode1` +2.9 KB and `input/mouse` +2.0 KB. This is a hypothesis for S4, not a
measurement: the figure is not from recompiled text.

## 1. The criterion

### 1.1 Who is a candidate (the "size rule")

A call to `g` from `f` in the same unit is copied by the size rule when all of these hold:

1. `g` passes every condition of `auto_inline_ok` except the `sites` / `inline` one (static, address never taken, not `f`,
   not on the chain, no section of its own, no alloc, not variadic, no struct, the temporary room).
2. `g` has **at least 2 direct sites** in the unit (one site stays today's rule, which costs no ROM).
3. `g` is **not `noinline`** (1.4) and carries no `inline` keyword (those keep today's rule, 1.4).
4. **Own size** `own(g) <= N_SIZE`, IR instructions counted as `inline_record` counts them (`Odbgloc` excluded), **before**
   `g`'s own inlining. **`N_SIZE = 72` by default** (`CC_INLINE_SIZE=N`), from the calibration above (about 256 bytes);
   S2 recalibrates it on the games. Own size and not final size, because own size is what S0 measured. The nested
   growth is priced by the budget (1.3), not by the criterion. Hypothesis H5 (section 7) says what is lost by using the
   final size instead.
5. `f` has **no section of its own** (`__ramcode`): a copy into the RAM code window grows a window the program sized. The
   one-site rule keeps today's behaviour there (a body absorbed into a RAM caller runs from RAM).
6. **Free copies**: a `g` with `own(g) <= 6` (no more code than its call sequence: `pea`/`pha` + `jsl` + cleanup is 8 to 12
   bytes) is always taken and does not count against the budgets. This is a hypothesis about the threshold, checked in S2
   on the bytes.

### 1.2 Order: today's rules first, then the size rule

`inline_check` becomes two walks per caller. **Walk 1** is today's walk, unchanged: the small inliner, the one-site rule and
`static inline`. **Walk 2** runs only when the size rule is on, on the result of walk 1: the admitted size-rule candidates,
under the room the first walk left. Two reasons:

- A one-site absorption costs no ROM and gains more per call than a copy. Under one walk, a copy met earlier in the
  caller's block order would take the temporary room that a one-site callee met later needs. S0 already sees
  0.81 / 0.75 % lost in declined one-site callees (section 3).
- With `QBE_NO_INLINE_SIZE=1`, walk 2 does not run and **the output is byte-identical to the step before**. That is the
  switch's gate.

Bottom-up stays: walk 2 on `f` runs walk 1 and walk 2 on each admitted `g` first (`checked` covers both walks), so a copy of
`g` already holds its own copies.

### 1.3 Budgets: per caller, per unit, and what happens beyond

The compiler sees one unit and does not know the ROM's free space. So the bound is per unit and per function, in estimated
bytes, `est(x) = 3.24 × IR instructions`. The factor is recalibrated in S2, and `CC_INLINE_BPI` holds it for a measurement.

- **Planning pass, before any splice** (new, `inline_plan` after `inline_count_sites`): over the unit's call graph restricted
  to the candidates (acyclic: a cycle is never a candidate), compute bottom-up
  `final(g) = own(g) + Σ_{admitted h called in g} sites_g(h) × (final(h) − CALLSEQ)` and the cost of admitting `g`:
  `cost(g) = (sites(g) − 1) × final(g) − sites(g) × CALLSEQ` (the standalone goes when every site is copied). Admit in
  priority order (1.5) while the budgets hold. Walk 2 copies only the admitted set. **Decisions do not depend on the
  order the functions appear in**: that is a property the planning pass gives and one walk would not.
- **Per unit**: `Σ cost <= UNIT_BUDGET`, by default **32 768 estimated bytes** (one LoROM bank; about 10 100 IR
  instructions at 3.24; `CC_INLINE_UNIT_BUDGET=N`). Hypothesis H3: the budget does not bind on either game at the
  default. S0's +146 KB on jeu 2 is spread over its units, and S2 reads the per-unit growth from the `i1-256b` objects.
  If one unit of a game needs more, S2 says by how much and what the gain beyond is worth, and the default moves on that
  measurement only.
- **Per caller**: the size rule adds at most **8 192 estimated bytes to one caller** (about 2 500 IR instructions), and a
  caller's final estimate stays **under 16 384 bytes**. Every function is a `SUPERFREE` section that must fit one bank
  (32 KB in LoROM): a caller grown past a bank fails the link, and long callers also lose the short branches
  (`relax_branches`, 124 bytes). The temporary room (1 800) stays as it is, and walk 2 asks it per splice as today.
- **Beyond a budget**: the candidate is not admitted. Its calls stay `jsl` and its standalone stays. No warning (the build
  has zero warnings, and a full budget is a choice, not a fault). `CC_TRACE_INLINE=1` prints one line per refusal with its
  reason (`size`, `unit budget`, `caller budget`, `tmp room`, `ramcode caller`, `noinline`). The game keeps the policy
  (`partners.md`, check 3): the SDK gives the mechanism (`noinline`, `static inline`, the budget as a number), not a
  ceiling that only it can move.
- **The standalone**: a plain static copied at every site is not emitted. Walk 2 counts direct references as the `inline`
  path does (`n_direct`, `n_declined`). After both walks, a reference scan over the functions that will be emitted
  (repeated to a fixed point, since dropping a standalone removes its own calls) keeps it only when a call remains or its
  address is taken. A wrong keep costs nothing in the ROM, because `wlalink -d` drops an unreferenced section. A wrong drop
  fails the link loudly.

### 1.4 The programmer keeps the hand

- **`noinline`**: `__attribute__((noinline))` and `[[gnu::noinline]]` reach QBE as a linkage keyword (`noinline`, beside
  `inline` in `cproc/qbe.c` and `parse.c`). Every inliner refuses it: the small one, the one-site rule and the size rule
  (GCC's meaning). Today the attribute is silently skipped. Making it work changes an existing program that already
  writes it on a one-site static: S3 counts the uses in the games privately before it lands.
- **`static inline`** keeps today's rule (copied at every site up to 160 IR, `CC_INLINE_MAX_BIG`). It is admitted before
  the size rule in the planning pass and counted in the budgets, but **never refused by them**: the programmer asked.
- **`always_inline`** is not in this lot (a non-goal: `static inline` already says it, under a limit that can be set).
- The switch `QBE_NO_INLINE_SIZE=1` turns the size rule off for one build. A user-facing knob (a `make/common.mk`
  `?=` variable) is a separate decision: it is a second audience (`two_audiences.md`), and anchor 12 of
  `doc_consistency.md` would need its line on `docs/tools/build.md`.

### 1.5 Priority, without a profile

The profile's order (gain per byte) is not knowable to a compiler. The static score of a candidate `g` is
`score(g) = Σ_sites 8^depth(site) / cost(g)`, where `depth` is the loop depth of the site's block in its caller (QBE's
`loopiter` on the caller's CFG after `filldom`, already run in `func`). A ratio is ordered smallest cost first. Ties go
by name, which keeps it deterministic.

What it loses, said before: (a) the caller's own frequency (a site in a loop of a function called once at boot outranks a
straight site in the per-frame update); (b) branch bias (a site behind a rare `if` in a loop counts as hot); (c)
frequency across units (a helper called from a hot loop of another file). **It only matters when a budget binds.** If H3
holds, it changes nothing on the games, and the corpus decides whether it is worth more than size order (S4 trace: how
many candidates a budget refused, per unit).

## 2. First step, before any criterion: the cut label

**Defect** (S0, `I1_s0_count.md`): `snprintf(c->name, NString, "inl%d_%.22s.%d.%.40s", ...)`. The cut of the block name
(40 characters) can end in `.b`, `.w` or `.l`, upper or lower case (static names are `name.unit`, nested clones put a whole
label in the block name), and WLA reads the operand `@...x.l` as a label plus a size suffix. The name part (`%.22s`) is
followed by `.%d`, so its cut is never last. The continuation `.ret` is safe.

**Fix**: a function `inl_label(char *dst, int seq, const char *fn, int idx, const char *blk)` builds both labels. When the
result ends in `.` followed by one letter of `bBwWlL`, it appends `_`. Every label that does not end that way is
unchanged, so **the corpus's `.c.asm` stay byte-identical** (prediction below). Second guard, in the emitter: any label
the backend prints (`@name:` and every `@name` operand from a block name) that ends in `.[bwlBWL]` is an internal
compiler error naming the function. That catches a cproc block name of the same shape, which the fix does not touch.

**Test** (runtime fixture, since the failure is at link time, which `devtools/compiler-tests` does not reach):
`testing/fixtures/compiler/inline_label_cut/`. One unit holds three nested static helpers copied at two sites each
(`static inline`, so the step does not depend on the size rule). Their names and the unit name are chosen so that, at
each of the three levels, the 40-character cut falls right after a `.b` (one level), a `.w` (one) and a `.l` (one). It
holds values against a host model and runs the same code in the main loop and in the NMI callback, like `leaf_frame`.
A `.checks` case (`inline_label_cut`) pins that the three repaired labels appear with their `_`. If a later change to
`inl_seq` moves the cut, that check fails loudly instead of the fixture passing for the wrong reason.

**Injected fault**: `QBE_INLINE_FAULT=label` skips the repair. The fixture must then **fail to link**, with the WLA message
of S0 ("Reference to an unknown label"), and the emitter guard must stop first when it is on. Both are run once in the
step and their output is quoted in its note.

**Prediction (written before)**: no `.c.asm` of the library or of the corpus changes (`cmp` over `lib/build/*/` and every
example's objects); `.sfc` byte-identical; both games' ROMs `cmp`-identical to their reference (nothing of theirs nests
deep enough today, S0). If one file changes, that is a cut the corpus already had: it is named.

**Gates**: `make clean && make`, `make test-compiler`, the new fixture, `make tests`, `diff_corpus.py --ref` (every MATCH),
`difftest.py --seeds 1-2000`, `difftest_stmt.py --seeds 1-2000`. Lesson: written in the tree's lessons folder (the porting
kit's template) and linked from the step's note. It covers how a cut that is safe for uniqueness became unsafe for the
assembler, why the corpus never showed it (no nesting three deep), and what prevents it now (the emitter guard on every
label).

## 3. The temporary room and the one-site callees that are already declined

S0 found 10 (jeu 1) and 101 (jeu 2) one-site eligible callees that today's splicer declines. They hold **0.81 / 0.75 %**
of busy time (count, lower bound, inside the 2.55 / 2.35 %). On jeu 2's object unit, its two big callers absorb 17 and
decline 18, absorb 10 and decline 90. The cause is a hypothesis (H2): `caller->ntmp + callee->ntmp >= 1800`, reached as a
caller grows with each absorption. Other causes the source allows: `nargs != npar` in `splice_fn` (a call with fewer
`Oarg` than parameters), the second `auto_inline_ok` after the callee's own inlining, `busy`.

**The measurement that settles it (S2, no change to the output)**: `CC_TRACE_INLINE=1` prints the reason of each decline of
`auto_inline_ok` and `splice_fn`, with `caller->ntmp`, `callee->ntmp` and the number of temporaries **still used** in the
caller (defined or read by an instruction or a phi; hypothesis H8: most of `ntmp` is dead after the copies and SSA). It is
run on jeu 2's object unit and on jeu 1's unit that holds its declined callee, built privately from the frozen
reference. **Decided by**: the share of the 111 declines whose reason is `tmp room` (H2 holds if ≥ 80 %), and the ratio
of used to counted temporaries in the two big callers.

**What follows from it**, chosen in S2's note, not now:

- H2 holds and H8 holds (used ≤ 60 % of `ntmp`): step **S5** compacts the caller's temporaries before the room test (one
  renumbering pass over the caller after each splice, or once before walk 2). It recovers room without raising the
  2 048 limit of the back end.
- H2 holds and H8 does not: the room is real. S5 measures what raising `MAX_ALLOC_TEMPS` (a back-end table size; build
  time and colouring cost) or splitting would give, against the 0.75 % ceiling. It goes ahead only if the predicted
  recompiled gain is ≥ 0.3 % on one game.
- H2 does not hold: the real cause is named and becomes S5's object, or the lot leaves it.

**The interaction with the size rule** comes from 1.2: walk 2 runs after walk 1, so a copy never takes room from a one-site
absorption. Copies into the big callers will mostly be refused by `tmp room` (the same callers). S4's trace counts those,
and S5 gives them back if it happens.

## 4. Safety: what a copy changes

- **Frames.** A copy merges the callee's temporaries into the caller's colouring: no frame carve for the callee, and the
  caller's frame grows by what does not share a slot. The stack depth at the caller's other calls grows by that amount
  (not measured by S0). A caller that loses its last call becomes a leaf (`fn->leaf = 1` at the end of `inline_check`)
  and uses `tcc__lf`, with `leaf_split` beyond 20 words. Nothing new: the one-site rule already makes these shapes. What
  is new is the number of copies. Gate: the stack floor of `rom_coverage.py` on the corpus, and the lowest stack pointer
  over the gauge windows on the games (section 6).
- **L4 S1's call table.** `sframe_dump` writes `; sframe call` from the final IR at emission, so a copy's own calls are
  table lines of the caller, and a standalone that is not emitted has no `; sframe fn` line. One edge needs a test: a
  callee that ended in a tail call (`jml`) has it as a `jsl` inside the copy (no longer in tail position). The emitter
  decides that, so the table follows. **Gate**: `make test-sframe` (`opensnes-rom check --objects` on the corpus) after
  S4, and on both games (`--objects` on their units). **Fault**: `QBE_SFRAME_FAULT=noedge` must still turn it red.
- **NMI.** A C function reached from the NMI (a callback) may absorb copies. Its direct page is the NMI's own
  (`tcc__nmi_registers` mirrors `tcc__lf`), and its time is measured by `nmi_budget.py` (12 000 mclk ceiling). Gate:
  `nmi_budget.py`, and the new label fixture runs its code in the NMI too.
- **Recursion.** A candidate is never in a cycle: the planning pass works on the candidates' graph, Tarjan on the unit's
  direct calls (as `i1_count.py` does), and `busy` still refuses at splice time.
- **Own section, `__ramcode`.** A callee with `lnk.sec` is never copied (today's rule, check `ramcode_not_inlined`). A caller
  with `lnk.sec` gets no size-rule copy (1.1-5). A new check case is needed: a `__ramcode` caller with two calls to a small
  static keeps both `jsl`, and the static stays in ROM.
- **Jump tables (L2).** A callee with a dense `switch` copied k times gives k regions, so k tables in the caller's section,
  labelled by `jt_seq` (unique in the unit). The region rules (members entered from the region alone) hold for a clone,
  since the clone is the same graph. Cost: each table counts its slots, which the IR size does not see (the tree's compares
  are in `own`), so the byte estimate is low for such callees (hypothesis, S2 reads it). **Gate**: a new case in the
  `jump_table` fixture, a static switch helper called from two sites, values against the host model; `QBE_JT_TRACE=1`
  says each copy took a table.
- **X/Y colour, far pointers, text rules.** These run per emitted function on the result. The copies create no new
  invariant, only more of the shapes the one-site rule already makes. The proof is the differential hunt (S4), not the
  argument.
- **Volatile, order of evaluation.** `splice_fn` keeps the `Oarg` values computed before the jump into the clone, as the
  call did. A `volatile` access inside the callee stays one access per execution.
- **Library.** The library's units go through the same compiler: its 21 eligible static callees (S0) are copied when they
  pass the rule. Every example that links them changes bytes: the corpus A/B covers it. The four object sets (LoROM, HiROM,
  SA-1, Super FX) are four builds of the same C, so `make lib` checks all four.

## 5. Steps (each the size of one sub-agent)

Every step writes its prediction in its note and commits it **before** it runs. A prediction about code generation is
made on **recompiled** text (three counts were wrong at execution: LF, M5, L4 S3). A step that changes `bin/` is announced
to the games before and after. The common gates of a code step are: `make clean && make` (zero warnings), `make tests`,
`make test-compiler`, `make test-sanitizers` (after any compiler change), `diff_corpus.py --ref <before>`,
`difftest.py --seeds 1-2000` and `difftest_stmt.py --seeds 1-2000` (10 000 each for S4), and `make lint`.

### S1: the cut label (section 2)

- Object: `inl_label`, the emitter guard, the fixture, the check, the fault.
- Files: `compiler/qbe/inline.c`, `compiler/qbe/w65816/emit.c` (guard), `testing/fixtures/compiler/inline_label_cut/`,
  `devtools/compiler-tests/cases/inline_label_cut.{c,checks}`, `.claude/rules/compiler.md` (one sentence in the inlining
  entry), CHANGELOG.
- Prediction: every `.c.asm` and `.sfc` of the library and the corpus byte-identical. The games' ROMs `cmp`-identical to
  the reference.
- Proof: the fixture passes; it fails to link under `QBE_INLINE_FAULT=label` (output quoted); the common gates pass.
- Switch: none needed (byte-identical); the fault is the provoked failure.

### S2: measure before deciding (no change to the output)

- Object: (a) `CC_TRACE_INLINE` prints decline reasons and used against counted temporaries (section 3); (b) a maintainer
  script `devtools/sframe/i1_calib.py`. It reads the trace's `record` lines and the `.c.asm` and gives bytes per IR
  instruction and the separator table of section 0, on the corpus, the library and (privately) both games' sets of 146 /
  181 callees; it also prints the free-copy threshold (1.1-6) and the error for callees holding a `switch`; (c) privately,
  per unit of both games, the code growth from `i1-temoin` to `i1-256b` (`.sym` sections by unit); (d) privately, the count
  (`i1_count.py --set`) of the set an IR criterion picks, against the 256-byte set. The count is used only as a **ratio
  between two sets** of the same count, so the 0.73 measured/count of S0 carries.
- Files: `compiler/qbe/inline.c` (trace only), `devtools/sframe/i1_calib.py`, `Makefile` (`count-i1-calib`), the note
  `I1_s2_measure.md`.
- Prediction: output byte-identical (trace on stderr). H2 holds for ≥ 80 % of the 111 declines. Bytes per IR instruction on
  the games' callees in 2.8 to 3.8. `N_SIZE` for 256 bytes on the games in 60 to 90. The IR-picked set holds ≥ 90 % of
  the 256-byte set's count. No unit of either game grows by more than 32 KB at `i1-256b` (H3).
- Proof: `cmp` of the corpus objects; the trace's counts sum to S0's 10 / 101 one-site declines.
- Output: the values of `N_SIZE`, the factor, the budgets and S5's branch, written in the note **before S4 starts**.
  Any default of section 1 that the measurement contradicts is changed there, with the figure.

### S3: `noinline`, the checks that need a function to stay, the gapbench twins

- Object: (a) `noinline` from cproc to QBE (linkage keyword), refused by every inliner; (b) the eight checks and `d_quals`
  that rely on two call sites get `__attribute__((noinline))` on the function they need to stay, so the size rule cannot
  empty them silently; `compiler.md`'s sentence ("gives it two call sites") becomes "marks it `noinline`"; (c) a check
  `noinline` (a one-site static marked `noinline` stays a function; the same unmarked is absorbed) and a negative check
  that the attribute on an object is refused or ignored as GCC does; (d) the gapbench twin rows `callst` and `chainst`:
  the `call` and `chain` workloads with **static** callees, each called from two places in the unit, so that the one-site
  rule does not take them and today's emission is the baseline. These rows are what S4 must move.
- Files: `compiler/cproc/{attr.c,decl.c,qbe.c}`, `compiler/qbe/{parse.c,all.h,inline.c}`, the eight cases and their
  `.checks`, `testing/fixtures/compiler/d_quals/main.c`, `devtools/gapbench/{kernels.c,kernels.asm,baseline.json,README.md}`
  (`--update` for the new rows only), `docs/BENCHMARK.md` if the generated table lists rows (anchor 13 and 22).
- Prediction: every `.c.asm` of the library and the corpus byte-identical (no code writes `noinline`; the checks' output is
  unchanged while two sites still block). `callst` / `chainst` read about the same as `call` / `chain` today (the same code
  with a `static`, so within 2 %, since a static leaves out its `rep #$20`). Privately: the number of `noinline` in the
  games, expected 0 (H9). If not 0, the absorptions that would stop are listed before landing.
- Proof: the checks pass; with `noinline` stripped out by a fault (`QBE_INLINE_FAULT=noinline`), the new `noinline` check
  fails. Common gates.
- Switch: none for (a): an attribute that the user writes means what it says.

### S4: the size rule

- Object: `inline_plan` (candidates, `own`, `final`, `cost`, score, admission under the budgets), walk 2 in
  `inline_check`, the standalone scan, the decline reasons, `QBE_NO_INLINE_SIZE=1`, `CC_INLINE_SIZE`,
  `CC_INLINE_UNIT_BUDGET`, `CC_INLINE_CALLER_BUDGET`, `CC_INLINE_BPI`. `difftest_stmt.py`: a new seed class (8k+5) in which
  every helper is plain `static`, called from **two to four places**, at least one site inside a loop, one helper calling
  another (nested copies two deep), and a helper with a dense `switch` one time in four. `difftest.py`: in the `f` shape of
  odd seeds, the test function is `static` and called twice (from two call sites that are each checked).
- Files: `compiler/qbe/inline.c`, `compiler/qbe/main.c` (planning call, standalone scan), `testing/difftest_stmt.py`,
  `testing/difftest.py`, new checks `inline_size` (a 3-site static of `N_SIZE` instructions copied and gone; one of
  `N_SIZE + 1` kept; the unit budget forced small by `env CC_INLINE_UNIT_BUDGET=64` refuses by score; a `__ramcode` caller
  refused) and `inline_size_off` (`QBE_NO_INLINE_SIZE=1`, the same source, every `jsl` there), the `jump_table` fixture
  case (section 4), `.claude/rules/compiler.md`, `KNOWN_LIMITATIONS.md` if a limit is user-visible, CHANGELOG, the
  baselines (`wram_regress.py --update`, `luna_runner.py` baselines, `audio.json` with the `luna diff --audio` quote,
  `bench_baseline.json`) where they move, each with why.
- Prediction (on recompiled text, from S0's measured 1.80 / 1.71 % scaled by S2's ratio r of the two sets; written here with
  r = 0.95, rewritten in S4's note with S2's r before the run): **gauge, busy per step, -1.7 % (jeu 1) and -1.6 % (jeu 2),
  range -1.3 to -2.1**, equivalence identical at every step. To that is added, as a hypothesis: the library's copies
  (+0.0 to +0.1) and walk 1 before walk 2 (+0.0 to +0.2). Code +40 to +55 KB (jeu 1) and +120 to +160 KB (jeu 2). Corpus:
  +20 to +35 KB over all ROMs, no example changes its number of banks, every `diff_corpus` MATCH (a boot offset allowed
  and explained). gapbench: `callst` and `chainst` faster by at least 30 % of the gap between today's figure and the
  `call` row's assembly twin (hypothesis), `fptr` faster or equal, no row slower. The hunts: 0 failures in 10 000 seeds
  each.
- Proof: the gauge `--compare` against the S3 build (private), the equivalence; every common gate; `make test-sframe` and
  `--objects` on the games; `nmi_budget.py`.
- Switch: `QBE_NO_INLINE_SIZE=1`, **byte-identical to S3** on the corpus, the library and both games (gate).
- Injected faults: `QBE_INLINE_FAULT=standalone` (drop the standalone even when a call remains) must fail the link of the
  check `inline_size`; `QBE_INLINE_FAULT=order` (walk 2 before walk 1) must make S2's one-site decline count on jeu 2 go
  up (the trace shows it) and a new `.checks` line red (a one-site callee after a copy in a caller built near the room).

### S5 (conditional on S2): give the room back

- Object: as section 3 decides (temporary compaction, or a measured raise of the limit, or the named cause).
- Prediction: written in S2's note with its figure. Ceiling 0.81 / 0.75 % (count), so about 0.6 / 0.55 % at the 0.73
  ratio, minus what S4 already took.
- Proof, switch (`QBE_NO_TMP_COMPACT=1`, byte-identical), fault: as S4. It goes ahead only if its prediction is ≥ 0.3 % on
  one game.

### S6: closure

- Fable reviews the closure. The note `I1_result.md` gives figures against predictions. `STRUCTURAL_DEFECTS.md` is
  updated, the CHANGELOG entry written, the lesson linked. The weekly comparison with the other compiler gets the
  `callst` / `chainst` lines.

### The stop rule (written before S1)

- **The lot lands** if, at S4 (or S5), the gauge gives **at least 60 % of 1.7 %, so ≥ 1.02 % of busy time per step on at
  least one game**, with equivalence identical on both, **and** the code stays within the budget: (a) no unit of the
  corpus, library or games exceeds `UNIT_BUDGET` estimated bytes, measured from the objects; (b) each game's code growth
  is ≤ 1.2 × S0's measured growth (+57 KB / +175 KB) and ≤ 15 % of its free ROM; (c) no example exceeds its ROM size or
  changes its number of banks.
- **Under 1.02 % on both games**: the size rule leaves `develop` (the commit is reverted, not left off behind a switch).
  S1 and S3 stay (they are fixes and tools of their own), and the figures go to the conductor.
- **Over budget with the gain**: the step lowers `N_SIZE` or the budget and measures again **once**. A second failure
  stops the lot with the figures.
- Any equivalence difference, hunt failure or broken gate stops the step until its root cause is fixed (`debugging.md`).

## 6. Side effects to measure (S4 and S5)

| What | How | Limit |
|---|---|---|
| ROM size per example | `opensnes-rom budget --json` before and after, every example | none exceeds its declared ROM; no change of bank count; list the ten largest growths |
| Bank $00 | `opensnes-rom check` | no pinned section refused (code moves out by itself) |
| Library objects | `.sym` sections per module, LoROM | growth listed per module |
| Compile time | `make clean && make` wall time of the corpus, three runs; QBE time on the largest unit of each game (private) | ≤ +20 % on the corpus and on each game's build (hypothesis threshold, said as such) |
| Maximum stack depth | `rom_coverage.py` stack floor (corpus); lowest SP over the gauge windows (games, luna) | stack floor gate green; the games' lowest SP stated and above their stack's reserved floor |
| NMI time | `nmi_budget.py` | under 12 000 mclk, worst frame stated |
| gapbench | `run.py --check`, both FastROM and SlowROM | no row slower; `callst`, `chainst`, `fptr` stated |
| Benchmark page | `make bench` | `bench_baseline.json` and `docs/BENCHMARK.md` updated together if they move (anchors 13, 22) |
| Boot length | `diff_corpus.py` boot offsets | each offset explained (the 2026-10-10 `mode7/extbg` precedent: a big fill inlined into `main` booted four frames later) |
| Late frames (games) | the gauge | stated; not worse than S0's 147 / 419 by more than 5 |

## 7. What I am not sure of (hypotheses, each with the measurement that settles it)

- **H1**: 72 IR instructions separate "at most 256 bytes" on the games as on the corpus (3.24 bytes per IR instruction).
  Settled by S2 (b) on the games' 146 / 181 callees.
- **H2**: the 111 one-site declines are the temporary room. Settled by S2 (a), the decline reasons.
- **H3**: a 32 KB unit budget and 8 KB per caller do not bind on either game at `N_SIZE`. Settled by S2 (c), per-unit
  growth of `i1-256b`, then S4's trace (refusals by `unit budget` / `caller budget`).
- **H4**: running the size rule after today's walk gives back part of the 0.81 / 0.75 % rather than nothing. Settled by S4's
  trace (one-site declines before and after) and by the `order` fault.
- **H5**: own size (before nesting) loses nothing against final size; final size would refuse nested callees whose copies
  already hold copies. Settled by S2 (d): the count of both sets.
- **H6**: the measured/count ratio 0.73 holds for the IR-picked set as for the 256-byte set. Settled by S4's gauge only.
- **H7**: the static score (loop depth / cost) is no worse than size order where a budget binds. Settled by S4 on the units
  where the trace shows a refusal; if none binds, it is not settled and that is said.
- **H8**: most of a big caller's `ntmp` is dead (copy and SSA leftovers), so compaction would give room back. Settled by S2
  (a), used against counted.
- **H9**: neither game writes `noinline` today. Settled by S3, a private grep of both games' sources.
- **H10**: the corpus grows about +27 KB at `N_SIZE = 72` (today's text, one level, throwaway script). Settled by S4's
  recompiled ROMs.
- **H11**: the free-copy threshold (`own <= 6` costs no bytes) and the byte estimate of callees with a `switch` (tables
  unseen by the IR count). Settled by S2 (b).
- **H12**: `callst` / `chainst` move by at least 30 % of their gap. Settled by S4's gapbench (the other compiler's 0.41 on
  `call` also passes arguments in registers, which this lot does not do: A1 stopped at S0).

## Contest (the strongest model, 2026-11-14, one task: where can the S4 size rule build a wrong program silently), checked against `inline.c`, and what is taken

Checked in the code before keeping anything: `splice_fn` guards only `nargs != npar` (`inline.c:600`); a walk rewrites the block (`blk->ins = head; blk->nins = nhead`, `inline.c:753-754`); `rec->absorbed = 1` is set after the first successful copy (`inline.c:850`); `inline_check` returns early on `checked || busy` (`inline.c:824`). Taken as requirements of S4, in this order of severity:
1. **A planned site goes stale (severe, silent)**: the plan must never keep (block, index) pairs across a walk. The decision is keyed by the call's identity, re-found by callee name at splice time, and `splice_fn` asserts the instruction at `call_idx` calls `callee` (an internal error otherwise). Fixture: one caller with, in the same block, a single-site `static` then two `static` of the same arity with two sites each, values against the host model; fault `QBE_INLINE_FAULT=stale` shifts the index by one and must turn red.
2. **A function dropped while a `jsl` to it remains (not always loud at link)**: `absorbed` is set only when every site was copied and no reference remains (a scan of the final IR of the unit, not a counter fixed before the copies); `sites` is recomputed after the copies. Fixture, linked and with values: two units with the same file stem and a `static` of the same name (the decorated name is `name.<stem>`: the orphaned `jsl` would bind to the other one), one copied at a site and declined at another by a `__ramcode` caller, under `QBE_INLINE_FAULT=standalone`: the link must fail or the values must differ from the model. A unit named `b.c`, `w.c` or `l.c` is refused for a candidate until the label rule of S1 covers function names (`L4_s1_result.md`).
3. **`checked` / `busy` across the two walks**: written in the plan before the code: walk 1 is today's walk unchanged and sets `checked`; walk 2 has its own mark; a clone is walked again if it contains a call that walk 2 admits. Check: `f` before `g` before `h`, `g` (two sites) calling `h` (two sites): no `jsl h` in `f`, and the copies counted in the trace equal the copies planned.
4. **`__ramcode` through transitivity**: a ROM `static` absorbed whole into a RAM caller brings its copies with it: decided and pinned either way (refuse the absorption of a function that holds admitted copies into a section of its own), with a `.checks` case and the link-time refusal of an overflowing RAM window shown.
5. **Stack depth through a recursive or NMI-reached caller**: the gates (the stack floor of `rom_coverage.py`, the lowest SP on the gauge windows) only see executed paths. Fixture: a recursive `static` of depth N calling a helper twice, the lowest SP read on luna and compared with `QBE_NO_INLINE_SIZE=1`, with a written ceiling.
6. **The call table of L4 S1 cannot become false, but proves nothing about a copy** (the comparison is set-wise: a duplicated or lost call to a callee still called elsewhere in the function is not seen): a golden case of `opensnes-rom` with two `jsl h` for one table line, to decide whether the multiplicity is compared.
7. **The new seed class bites into the old one** (8k+5 is half of 4k+1, the only class that hunts single-site absorption) and never produces a partial copy: run the class under a tiny `CC_INLINE_CALLER_BUDGET` to force per-site refusals.
8. **The `-d` trace mode** calls `inline_check(fn)` alone (`main.c:370-374`) and shows an IR without the copies: a `.checks` case comparing the `call` count of the traced IR with the emitted `jsl`.
Out of the contest's field and left to measurement: the budgets, the predictions, the priority criterion.

## Revision after S2 (2026-11-14): the stop rule of S2 fired, the plan is changed

S2 (`I1_s2_result.md`, output byte-identical, gates replayed) measured, with the predictions written before:
- the size threshold of 72 IR instructions holds on the corpus (376 right, 5 wrong, 15 missed for 256 bytes); the games are denser (2.94 / 2.63 bytes per instruction against 3.36), their best threshold is around 80 to 100; on the final size (H5) the own size is the right criterion (the final size would lose 19 / 12 % of the count);
- the ratio between the set the static criterion picks (at most 72, two sites or more) and S0's 256-byte set is **0.683 (game 1) and 0.734 (game 2)**, not the 0.95 the plan assumed: the missing gain sits in a few dense callees (one of 190 instructions is worth 0.64 % alone; no threshold under 190 catches it);
- the unit growth of S0's recompiled copy is 22.6 KB at most on game 1 and **+124 KB in one unit on game 2**, with no function growing past 5.1 KB and no section past 16 KB: the per-caller (8 KB) and per-section (16 KB) caps never bind; with 32 KB per unit and a priority by size, game 2 would lose 35 % of its count (1.73 to 1.12 %);
- the temporaries margin is real (96 to 98 % of the counted temporaries are live: H8 falls), but the test over-counts the 64 temporary registers of the callee, which a copy never clones (counted right, 4 of 7 and 39 of 97 refused calls would pass at their test: instantaneous, the cumulative effect is not measured); the recompiled check of S0 lost 14 of 42 single-site absorptions the compiler makes today (the copies took the margin first), which is the plan's own argument for running today's rule before the size rule;
- H7 (priority by loop depth) is not settled: the trace does not give the loop depth; the stake is up to 0.6 point of game 2.

Changes taken in the plan:
1. **Unit budget 128 KB estimated** (was 32 KB, whose justification, one LoROM bank, does not hold for code placed function by function); per caller 8 KB and per section 16 KB kept.
2. **Threshold kept at 72**; the free copies (no budget) drop from "6 instructions or fewer" to "2 or fewer".
3. **The S4 prediction is rewritten with the measured ratio: -1.23 % (game 1) and -1.26 % (game 2)** instead of -1.7 / -1.6 %, range -0.9 to -1.6; with the old 32 KB budget game 2 would fall near -0.8 %, under the stop bar of 1.02 %: the stop bar stays 1.02 % and the budget is part of what S4 must show.
4. **S5 starts with the exact count of the margin** (the callee's 64 temporary registers not counted), before any idea of raising the 1 800.
5. The dense callees (190 instructions and above) are out of this lot's reach; if they are wanted, it is a second rule with its own count.
