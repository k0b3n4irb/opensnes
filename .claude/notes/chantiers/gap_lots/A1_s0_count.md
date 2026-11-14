# A1 S0: the count before any code (first integer argument in A)

Step S0 of `A1_first_arg_in_A_plan.md` (the conductor's plan of 2026-11-14). A count: no compiler change, no ROM built
differently. The names are neutral (jeu 1, jeu 2, the games).

## Prediction, written 2026-11-14 before the count (committed alone, before any census or script exists)

Nothing below is measured. The plan predicts **0.4 to 0.9 %** of busy time on each game. My own, with its arithmetic:

- **Ceiling of the push part**: L4 M1 counted 0.58 % (jeu 1) and 0.61 % (jeu 2) for the cost of the first `pha` / `pea` of
  every executed `jsl` to a callee of the same file (build `l2`). A reading of `m1.py` (Fable's report, checked): exported
  and static callees alike, any class of first argument, the push only (no cleanup, no read in the callee).
- **Share eligible** (hypothesis H3): the static callees whose first parameter is Kw, with every call of the unit agreeing.
  The games pass many structure pointers first (a pointer is Kl, excluded); I take **one half** of M1's calls by
  execution count, range 0.35 to 0.65.
- **What one eligible call saves, against the push M1 priced** (plan 2.2, cycles of 16-bit A): 1 argument, read at the
  head: push 4 + cleanup 5 + first read 5 = 14, i.e. 3.5 pushes; 2 arguments: push about 0 (reloaded), cleanup 4-5, read
  5, i.e. about 2.3 pushes; 3+: read 5, cleanup 0-3, about 1.5 pushes; minus entry stores (4 or 9) where the parameter is
  read after A is written. Hypothesis on the mix (most hot static helpers take one or two arguments): **about 2.5 pushes**
  per eligible call, range 1.8 to 3.0, entry stores included.
- **Figure**: 0.58 x 0.5 x 2.5 = **0.73 % (jeu 1)**, 0.61 x 0.5 x 2.5 = **0.76 % (jeu 2)**; with the ranges
  (0.35-0.65, 1.8-3.0): **0.4 to 1.2 %** on each game. Tail calls, A1-L and A1-W are not in it (lines apart).
- **The rule** (plan section 3, written before S0): at least 1.5 % of one game's busy time, or the lot stops.
  **Prediction: under 1.5 % on both games, the lot stops after S0.** What would contradict it: more than three quarters of
  the same-file call executions going to Kw-first static callees AND a mix dominated by one-argument calls read at the
  head (then about 1.5).
- **Coherence check announced** (mission point 6): the census's count of executed `jsl` to a callee of the same file must
  equal what `m1.py`'s rule counts on the same trace (its condition: callee a C function of the same `.c.asm`).

## Result (2026-11-14, measured after the prediction above was committed)

**Verdict of the rule: the lot stops.** A1 as scoped (plan section 1, conditions 1-5, with the cost test of condition
6) reaches **0.90 % of busy time (jeu 1) and 0.97 % (jeu 2)**, under the 1.5 % the plan wrote before S0, on both games.
No reading of the count reaches the bar: the best gauge window is 1.08 % (jeu 2), the selection by profile (callees that
win, which no compiler can know) 0.94 / 1.03 %, and even with every entry store taken as free (a ceiling, not a design)
1.44 / 1.26 %. With the three variants added (not in the rule), about 0.87 / 1.07 %. S1 is not started; the figures
and the variants' lines go back to the conductor.

### Build, trace, method

- The gauge's frozen reference `ref1114` (`develop` of 2026-11-14, the SDK's `bin/` and `lib/`), both games: `.c.asm`
  with the side table, ROM, `.sym`. The IR of each unit kept by `CC65816_KEEP_IR` on a second build of the frozen sources
  with the same SDK: **ROMs `cmp`-identical to `ref1114`** (so the IR is the IR of these texts). The library's IR from
  this branch's build (its 26 LoROM `.c.asm` are byte-identical to the SDK's that `ref1114` linked).
- Traces of `ref1114`, the gauge's 4 x 40 frames a game (private `trace.py`, unchanged). Busy on the trace: 27 866 030
  mclk (jeu 1), 34 243 412 (jeu 2).
