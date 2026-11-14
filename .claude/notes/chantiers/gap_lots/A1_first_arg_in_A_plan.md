# A1: the first integer argument passed in A (cut plan)

Plan written 2026-11-14, before any code and before any count. Decided by the conductor from the comparison with
another 65816 compiler (`G1_generator_prototype.md`: its `call` costs 0.41 of ours, `chain` 0.68; in our code every
argument is pushed and read back as `n,s`). Read for it: `compiler/ABI.md`, `.claude/rules/compiler.md`,
`abi_lint.md`, `testing.md`, `L4_frames_and_calls.md`, `L4_plan_decoupe.md`, `L4_s0_count.md`, `L4_s1_result.md`,
`L4_s3_result.md`, `E1_far_pointer.md`, `devtools/gapbench/` (`kernels.c` rows `call`, `chain`, `state`, `fptr`),
`compiler/qbe/w65816/abi.c` (Opar lowering), `emit.c` (Oarg / Ocall, prologue, `detect_block_tail_call`,
`count_fn_param_bytes*`, `sframe_dump`, `pp_reads_a`), `compiler/qbe/main.c` (`addrtaken`, `emit_collected`).

Nothing here is measured after a change. Every figure is a reading of today's tree, a prediction, or a hypothesis, and says
which. The names are neutral: jeu 1 / jeu 2, the games, another 65816 compiler.

Pair: a change to the calling convention touches code generation everywhere: complex, Class A. Opus writes each code step in
its own worktree (`git checkout -B work develop` first), Fable contests this plan once before S0 and reviews the closure;
the session re-runs every gate before a merge.

## 0. What decides argument passing today (read 2026-11-14)

- `abi.c`, `w65816_abi`: every `Opar*` becomes `Oloadsw SLOT(-paroff)` (Kw) or `Oload Kl SLOT(-paroff)`, walking the
  parameters BACKWARDS, so the last parameter has the lowest offset. Every `Oarg*` stays for the emitter.
- `emit.c`, `case Oarg*` (~7042): each argument in IR order, so leftmost first: `pea.w #k` for a constant, else
  `lda <arg> / pha` (`emitload_adj` with `argbytes`, the bytes already pushed), a Kl argument as two pushes, high first.
  The tail-call branch stores into the caller's own parameter slots instead.
- `case Ocall` (~7147): `jsl f` (direct), `jml [tcc__r9]` after `phk / pea` (indirect), or `jml f` (tail call, after the
  frame teardown). Cleanup: `plx` per word up to 2 words with a return value (1 without), else `tax / tsa / clc / adc.w
  #N / tas / txa`. Result in A (+ `tcc__retval_hi`).
- Parameters are read where the caller left them: `alias_opt` aliases each parameter temp to its stack slot
  (`temp_alias`), read as `lda F+PARAM_OFFSET+(-slot)+2,s`; a parameter is never copied at entry.
  `count_fn_param_bytes_via_slots` derives the bytes from the deepest slot named.
- Prologue: `rep #$20` (left out for `!lnk.export && !addrtaken(name)`, `emit.c` ~12318), then `tsa / sec / sbc.w #F /
  tas` when `F > 2`: **the frame is carved through A**.
- Tail calls (`detect_block_tail_call`): direct callee only, all arguments Kw, callee's argument bytes equal to the
  caller's parameter bytes, and (2 arguments or more) every argument already at its slot.
- `main.c`: every function of the unit is collected before any is emitted (`emit_collected`); `addrtaken()` is complete
  when the first function is emitted (the data references and every non-call code reference).
- `sframe_dump` (L4 S1, on `develop`): `; sframe fn NAME export= addrtaken= leaf= ... params=` and `; sframe call CALLER
  CALLEE ARGBYTES` after each function, `; sframe addr` at the end of the unit; `opensnes-rom check --objects` derives the
  same graph from the instruction lines and fails when they differ (`test_sframe_link.py`, in `make tests`). Not wired
  into user links (that was L4 S2, frozen).
- Static functions are emitted as `name.unit` (`stopCurrentEffect.main`).

**One property of our left-to-right push that this lot rests on**: the leftmost argument is pushed first, so it has the
HIGHEST offset in the callee. Taking it off the stack changes no other argument's offset (`F+4` is still the rightmost).
Read from `abi.c`'s backward walk: the first parameter is the last one numbered, so leaving it out only removes the deepest
slot. Hypothesis until S2's compiler check shows it on a 3-argument function.

