# L4 S0: the count before any code (static frames, three edge models)

Step S0 of `L4_plan_decoupe.md`, with the contest's changes 1 (three edge models, structural part apart) and 9 (the
stop rule as rewritten). A count: `QBE_SFRAME_DUMP=1` writes comment lines only, the ROMs do not change.

## Prediction of (d), written 2026-11-13 before the count (committed alone, before any dump or script exists)

Nothing below is measured. The figures are M1 / M2 (`L4_frames_and_calls.md`, build `l2`, before the LF+M5 lot) split by
M1's proportions; each step of the arithmetic says so.

**The rule (contest point 9)**: the lot stops if *structural part + half of the access-cycle part*, under the edge model
actually delivered (S1 delivers the library closure table, model 2) and net of refusals, is under 2.0 % of busy time on
both games.

Split of M2's accepted figure (part 1 with parameters static, 2.56 / 2.88 %) by what M2 measured and M1's proportions:

| % of busy, M2 model, accepted candidates | game 1 | game 2 | source |
|---|--:|--:|---|
| access cycle of temps (6 mclk an `n,S` access) | 0.74 | 0.95 | M2 `accepted_by_kind`, measured |
| access cycle of parameter reads (change 3) | 0.18 | 0.19 | M2, measured |
| `ldx d` + `inc d` (S4's text rules) | ~0.69 | ~0.75 | remainder split 1.04 : 1.43 (game 1), 1.26 : 1.65 (game 2) as M1: hypothesis |
| frame made and undone | ~0.95 | ~0.99 | same split: hypothesis |
| caller's stack restore (change 3) | 0.31 | 0.38 | M2, measured |
| **structural (frame + restore)** | **~1.26** | **~1.37** | |
| **half of the access cycle (temps + parameters)** | **~0.46** | **~0.57** | |
| **rule figure under M2's approximation** | **~1.7** | **~1.9** | |

Then the corrections, in the direction each one moves:
- **Model 2 refuses more than M2's 1 of 143 / 1 of 233** (H4, and Fable's count on the examples: 85 of 98 static non-leaf
  functions call the library or assembly directly). Hypothesis: the closure table accepts most of the library's leaves
  and DMA / sprite helpers, and refuses what reaches the object engine, the scene stack, the game loop and anything with a
  callback; on the games, refusals take **20 to 50 %** of the gain. Under model 1 (coarse edge) **most of the gain goes**
  (over 80 % refused on each game).