- `devtools/sframe/a1_count.py` (generic, `make count-a1`): eligibility from the table (`export=`, `addrtaken=`) and the
  IR (parameter classes, variadic, each direct call's arity and first-argument class); per site and per callee, what
  changes is read in the TEXT (module docstring: push of argument 1, moved / reloaded / spilled when 2+ arguments, the
  cleanup under the emitter's own `plx` rule, the callee's reads of the slot found by entry-relative offset with pushes
  and the carve tracked, head read, crossing a call, `tcc__lf` room). The trace is used only to count: every text
  instruction of every C function is placed at its ROM address by decoding the ROM bytes, checked mnemonic by mnemonic
  and at every local label against the `.sym` (**390 / 389 functions placed, 0 mismatch**; the 203 / 226 not placed have
  no label: library functions the link dropped). Each removed or added instruction is priced at the median measured cost
  of its shape in the same trace (`pha` 28 mclk, `plx` 34, `lda n,s` 34, `lda dp` 28, `sta dp` 28, `lda #` 18, `tax` 12).

### The coherence check (mission point 6)

Executed `jsl` to a C function of the same `.c.asm`, summed over the text's sites: **6 248 (jeu 1), 7 913 (jeu 2)**. M1's
rule replayed on the same `ref1114` trace (next row in a C function of the caller's file): 6 247 and 7 909; the 1 and 4
missing are calls whose next row is the NMI handler (an interrupt taken right after the `jsl`, listed one by one).
M1's own figure on its build `l2`: 6 248 (jeu 1, the same) and 8 011 (jeu 2: the compiler has changed since, inlining
and the lots XY / E1). Totals agree.

### (a) Census

| | corpus (65 examples) | library alone | jeu 1 (project + library) | jeu 2 |
|---|--:|--:|--:|--:|
| C functions emitted | 260 | 251 | 593 | 615 |
| no parameter | 156 | 64 | 181 | 149 |
| exported | 38 | 168 | 230 | 201 |
| static, first parameter Kl (pointer, 32 bits) | 16 | 5 | 27 | 22 |
| **A1 eligible (conditions 1-5)** | **50** | **14** | **155** | **243** |
| of which kept by the cost test (condition 6) | 35 | 8 | 103 | 174 |
| call sites of the eligible (text) | 180 | 81 | 621 | 1 027 |
| executed sites / executed calls (gauge window) | | | 97 / 5 563 | 92 / 6 280 |

(The games' columns count the library's `.c.asm` too, linked or not.) No unit without its IR; no function where the arity or the first class
disagrees between calls; no write to a parameter slot; every recognised push sequence and cleanup matched the callee's
parameter bytes (one shape fixed in the script before any figure: a frame popped by `plx` in the epilogue right after the
cleanup's `plx`).

Gate of the plan (the census equals by hand five examples read in full): `basics/aim_target` (`turret_tile(u8)`, 2
sites), `mode7/mode7_racing` (`draw_rival(u8)`, 1 site; `to_screen` has one site and is inlined whole),
`input/controller` (`show_pad(u8)`, `walk(u8)`, `first_name(u16)`, 2 sites each), `memory/save_game` (`show_slot(u8, u8,
ptr, ptr)` eligible, 4 sites, refused by the cost test: 4 arguments, entry store in `tcc__lf`), `backgrounds/mode2`
(`loadScene(u8)`, 2 sites): the script says the same, function for function.

Entry cost (plan 2.2), eligible / kept:

| case | jeu 1 | jeu 2 | corpus (eligible) |
|---|--:|--:|--:|
| read once, at the head, A untouched (0 cycles) | 35 / 35 | 32 / 32 | 13 |
| the same in a framed callee (`tax/txa` around the carve, 4) | 6 / 5 | 15 / 11 | 3 |
| stored in `tcc__lf` before the carve (4) | 55 / 43 | 77 / 64 | 24 |
| stored on the stack, framed callee (`tax/txa/sta n,s`, 9) | 43 / 19 | 90 / 67 | 8 |
| a frame to create (23) | 15 / 0 | 26 / 0 | 2 |
| a leaf over the 20 words of `tcc__lf` | 0 | 3 / 0 | 0 |
| parameter unused | 1 / 1 | 0 | 0 |

### (b) Gain, A1 as scoped (% of busy, kept by the cost test)

| | jeu 1 | jeu 2 |
|---|--:|--:|
| caller, push of argument 1 | 0.441 | 0.353 |
| caller, cleanup | 0.530 | 0.424 |
| callee, reads of the parameter | 0.467 | 0.479 |
| callee, entry stores | -0.539 | -0.290 |
| **A1 (rule)** | **0.899** | **0.966** |
| every eligible, no cost test | 0.881 | 0.824 |
| only the callees that win on this trace (profile, not decidable) | 0.942 | 1.032 |
| ceiling: entry stores free | 1.438 | 1.256 |
| per gauge window (4 x 40 frames) | 0.81 / 0.93 / 0.91 / 0.91 | 1.08 / 1.06 / 0.99 / 0.79 |

Executed calls by shape of argument 1: jeu 1 one argument 3 781, two or more with the `lda` moved after the pushes 1 269,
a constant (`pea` -> `lda #`) 485, spilled and reloaded 28; jeu 2 3 616 / 2 099 / 555 / reloaded from its slot 8 /
spilled 2. By entry case of the kept callees: jeu 1 `tcc__lf` 0.47, head 0.22, stack 0.21; jeu 2 head 0.56, `tcc__lf`
0.26, stack 0.10, head framed 0.04. The gain is concentrated as L4's was: jeu 1's actor-step helpers, jeu 2's object code
(one helper alone 0.37 point). Top ten sites by caller saving: one-argument calls of the hot helpers, 0.03 to 0.15 point
each (the private run's JSON lists them with their names).

### (c) Variants, apart, not in the rule

| % of busy | jeu 1 | jeu 2 |
|---|--:|--:|
| **A1-L** (Kl first parameter in A + X), kept / all | 0.000 / 0.000 | 0.000 / 0.015 |
| **A1-W** (exported, second entry `lda n,s` for calls from other units), kept | -0.032 | +0.080 |
| **A1-T** tail calls executed: A1 -> A1 (kept) / A1 -> stack (lost) / stack -> A1 (lost) | 2 / 0 / 0 | 129 / 2 / 0 |

A1-L: the hot pointer-first statics of these games are inlined or cold (jeu 1: no call executed; jeu 2: 118). A1-W: the
exported functions are almost never called from their own unit in the window (jeu 1: no such call; jeu 2: 840, one
function), and every call from elsewhere pays the extra `lda`. A1-T: the 2 lost tail calls of jeu 2 at L4's hypothesis of
100 mclk each are 0.0006 %; the kept ones save at most a `sta n,s` each (0.013 %). By the plan's per-part rule (0.5 % on
one game), none of the three would open a step.

### (d) The rule applied

Plan section 3, written before S0: at least 1.5 % of the busy time of one game. **0.90 % (jeu 1) and 0.97 % (jeu 2): the
lot stops.** Not one reading gets there (best window 1.08, ceiling 1.44). The wider variants (A1-L, A1-W) are another
lot's design and are not opened by this plan; their lines above are what the conductor gets.

Against the predictions: the plan's 0.4 to 0.9 % holds for jeu 1 and is exceeded by 0.07 for jeu 2 (its "contradicted
above 1.2" is not reached); mine (0.73 / 0.76, range 0.4 to 1.2) holds, for the wrong reasons on both factors: **H3 is
contradicted upwards** (the eligible take 89 % and 79 % of the executed same-file calls, not half: the hot static helpers
take a small integer first, an index or an id) and **the saving per call is smaller than predicted** (45 and 53 mclk, 1.6
and 1.9 pushes, not 2.5): the entry stores eat most of what the callee's reads give back (jeu 1: -0.54 against +0.47).
**H4 is contradicted for jeu 1**: callees that need a stack store still win (0.21 of 0.90), because one-argument callers
save a `pha` and a `plx` against a 9-cycle store; the cost test as written (entry cost at most the caller-side saving at
the arity) refuses 52 and 69 functions, which together would lose 0.02 and 0.14 point, and keeps 95 % of what a profile
would pick (0.90 against 0.94, 0.97 against 1.03).

### Not measured

- Recompiled text: every figure is today's text with the changed instructions priced; what the text rules would do with
  the new shapes (the LF+M5 lesson: a removed `sta` / `lda` pair can unlock others, or a rule can lose a shape) is not in
  it. Plan S2's part (b) prediction is the step that would see it; it is not reached.
- The crossing test is textual (a read after a call in the layout, or inside a backward branch that contains a call),
  not the compiler's liveness: it can call a value crossing when it is not, never the reverse (an entry store over-priced,
  never under-priced) on functions whose calls and reads are interleaved.
- A1-L's caller side is approximated (the high half to X by `ldx` of the same operand or `tax`); A1-T's kept tail calls
  are bounded, not priced; the tail calls A1 could create are not counted.
- The gapbench twin rows `callst*` / `chainst*` of the plan were not predicted: the lot stops before they would serve.
- No gain on the examples (no trace): census only.
- The windows are the gauge's; the busy figures carry luna's DRAM-refresh bias (the gauge's notes), which makes every
  percentage here slightly low, by much less than the margin to 1.5.

### Replay

```
make clean && CC65816_KEEP_IR=$PWD/build/a1_ir make      # the SDK, the library and the corpus with their IR kept
make count-a1 A1_IR=build/a1_ir                          # census of the corpus and of the library
# the two games (private, the gauge's a1s0 folder, run.sh): the frozen sources built again with CC65816_KEEP_IR
# (ROMs cmp-identical to ref1114), trace.py ref1114 sb x2, then a1_count.py --casm-dir <game> --casm-dir lib/build/lorom
# --ir-dir <game IR> --ir-dir <SDK IR> --rom --sym --trace [--frames A-B for one window] --json
```