## 1. The safety question first: who calls a C function?

Every way in, and what A1 must assume about it:

| Who | How it calls | Today | Under A1 |
|---|---|---|---|
| compiled C of the same unit | `jsl` to the symbol, arguments pushed by our emitter | stack ABI | the only caller A1 changes |
| compiled C of another unit | `jsl` to an exported symbol | stack ABI | unchanged: an exported function keeps the stack |
| library and project assembly | `pea` / `pha` then `jsl name` (`ABI.md`, "Calling C from assembly") | stack ABI, any A width for exported or address-taken C | unchanged for those; see the hole below |
| function pointers (tables, the object engine, scene stack, game loop callbacks) | `jml [tcc__r9]` from C, or assembly through a pointer | stack ABI | address taken: excluded |
| the NMI handler (`nmiSet`, `nmiSetLag`) and IRQ (`irqSet`) | `jml [nmi_callback]` from `crt0.asm` | stack ABI | address taken: excluded |
| an inlined copy (`splice_fn`) | no call: parameters become their arguments | n/a | unaffected |
| `__ramcode` functions | `jsl` to a `$7E` address | stack ABI | the convention is registers only, the address does not matter |
| a tail call (`jml f`) | arguments written into the caller's parameter slots | stack ABI | must follow the callee's convention (section 2.4) |

**The narrowest scope that stays correct**: a function is A1 when (1) it is not exported, (2) its address is never taken
(`addrtaken()`, the set the `rep #$20` rule already trusts), (3) its first parameter is Kw (16 bits or narrower:
`u8`, `s8`, `u16`, `s16`, `int`, `bool`; a pointer or a 32-bit first parameter keeps the stack in this lot), (4) at every
direct call of it in the unit the call has the same number of arguments and the first argument's class is the parameter's
(an unprototyped call that does not match keeps the stack: decided module-wide, before any function is emitted, in
`emit_collected`), (5) it is not variadic (refused anyway) and (6) a cost test says the callee does not lose
(section 2.2). Then every caller is a direct call this unit compiles, and both sides of the convention are written by the
same compiler run. Unlike a static frame (L4), the convention keeps no storage: recursion, re-entry and a call from both
the NMI callback and the main loop are all safe, so no call-graph check is needed for correctness.

**The hole, and how A1 closes it**: assembly can name a static today (`jsl foo.main`, the decorated label is global to
the link). `L4_s1_result.md` lists it as "not yet refused", and `ABI.md`'s sentence "assembly cannot name such a
function" is not true of the label. Today it works by accident (the arguments are on the stack). Under A1 that assembly
would run `foo` with garbage in A: a silent failure. Two ways out, to contest:
- **(chosen, hypothesis H1) a renamed entry**: an A1 function's label becomes `foo.main.A1` (a suffix that is not WLA's
  `.b/.w/.l` size suffix) and `foo.main` is not defined. Assembly that names `foo.main` fails the link (unresolved
  label): loud, before any ROM. `ABI.md` says a static's label is not an interface. Cost: the luna profiles and the
  gauge's per-function tables see the new name (the private tooling maps the suffix); `opensnes-rom`'s symbol readers
  (`take_sym`, the NMI / WRAM-port lint, `place` and `check --objects`) learn that `X.A1` is the function `X`.