- **The LF+M5 lot took temps off the stack** (`tcc__lf` 32 -> 40 bytes): the candidates' stack traffic can only shrink.
- **The window fit**: the deepest chain with parameters, predicted at 120 to 165 bytes (game 1) and 100 to 135 (game 2)
  (M2's 136 / 104 temps, minus what LF took, plus 10 to 30 bytes of parameters). Over 88 on both: the reorder (S2b) would be
  mandatory (contest point 8). Hypothesis H1: game 1 does not link SNESMOD (it has its own audio code), game 2 unknown.
- **RAM**: first delivery (NMI page = the whole window) costs twice the window in the plain band, about 200 to 330 bytes a
  game.

**Prediction of (d): the lot stops after S0.** The rule figure is under 2.0 % on both games already under M2's optimistic
model (about 1.7 and 1.9), and every correction lowers it; under model 2, predicted **1.0 to 1.5 % (game 1) and 1.1 to
1.7 % (game 2)**; under model 1, under 0.5 % on both. What would contradict it: model 2 refusing under 10 % of the gain AND
the frame part being larger than M1's proportions say (the split above is the weakest step).

## Result (2026-11-13, measured after the prediction above was committed)

**Verdict of the rule: the lot does not stop, by 0.07 point on one game, and the figure is not robust.** Under the model
S1 delivers (strict closure table), structural + half of the access cycle is **1.71 % (game 1) and 2.07 % (game 2)**;
the rule stops only under 2.0 on both. The prediction (stop) is contradicted, narrowly. Two facts weigh against reading
2.07 as a clear pass: game 2's four gauge windows give 2.51 / 2.61 / 2.43 / **1.02** (game 1: 1.07 / 1.89 / 1.77 / 1.83),
so the scene moves the figure by more than the margin; and the bound of the tail calls point 6 would turn off is 0.04
(2.03 left). The second, measured stop of point 9 (after S3, the gauge must return at least 60 % of S3's prediction) is
therefore the gate that decides; written here: S3's prediction is F1 under the closure model, **1.79 % (game 1) and
2.18 % (game 2)**, so S3 must measure at least **1.07 and 1.31 %**.

### What was built and proven

- `compiler/qbe` (`emit.c` `sframe_dump`, `main.c` `addrtaken_count/name`): with `QBE_SFRAME_DUMP=1`, after each
  function's `.ENDS` (after every text pass, which never see these lines): `; sframe fn NAME export= addrtaken= leaf=
  alloc= dynalloc= framesize= stack= params= lf= large= cand=yes|REASON`, one `; sframe call NAME CALLEE|* ARGBYTES` per IR
  call, and at the end of the unit one `; sframe addr NAME` per address-taken name.
- **ROMs identical, with and without the variable**: the 65 corpus ROMs and their `.sym` built from `develop` (stock),
  from this branch without the variable and with it: `cmp` 65/65 identical, 0 differences; `diff_corpus.py --ref` the stock
  tree: 65/65 MATCH with the variable and 65/65 without. Both games built by the gauge from this branch without and with
  the variable: ROMs identical to each other and to the gauge's `lfm5` build; state identical to `depart` at every step
  (341 and 700 steps).
- `devtools/sframe/sframe_count.py` (generic: candidates, refusals and their path, placement, deepest chain, NMI roots,
  window fits, H5) and `devtools/sframe/test_sframe_count.py` (`make count-sframe`). The game side is private (the gauge's
  traces; `gain.py`, which replays them on the accepted and placed functions).
- Cross-check of the two sources of a function's calls (IR dump against the instruction lines): **0 mismatches** in the
  65 projects and both games; 0 project function without its dump line.
- **Faulted run** (`test_sframe_count.py`, 7 generated programs, PASS): the cross-file re-entry `f -> B.x -> A.e0 -> f`
  refused by all three models when B is C; through an assembly routine that jumps through a pointer, refused by coarse
  and closure (via UNKNOWN), accepted by m2 (blind, as M2 was); through assembly calling `e0` by name, refused by closure
  through the named edge; through a function pointer, refused by coarse and closure, accepted by m2; Fable's one-file
  shape (`f -> e1 -> h`, no re-entry of f): accepted, h placed past f; a re-entry inside one file is a cycle of the file's
  graph (not a candidate); no re-entry: accepted everywhere. With `SFRAME_COUNT_FAULT=no_unknown` the opaque-assembly case
  is accepted by closure (the fixture sees the unknown-edge rule) while the named-edge case stays refused.

### (a) Candidates, refusals and why

| | corpus (65 examples) | game 1 | game 2 |
|---|--:|--:|--:|
| C functions emitted (after inlining) | 260 | 342 | 364 |
| exported / leaf / address taken | 125 / 35 / 13 | 133 / 72 / 1 | 77 / 59 / 1 |
| alloc (share of static non-leaf) | 4 of 87 | 12 of 136 (8.8 %) | 4 of 227 (1.8 %) |
| on a cycle of the file | 0 | 1 | 2 |
| **candidates** | **83** | **123** | **221** |
| candidates with a tail call (point 6) | 5 | 9 | 62 |
| refused, coarse | 77 (93 %) | 99 (80 %) | 102 (46 %) |
| refused, closure (strict) | 4 | 22 | 24 |
| refused, closure + reviewed routines | 0 | 0 | 0 |
| refused, m2 | 0 | 1 | 1 |

- **Coarse**: every refusal goes through UNKNOWN, almost always as `f -> <library call> -> UNKNOWN -> main -> f` (`main`
  is exported). H4 is confirmed for this model, as Fable counted (85 of 98).
- **Closure (strict)**: every refusal comes from one opaque routine: **`memcpy`** (22 in each game; `memmove` 2 more in
  game 2), whose MVN stub is built on the stack and entered by `rtl`; in the corpus, `lzssDecodeRam` (`jmp (tcc__r3h)`).
  Read by hand, each computed jump goes only into the routine's own code (`REVIEWED` in the script, with the reason per
  routine; also `sprite_oamset`'s `_ohr_big` / `_ohr_lo1`, `pea _ohr_lo1-1` / `rts` into its store run). With them closed
  (`closure_rev`), no refusal is left anywhere. Whether S1 delivers such a list, or an annotation in the assembly, is S1's
  decision; the rule is applied on the strict model. A first version of the reader took `pea $0000 / plb / plb` (the data
  bank set to 0) for a computed jump, made `apuUpload` and `tilemapFlushOn` opaque and refused shard_dash's `fade` / `pump`:
  fixed (a `pea` pulled back is not one) before any figure here was taken.
- H4 for the closure model: refused 18 % (game 1) and 11 % (game 2) of the candidates, but **0.01 and 0.06 point** of the
  rule figure: `memcpy`'s callers are cold in the gauge's scenes.

### (b) Gain by model, net of refusals (% of busy time; window of the gauge, 4 x 40 frames)

Busy on the trace: 28 326 368 mclk (game 1), 35 490 726 (game 2). Structural = frame made and undone; with change 3 also
the caller's stack restore and the pushes turned into `sta.b` (a loss: `sta.b` fetches an operand `pha` does not, `pea`
becomes `lda #` + `sta.b`). Access = 6 mclk per executed `n,S` access (12 for `(n,S),y`). S4 (`ldx d`, `inc d`) is not
in the rule. Rule = structural + access / 2 of F1 + change 3, on the functions accepted and placed (unbounded window, and
in 88 B where it differs).

| % of busy | F1 struct + access | F1 + c3 struct + access | **rule** | S4 |
|---|--:|--:|--:|--:|
| game 1, ceiling (every C function) | 1.56 + 1.25 | 2.32 + 1.66 | 3.15 | 1.06 |
| game 1, every candidate, nothing refused | 1.05 + 0.75 | 1.26 + 0.92 | 1.72 | 0.65 |
| game 1, coarse | 0.38 + 0.05 | 0.50 + 0.08 | 0.54 | 0.03 |
| **game 1, closure** | 1.05 + 0.74 | 1.26 + 0.91 | **1.71** | 0.65 |
| game 1, closure + reviewed | 1.05 + 0.75 | 1.26 + 0.92 | 1.72 (1.45 in 88 B) | 0.65 |
| game 1, m2 | 1.05 + 0.75 | 1.26 + 0.92 | 1.72 (1.45 in 88 B) | 0.65 |
| game 2, ceiling (every C function) | 1.75 + 2.84 | 2.63 + 3.22 | 4.24 | 1.23 |
| game 2, every candidate, nothing refused | 1.06 + 1.19 | 1.43 + 1.38 | 2.13 | 0.55 |
| game 2, coarse | 0.37 + 0.11 | 0.55 + 0.24 | 0.67 | 0.09 |
| **game 2, closure** | 1.01 + 1.17 | 1.38 + 1.37 | **2.07** | 0.55 |
| game 2, closure + reviewed | 1.06 + 1.19 | 1.43 + 1.38 | 2.13 (2.11 in 88 B) | 0.55 |
| game 2, m2 | 1.01 + 1.17 | 1.38 + 1.37 | 2.07 | 0.55 |

Closure, the parts of change 3: parameter reads 0.18 / 0.19, caller's restore 0.32 / 0.40, pushes -0.11 / -0.03. Every
window (88, 131, 164) holds the closure model's placement but one function of game 1 at 88 bytes (no measurable gain
lost). Tail calls (point 6): game 2 executes 137 tail calls from accepted candidates to functions that are not, bound
0.04 % if each became `jsl` + `rtl` + a restore (hypothesis: 100 mclk each); game 1, none. The gain is concentrated: in
game 2 the object code's list walker alone gives 0.78 point of 2.75, five functions 2.0.

### (c) Window, NMI, RAM

| | game 1 | game 2 |
|---|--:|--:|
| deepest chain, temps / temps + params, closure | 64 / **90** B | 56 / **80** B |
| the same, closure + reviewed | 98 / 150 B | 90 / 116 B |
| the same with params, coarse / m2 | 28 / 134 B | 50 / 90 B |
| SNESMOD linked (H1) | no | no |
| NMI roots (literal `nmiSet` / `nmiSetLag`) | one, a HUD callback | one (and NULL twice) |
| NMI-reachable accepted candidates, any model | **0** | **0** |
| plain band free today | 1 154 B | 1 297 B |
| RAM, first delivery (window + whole mirror, 2 x chain) | 180 B (15.6 %) | 160 B (12.3 %) |
| the same after S2b (NMI page -76 B) | 104 B | 84 B |
| RAM, NMI confined (no NMI-reachable frame: window only) | 90 B | 80 B |

- Against 88 / 131 / 164: game 2's 80 fits today's hole; **game 1's 90 does not, by 2 bytes**: by point 8, S2b is
  mandatory before S5 (S3's F1 alone needs 64 B and fits). The cost of not doing S2b, measured: one function of game 1
  left on its stack frame, 0.00 point. Point 8 decides by contiguity alone; the conductor may weigh the fact.
- With the reviewed routines closed the chains grow (150 / 116 B): game 1 then needs S2b's 164 (no SNESMOD), placed in
  88 B it loses 0.27 point (1.72 -> 1.45).
- Corpus: deepest chain with parameters 38 B at most (closure); SNESMOD examples 0 / 8 / 0 / 24 B; no NMI-reachable
  candidate in any example, under any model; `irqSet` is called with assembly handlers only (two examples).
- H5 (assembly that moves D and calls C): none. `NmiHandler` (moves D to the NMI page, then `jml [nmi_callback]`: the
  mechanism itself) and `sprite_oamset`'s `_ohr_*` (computed return into its own store run, no C) are the only routines
  that move D and jump indirectly.

### (d) The rule applied

Point 9, delivered model (closure, strict), net of refusals: 1.71 % and 2.07 %: not under 2.0 on both games, **the lot
continues** to S1. With the reviewed routines: 1.72 / 2.13. In the window today's page offers (88 B): 1.71 / 2.07
(closure), 1.45 / 2.11 (reviewed). The RAM cost (point 9 says it enters the rule, without a weight): 160 to 180 B of plain
band in the first delivery, 80 to 90 B with the NMI confined, at most 16 % of the tight game's free band.

Where the prediction went wrong: it expected the closure model to refuse 20 to 50 % of the gain; it refuses 18 % and 11 %
of the candidates but under 3 % of the gain (cold callers). And M1's proportions under-estimated game 2's access part
(1.37 measured against about 1.14 predicted from M2).

### Not measured

- No gain on the examples (no gauge, no trace): (a) and (c) only.
- The windows are contiguous ranges (today's 88-byte hole, and S2b's 131 / 164); placing frames in the holes of today's
  page is not computed. A frame that does not fit is dropped and the rest placed again; which frame to drop by its gain is
  not optimised.
- The pushes' replacement cost and the tail-call bound are cycle models (opcode and operand fetches derived from the
  measured costs), not measured replacements.
- The library's `.c.asm` is LoROM's for every example (HiROM and SA-1 projects read the same call graph).
- The trace windows are the gauge's (4 x 40 frames), and the window-to-window spread is larger than the margin; the busy
  figures carry luna's DRAM-refresh bias (the gauge's notes).