- (fallback) keep the label and make `check --objects` refuse assembly naming a non-address-taken static: needs every
  project assembly object's text beside it (only `crt0` and the library keep a `.text.asm` today) and the check wired into
  user links (L4 S2's wiring, frozen). More code, and a refusal rather than an impossibility.

**What proves the scope, and which table is reused**: the L4 S1 side table already says, per function, `export=` and
`addrtaken=`, and per call the callee and the argument bytes. A1 adds one field, `arga=0|1`, to `; sframe fn`, and the
call line names the callee as it does today (the function, not the label). `check --objects` already derives the edges
from the instruction lines and compares them with the tables; A1 adds two comparisons, both from the instruction lines:
(a) a `jsl` / `jml` to an `.A1` label comes from the same unit (no other object can define a call to it: the label is
unit-local by name only, so this is the check that it stays so), and (b) no data line (`.dl`, `.dw`, a table of code) names
an `.A1` label (an address taken behind the compiler's back). Provoked faults: section 3.

## 2. The internal convention

### 2.1 Shape

- **A holds the first argument, 16 bits, extended as it would have been pushed** (a `u8` zero-extended, an `s8`
  sign-extended: the caller's `Oargub` / `Oargsb` already did it before the push; the callee trusts it as it trusts the stack
  slot today, `abi.c` loads every narrow parameter with `Oloadsw`). A 16-bit A at the `jsl` is already guaranteed (the
  emitter restores `rep #$20` before any instruction that is not a byte access; the callee's `rep #$20` is already gone
  for these functions). Not 8 bits: an 8-bit A would need a `sep` / `rep` pair at each end and changes the accumulator
  width contract every text rule relies on.
- **The other arguments stay on the stack, pushed left to right, at unchanged offsets** (the property of section 0). The
  caller pushes arguments 2..n, then loads argument 1 into A, then `jsl foo.main.A1`, then pops `2(n-1)` bytes.
- **Callee entry**: the first parameter is no longer an alias of a stack slot. It is an ordinary temp whose definition is
  "A at entry" (in `abi.c`: the first `Opar` of an A1 function becomes a copy from a pseudo-register the emitter
  recognises, instead of `Oloadsw SLOT(-paroff)`). The emitter sets the A-cache to that temp at entry, so a first use
  before A is written costs nothing; the slot colouring then decides where it lives if it must be stored:
  - not stored at all when every use comes before the first write of A (one read at the head: the common shape of a
    small helper);
  - `sta.b tcc__lf+N` (4 cycles) when it crosses no call (a leaf, or a non-crossing temp of a function that calls): stored
    BEFORE the prologue, the direct page does not depend on SP;
  - a stack slot when it crosses a call: the prologue carves the frame through A, so `tax` before it and `txa` after it,
    then `sta N,s` (4 + 5 cycles), or `tay`/`tya` when X is taken by the XY colour at entry (hypothesis: never, nothing is
    live in X at entry; the colour proof refuses it otherwise).
- **If the function calls again**, A is clobbered by the call like any caller-saved register: the parameter lives in its
  slot as above, which is what the colouring already does for any temp that crosses a call.
- **Return value**: unchanged (A, + `tcc__retval_hi`).

### 2.2 What it gains and loses per call (CPU cycles, 16-bit A, read from the emitter's forms; to be checked in S0)

| Part | Today | A1 | Delta |
|---|---|---|--:|
| caller, 1 argument computed into A | `pha` | nothing | -4 |
| caller, argument 1 a constant | `pea.w #k` (5) | `lda.w #k` (3) after the pushes | -2 |
| caller, 2+ arguments, argument 1 computed first | `pha` while A holds it | reloaded after the other pushes (`lda` 4-5) | about 0 |
| cleanup, 1 argument | `plx` | nothing | -5 |
| cleanup, 2 arguments | `plx/plx` (10) with result, `tsa..tas` (9) void | `plx` | -5 / -4 |
| cleanup, 3 arguments | `tax..txa` (13) with result, (9) void | `plx/plx` (10) / `tsa..tas` (9) | -3 / 0 |
| cleanup, 4+ arguments | arithmetic | arithmetic, same cost | 0 |
| callee, first read of the parameter, before A is written | `lda F+k,s` (5) | nothing | -5 |
| callee, each later read | `lda F+k,s` or `op F+k,s` (5) | `.b` (4) in `tcc__lf`, `N,s` (5) on the stack | -1 / 0 |
| callee, entry store when read after A is written | nothing | `sta.b` (4), or `tax/txa/sta N,s` (9) in a framed function | +4 / +9 |

Best case (one argument, read once at the head, e.g. `ch_anim(frame)`): -14 cycles a call. Worst case (3+ arguments, the
parameter crosses a call inside a framed callee): +9. **The convention is therefore decided per callee by a cost test on
its own text** (point (6) of section 1): A1 only if the callee's entry cost (0, 4 or 9) is at most the caller-side saving at
its arity, weighted by nothing dynamic (the compiler has no profile). The exact test is written after S0 has measured the
distribution of the three entry costs; hypothesis: "no stack-slot store" alone keeps almost every winning function.

### 2.3 What it changes for the other mechanisms

- **Tail calls** (four quadrants, as L4's contest point 6): (i) stack -> stack unchanged; (ii) stack caller -> A1 callee:
  the callee's stack bytes are `2(n-1)`, compared with the caller's parameter bytes, then `lda arg1 / jml foo.main.A1`;
  (iii) A1 caller -> stack callee: the caller's own stack parameter bytes are `2(m-1)` (one fewer than its arity), the
  existing comparison runs on that figure; (iv) A1 -> A1: both. `detect_block_tail_call` compares `nargs * 2` with
  `caller_param_bytes` today: S2 turns both into stack bytes and adds the load into A before `jml`. A tail call whose
  convention it cannot honour is declined with a `CC_TRACE_TCO` reason (`arga`), never miscompiled; S0 counts the tail calls
  executed between such pairs (L4 S0: game 2 executes 137 tail calls from static candidates).
- **The jump table (L2)**: `switch (first parameter)` at the head of a function: `emit_jt` loads the value through the
  A-cache, so the `lda v` before the window may vanish. `jt_rewrite` matches the dispatch group (`asl a / tax / bvs ...`),
  not the load (read in `compiler.md`; hypothesis until a compiler check `arga_switch` shows it).
- **`tcc__lf`**: a leaf A1 function whose parameter must be stored takes one more word of the 20 (`DPF_WORDS`); a leaf at
  the limit is split (`leaf_split`) one word earlier. Counted in S0 (functions at 19-20 words today).
- **XY colour (lot XY)**: the parameter temp is an ordinary temp, so it may be coloured X or Y like any other; the entry
  `tax` that the colour emits is then the definition itself (`tax` instead of `sta`): a gain the plan does not count.
- **The text rules that read `N,s`**: the parameter's `lda F+k,s` lines disappear; what replaces them is a slot the rules
  already know (`tcc__lf+N`, `N,s`), not an opaque operand (the L4 S3 trap is not this one: section 4). The new risk is the
  other way: **a `jsl` now READS A**. `pp_reads_a` (`emit.c` ~9588) lists `sta`, `adc`, `pha`, `tax`... and no `jsl` /
  `jml`. A rule that walks forward from an `lda` and takes a `jsl` for "A overwritten, unread" would remove argument 1's
  load. The base peephole ends its straight line at a call (safe); `pp_a_dead_after`, `pp_nz_dead_after`, the A half of
  `xflow`, the loop rules' "first touch of A is a write" and the XY proof must each be read again. S2 adds `jsl` and `jml`
  (all of them, conservatively: an A1 callee and an asm routine are not told apart in the text) to `pp_reads_a`, and a
  provoked fault proves the tests see it (section 3).
- **The emitter's A-cache**: after the pushes, the cache may already hold argument 1 (`f(x, x)`); otherwise `emitload_adj`
  with the current `argbytes`, as the pushes do. Invalidated after the `jsl`.
- **The L4 side table**: `params=` becomes the stack bytes (`2(n-1)` for an A1 function), `arga=1` added; the call line's
  `ARGBYTES` the bytes pushed. `opensnes-rom` (`sframe.c`) reads both; `QBE_SFRAME_FAULT=noedge` must stay red.
- **`ABI.md`**: a section "Internal convention of static functions" (the six conditions, A extended, offsets unchanged,
  the renamed label); "Calling C from assembly" says the label of a static is not an interface (which it already implies).
  `check_asm_abi.py` reads only public signatures and hand-written assembly: not concerned (`abi_lint.md`).

### 2.4 How A1 articulates with L4 (frozen after S3, branch `l4s23`)

- **Same predicate at the root** (`!export && !addrtaken`), but A1 also takes leaves, functions with an `alloc`, recursive
  functions and functions an NMI root reaches: it keeps no storage, so none of L4's refusals (overlap, window, NMI page)
  applies. A1 needs no link-time placement and no `make/common.mk` change.
- **It does not contradict L4's change 3 (S5, arguments stored into the callee's static frame); it narrows it**: for a
  framed A1 function the first argument arrives in A and the callee stores it in its frame (`sta.b sf.<fn>+K`) or keeps it in
  A; S5 then moves arguments 2..n only. S5's parameter bytes shrink by 2 per A1 function (S0 of L4 found the window too
  small for parameters: 150 / 116 B against 88). Where S5 and A1 compete (one argument, read at the head), A1 is cheaper:
  the caller's `sta.b` disappears instead of replacing a `pha`.
- **Order**: A1 lands on `develop` first; L4's branch, if resumed (S4), rebases on it. The conflict points are known:
  the prologue (A1 stores before the frame is carved; S3 has no frame to carve), `color_slots` (the parameter temp joins
  the colouring), `count_fn_param_bytes*` and the side table's `params=`. S3's `sf_text_check` and A1's `.A1` labels do
  not meet.
- **It is not L4's F3 as first written** ("first argument in A for static callees, about 0.6 %"): F3 was folded into
  change 3 by the contest. A1 takes it back as its own short lot because the gapbench comparison shows calls as the gap,
  and because it carries none of the window's costs.

## 3. The steps

Each step is one sub-agent's work, its prediction written and committed before its code, its switch, and the provoked fault
that proves its check can turn red. **Stop rule, written now, before S0**: S0 must find that at least **1.5 % of the busy
time of one game** is reachable by A1 as scoped in section 1 (a lower bar than L4's 3 % because the lot is narrow and
carries no RAM cost); then, measured after S2 (the first code step that changes code), the gauge must return **at least
60 % of S2's own prediction** (L4's point 9) on that game, or the lot goes back to the conductor with the figures and the
switch defaults to off.

### S0, the count, no code

- **Question**: what share of the two games' busy time is in calls to functions that would be A1 (section 1, conditions
  1-5), and what does each such call save (section 2.2, the cost test of condition 6 applied)?
- **Method** (the private tooling of M1/M2, outside this repository; gauge builds of `develop`, 4 x 40 frames a game):
  1. **Static census, from the texts as compiled today**: the side tables of every `.c.asm` of both games and the corpus
     give `export`, `addrtaken`, `params` per function and the calls per site; the class of the first parameter comes from
     QBE's `-d A` dump (after ABI lowering: `Oload Kl` against `Oloadsw` on the deepest slot), no compiler change. Per
     candidate: arity, entry cost (0 / 4 / 9) read from its text (where the deepest-slot `lda` stands relative to the first
     write of A and to the first call), tail calls into or out of it.
  2. **Per site saving, from the text**: the caller's actual push of argument 1 (`pha`, `pea`, reload needed or not), its
     cleanup before and after (the table of 2.2), the callee's reads of the parameter: each instruction priced by the
     same cycle model as M1, on today's text.
  3. **Execution counts only from the traces**: how many times each call site and each callee block ran. A count of
     executions does not depend on the convention; a classification by executed path is not used (the LF lesson).
- **What it reports, per game and for the corpus**: the A1 figure (% of busy) split into caller push, cleanup, callee
  read, minus entry stores; the same for three wider variants, each a separate line and NOT in the rule: **A1-L** (a Kl first
  parameter in A + X), **A1-W** (exported functions called from their own unit, through a second entry label that loads the
  first parameter from the stack and falls into the A1 body: the stack offsets make it a 5-cycle `lda` before the
  prologue; its cost to calls from other units priced too), **A1-T** (the tail calls that quadrants (ii)-(iv) keep or lose);
  the distribution of entry costs; the leaves pushed over 20 words of `tcc__lf`; the top ten call sites by saving.
  Also the gapbench: `call` and `chain` call exported functions, so **A1 moves neither row as written**; S0 predicts, by
  hand on the text, two starred twin rows `callst*` and `chainst*` (the callees `static`, two call sites each so they are
  not inlined whole), out of the means like the other starred rows.