- The second stop of point 9 (after S3) and the tail-call handling of point 6 remain to be measured.

### Gates re-run (2026-11-13, on the normal build, variable unset)

- `make clean && make`: exit 0, the same 4 warning lines as `develop`'s build (clang's `-rdynamic`, make's jobserver).
- `cmp` of the 65 ROMs and `.sym`: stock `develop` = this branch without the variable = with it, 0 differences; no
  `; sframe` line in the normal build's `.c.asm`.
- `diff_corpus.py --ref <stock develop tree>`: `65/65 match, 0 diff, 0 missing` without the variable, and the same with it.
- `make test-compiler`: `150 passed, 0 failed (31/31 refusals pinned)`.
- `make tests`: `ALL CHECKS PASSED (luna)` (coverage 63 OK / 2 INPUT-DEP of 65, both power-ons; torture as expected.txt).
- `difftest.py --seeds 1-2000`: `2000/2000 seeds clean`; `difftest_stmt.py --seeds 1-2000`: `2000/2000 seeds clean`.
- `devtools/verify_toolchain.py`: pins match (qbe `42c86ad`, a local commit on branch `l4-s0-sframe-dump` of the
  submodule, to be pushed to the fork's `feat/b2-far-qualifier` before this reaches `develop`'s remote).
- `make lint-docs`: no drift. `test_sframe_count.py`: PASS.

### Replay

```
QBE_SFRAME_DUMP=1 make                                   # the SDK and the corpus, dump lines in every .c.asm
make count-sframe                                        # faulted run + corpus count
# the two games (private, the gauge's l4s0 folder, run.sh): the gauge built with QBE_SFRAME_DUMP=1
# (jauge.py --sdk <tree> --etiquette l4s0), sframe_count.py on each build, trace.py l4s0, gain.py l4s0 sb|x2;
# GAIN_FRAMES=A-B gain.py ... for one gauge window
```