- **Prediction (written now)**: L4 M1 counted 0.58 % (jeu 1) and 0.61 % (jeu 2) for the push of the first argument of
  every same-file call. A1 keeps only the static callees with a Kw first argument (a fraction, hypothesis one half), loses
  the push when argument 1 must be reloaded (2+ arguments), and adds the cleanup (L4 S0: the whole caller's stack restore of
  static candidates is 0.32 / 0.40) and the first read (5 cycles). **Predicted A1 figure: 0.4 to 0.9 % on each game, under
  the 1.5 % bar.** A1-L and A1-W together: hypothesis +0.3 to +0.8. Twin rows: `callst*` about -9 % of `call` (one `pha`, a
  cheaper cleanup, `a` read once at the head: about 12 cycles of 1 007 mclk an iteration), `chainst*` about -10 % of `chain`
  (four or five 14-cycle calls an update; `ch_update(Act *)` itself is Kl, excluded). Contradicted if S0 finds more than
  1.2 % on a game.
- **Gate**: no ROM changes (nothing built differently); the census of the corpus equals by hand the five examples read
  in full.
- **Stop**: under 1.5 % on both games for A1 as scoped: the lot stops and goes back to the conductor with the figure and the
  wider variants' lines (A1-L, A1-W), which are another lot's design (A1-W changes exported functions, a public-facing
  shape) and are not opened by this plan.

### S1, the decision and the table, no code change in any ROM

- **Object**: `arga_wanted(fn)` computed in `emit_collected` after inlining and after the second `addrtaken_scan` (conditions
  1-6; the cost test of 2.2 as S0 wrote it), a module-wide set read by the emitter; `arga=` in `; sframe fn`;
  `QBE_ARGA_TRACE=1` names each function and why it is or is not A1. The convention itself is not applied.
- **Files**: `compiler/qbe/main.c`, `compiler/qbe/w65816/emit.c` (`sframe_dump`), `tools/opensnes-rom/src/sframe.c`
  (reads `arga=`), `devtools/compiler-tests/cases/` (`arga_decide`).
- **Prediction**: the 65 ROMs and `.sym` byte-identical (comment lines only); the set equals S0's census function for
  function on the corpus and both games (two implementations, one answer: S0's script is the oracle).
- **Gate**: `cmp` of the 65 `.sfc` and `.sym`, both games' ROMs; `test_sframe_link.py` and the golden cases green.
- **Switch**: nothing to switch (no effect).
- **Provoked fault**: `QBE_ARGA_FAULT=addr` ignores `addrtaken()`: the census differs from S0's on the examples that hand a
  static to `nmiSet` / the object engine (named in the result), and the compiler check `arga_decide` fails.
- **H1 measured here** (no compiler change needed): a two-object WLA link where assembly names `foo.main` while the C object
  defines only `foo.main.A1` must fail; `foo.main.A1` must assemble and link where a `.b` / `.w` / `.l` suffix would not.

### S2, the convention, both sides at once (the first code step)

The two sides cannot ship apart (a callee reading A that the caller did not load is a miscompilation), so S2 is the whole
convention for scope A1, quadrants of tail calls included.

- **Object**: `abi.c` (the first `Opar` of an A1 function becomes the copy from A); `emit.c`: entry store before the
  prologue or `tax/txa` around it, A-cache at entry, `alias_opt` and `count_fn_param_bytes*` without the first parameter,
  `case Oarg*` defers argument 1 of a call to an A1 callee, `case Ocall` loads it after the pushes then `jsl
  name.A1`, cleanup on `2(n-1)` bytes; `detect_block_tail_call` and the tail-call `Oarg` branch on stack bytes, load into
  A before `jml` (four quadrants); the function's label `name.A1`; `pp_reads_a` reads A at `jsl` / `jml`; the side table
  (`params=`, `ARGBYTES`); `opensnes-rom` maps `X.A1` to `X` in every symbol reader.
- **Files**: `compiler/qbe/w65816/abi.c`, `emit.c`, `compiler/qbe/main.c`, `tools/opensnes-rom/src/{sframe.c,asm.c}`,
  `testing/difftest.py` (a fifth shape: the expression's operands as parameters of a `static` function with two call sites;
  the `f` shape calls an exported one), `testing/difftest_stmt.py` (static non-leaf helpers with two call sites: the
  generator change of L4 S3, branch `l4s23`, is NOT on `develop` and is ported here; today's odd seeds reach A1 only when a
  program calls a helper twice), `testing/fixtures/compiler/arg_in_a/` (new), `devtools/compiler-tests/cases/` (`arga`,
  `arga_off`, `arga_tco`, `arga_switch`), `devtools/gapbench/kernels.{c,asm}` (twin rows `callst*`, `chainst*`, the assembly
  twin unchanged), `compiler/ABI.md`, `.claude/rules/compiler.md`, `KNOWN_LIMITATIONS.md` (assembly naming a static now fails
  the link), `CHANGELOG.md`.
- **Prediction, in two parts, both committed before the gauge runs**: (a) S0's per-game figure as the ceiling, and the twin
  rows as S0 predicted them; (b) once the code compiles and before any game is measured: build both games with and without
  `QBE_NO_ARGA=1`, diff their texts function by function, price every changed line with the cycle model and multiply by
  the block execution counts of the reference trace (control flow does not change, the counts hold; a function whose blocks
  change shape is priced as a whole). That figure, (b), is the one the measured stop compares with. Code size: down by 2
  bytes per call that loses a `pha` + `plx` pair, up by 2 to 4 per stored entry (hypothesis: -0.2 to -0.5 % of the games' C).
- **Gate**: `make clean && make` (zero warnings); `make tests`; `make test-compiler`; `QBE_NO_ARGA=1`: every unit of the corpus
  and the library byte-identical to the compiler before the lot; `diff_corpus.py` 65/65 MATCH at equal frame; `difftest.py`
  and `difftest_stmt.py` **20 000 fresh seeds each** (a new call shape and a new reader of A: the standard of a new peephole
  rule, not Class A's 2 000); the fixture `arg_in_a` (values against a host model: arities 1 to 4, `u8` / `s8` first arguments
  with values above 127, a Kl first argument that must stay on the stack, recursion, a framed callee whose parameter crosses
  a call, a leaf whose parameter goes to `tcc__lf`, a parameter read once at the head, an unused parameter, `f(g(b), c)`,
  `f(x, x)`, `switch` on the parameter as a jump table, each quadrant of tail calls, the same static called from the main loop
  and from the NMI callback); both games' state identical to the reference at every gauge step; `make test-sanitizers`;
  `make test-torture` once; `make lint` (`lint-docs`, `ABI.md` anchors).
- **Switch**: `QBE_NO_ARGA=1` (the stack everywhere, byte-identical output); `QBE_ARGA_TRACE=1`.
- **Provoked faults** (each must turn its test red; `QBE_ARGA_FAULT=`):
  1. `calleestack`: the callee reads its first parameter from the stack slot as before: fixture red, `difftest_stmt` red on
     its gate seeds.
  2. `noload`: the caller pushes nothing and loads nothing into A: fixture red.
  3. `prologue`: no `tax/txa` around the frame carve: the framed case of the fixture red.
  4. `readsa`: `pp_reads_a` ignores `jsl`: red if any rule exploits it; if no emitted text changes, the result says so, as
     E1's fault 4 did (a guard with no case to catch today is still written down).
  5. `addr`: an address-taken static made A1, called through a pointer by the fixture: red.
  6. `tco`: quadrant (ii) without the load into A: `arga_tco` and the fixture red.
  7. `noedge` (L4's) still red on a unit with A1 calls; a hand-written object with a data line naming `x.A1`: `check
     --objects` refuses.
- **Measured stop**: the gauge (busy master cycles a step, state identical at every step) must return at least 60 % of
  prediction (b) on the game that passed S0. Otherwise the code stays on its branch with `QBE_NO_ARGA` defaulting on, and the
  conductor decides, as after L4 S3.

### S3, only if S0 counted it: one widening at a time

Each of A1-L (Kl first parameter in A + X), A1-T (if S2 declined quadrants for cost) and A1-W (a second entry for exported
functions) is its own step only if S0 counted at least **0.5 % on one game** for it (L4's per-part rule). A1-W is a change of
what an exported function looks like (two labels, one of them public): it needs the conductor's go and an `ABI.md` section
before its prediction is written.

### S4, closure

The gauge with each switch alone and together, `make bench-gap` (the twin rows, predicted against measured), the oracles
re-captured with their reasons (WRAM pages that held parameters move; the audio phase of examples whose C got faster, with
`luna diff --audio --align-onset` quoted), the lesson in the porting kit, `compiler.md`, `ABI.md`, `CHANGELOG.md`, the
games told before it lands (their builds change; `bin/` rebuilt on the checkout they read).

## 4. What is already known that should make us doubt

Three counts of this class were too optimistic once run:
- **LF** (more `tcc__lf`): counted 1.14 % on one game, measured 0.24. The count classified values by the path one run
  executed; the compiler decides statically.
- **L4 S3**: predicted 1.4-1.8 / 1.9-2.2 %, measured +0.54 % (slower) / -1.06 %. The new operand was opaque to the text
  rules, and what the rules did on `N,s` (X known across blocks, `lda S / sta S`, the entry reload) was lost; a replay of
  traces could not see it.
- **The opaque operand** in general: whatever a text rule knew about the old form is lost unless the new form is one it
  already reads.

What A1 does about each:
1. **No classification by executed path**: S0 decides A1 or not from the compiler's own static conditions; the traces give
   only execution counts.
2. **The prediction that the stop compares with is made on recompiled texts** (S2 part (b)): the games are built with the
   change and the text diff is priced, before the gauge runs. Nothing the text rules do or stop doing is hidden from it.
3. **No new operand**: the parameter lives in `tcc__lf+N` or `N,s`, forms every rule reads. What A1 removes is a stack read
   the rules could not improve anyway (a parameter is read in place and never written).
4. **The known way it can be slower is in the decision**: an entry store that costs more than the caller saves (section
   2.2); the cost test refuses it, and S0 measures how many functions sit on that edge.
5. **The prior figure already says "probably not enough"**: M1's 0.58 / 0.61 % for all same-file first-argument pushes is
   the ceiling of the push part, and S0's prediction (0.4 to 0.9 %) is under the bar. The plan is written so that S0 can
   stop the lot in a day, and so that the wider variants are reported, not slipped in.
6. **The bench does not see the lot as written**: `call` and `chain` call exported functions. A twin row that does see it
   must not be read as the games' gain; the weighted mean does not move, by construction.

## 5. Hypotheses, and the measurement that settles each

- **H1** a label `foo.main.A1` assembles, links, and makes `jsl foo.main` from assembly fail the link: two-object WLA fixture
  (S1).
- **H2** leaving the first parameter out of the stack changes no other offset (section 0): compiler check `arga` on a
  3-argument function, offsets read in the text (S2).
- **H3** half of the same-file first-argument pushes of M1 go to static callees with a Kw first parameter: S0's census.
- **H4** "no stack-slot entry store" alone keeps almost every winning function (the cost test): S0's distribution of entry
  costs.
- **H5** nothing is live in X at a function's entry, so the `tax/txa` around the frame carve never collides with the XY
  colour: `QBE_XY_ASSERT=1` over the fixture and 2 000 `difftest_stmt` seeds (S2).
- **H6** `jt_rewrite` does not need the `lda v` before the window: check `arga_switch` (S2).
- **H7** no text rule today takes a `jsl` for a write of A that kills an earlier load: fault 4 of S2 says whether one does; a
  read of the four helpers named in 2.3 before S2's code.
- **H8** `difftest.py`'s new shape and `difftest_stmt.py`'s two-site helpers reach A1 in at least a quarter of the gate's
  seeds: `QBE_ARGA_TRACE=1` over the gate (S2).
- **H9** the private gauge tooling, which matches functions by name across builds, maps `X.A1` to `X`: one run of the gauge
  on S1's build (labels unchanged) and S2's (renamed) with the same function list.
- **H10** the tail calls executed between an A1 function and a stack function are few (L4 S0: 137 from static candidates on
  jeu 2, about 0.04 %): S0's A1-T line.

## 6. To contest before S0 (Fable, written)

1. The scope (section 1): is there an entry into a non-exported, non-address-taken function that is not a direct call of
   the unit (a hook written by assembly to a C name, `__ramcode` copied by name, a debugger call, a `jml` the inliner
   leaves)?
2. The rename `name.A1` against the link check: is an impossibility (unresolved label) better than a refusal, given the
   profile and gauge tooling that read names?
3. A 16-bit A with the caller's extension, the parameter not copied when read at the head: does any emit path read a
   parameter's high byte, or rely on the parameter being in memory (an `Oload` of its address, `&param`)? `&param` makes it
   an alloc today; is the copy then correct?
4. The cost test as the decision (no profile): too blunt? Should a function whose parameter needs a stack store simply never
   be A1?
5. `pp_reads_a` made to read A at every `jsl`: does it cost any rule something today (a load before a runtime helper that a
   rule was right to remove)?
6. The stop bar at 1.5 % while the prior count says 0.4 to 0.9: should S0 run at all, or should the conductor decide on M1
   alone?
7. The tail-call quadrants inside S2 rather than declined: worth the risk for H10's 0.04 %?

## Outcome (2026-11-14): the lot stops at S0

S0 (`A1_s0_count.md`) found 0.90 % (game 1) and 0.97 % (game 2) of busy time, under the 1.5 % rule written before the count; even with the entry stores counted as free the ceiling is 1.44 / 1.26 %. The eligible callees are 89 % and 79 % of the executed calls (H3 was wrong by excess), but each call gains little (1.6 to 1.9 pushes saved, not 2.5: the entry store eats most of what the re-read returns). The variants outside the rule add nothing worth a step (A+X 0.00 / 0.02 %, double entry -0.03 / +0.08 %, tail calls about 0.0006 %). No code step is opened. Kept: the counting script (`make count-a1`) and the fidelity of the plan's prediction method (a count on the recompiled text, not on replayed traces).
