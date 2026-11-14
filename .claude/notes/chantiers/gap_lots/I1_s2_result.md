# I1 S2: measure before deciding (no change to the output)

Step S2 of `I1_inline_plan.md` (section 3, S2 in section 5, H1 to H11 in section 7, the Contest). Branch `i1s2`, from
`develop` `90595a10`, QBE at `f2e1f03`. Pair: Opus measures and writes (no sub-agent on this step). Names neutral: jeu 1,
jeu 2, the games. Nothing of the games is versioned here; their data are in the private folder of the gauge (`i1s2`).

## Inputs (read before the prediction, nothing measured yet)

- The games' IR kept by `CC65816_KEEP_IR` on the frozen reference `ref1114` (A1 S0's folder), and the gauge's two private
  builds of I1 S0: `i1-temoin` (unmarked, `cmp`-identical to `ref1114`) and `i1-256b` (146 / 181 callees of at most
  256 bytes marked `static inline`), each with its `.c.asm`, `.sym` and ROM.
- Today's `bin/qbe` compiles both games' IR (22 and 10 units) without an error; its outputs are kept in `/tmp` as the
  reference of the byte-identical gate of this step (with the corpus's and the library's `.asm` and `.sfc`).
- The method: the units' IR is fed to `bin/qbe` with `CC_TRACE_INLINE=1`, so the trace and the text come from the same
  compile. Sizes in bytes are the text estimate of `i1_count.py` (within 0.2 % of the ROM placement on the placed
  functions, I1 S0). Counts reuse `i1_count.py` on the `ref1114` traces, as S0 did.

## Prediction (written 2026-11-14 before any measurement; committed alone)

1. **Output byte-identical.** The trace prints on stderr only: every `.asm` of the corpus, of the four library object sets
   and of both games' units (`bin/qbe` on the kept IR) and every `.sfc` of the corpus `cmp`-identical to the reference
   taken before the change.
2. **The decline reasons (H2, H8).**
   - The trace's one-site declines (static, address not taken, one direct site, not absorbed) number **10 callees on
     jeu 1 and 101 on jeu 2**, S0's figures from the text.
   - **H2 holds**: the reason is the temporary room (`caller->ntmp + callee->ntmp >= 1800`) for **at least 90 %** of the
     111, on jeu 2 nearly all (its two big callers). Jeu 1's ten may mix reasons (I do not know its unit).
   - **H8, my prediction, against the plan's**: before SSA (the inliner runs on the IR as parsed, before the pipeline),
     cproc gives each value a fresh temporary and a splice clones every temporary of the callee once, so few of the
     counted temporaries are dead. **Used / counted between 0.80 and 0.98** in the two big callers of jeu 2: H8 does
     not hold, the room is real, and S5's branch is "raise the limit or split", not compaction.
   - **What settles S0's margin hypothesis (0.81 / 0.75 % in one-site callees declined today)**: if H2 holds, those
     callees were declined in the recompiled check `i1-256b` too (marking `static inline` does not move a one-site
     callee's room test). Prediction: **every one of them still has its `jsl` in `i1-256b`**, and the count of S0 minus
     their share (2.47 - 0.81 = 1.66 %, 2.35 - 0.75 = 1.60 %) is **within 15 % of the measured 1.80 / 1.71 %**. If so, the
     0.73 measured/count ratio of S0 is mostly this, not the register cost the count is blind to.
3. **Calibration of `N_SIZE` (H1, H5, H11)** on both games, the functions that absorbed nothing (own size = final size):
   bytes per IR instruction, median **3.0 to 3.6**; the `N` that best separates 256 bytes in **60 to 90**; at `N = 72`
   wrong takes ≤ 5 % and misses ≤ 15 % of the 256-byte set. On the candidates (static, address not taken, two sites or
   more), own size against final size: the final IR size refuses **at most 10 callees per game** that the own size takes
   (H5: little lost). Free copies (`own <= 6`): **at least 80 %** of them emit no more bytes than a call sequence (12).
   Callees holding a jump table: bytes per IR instruction **30 to 60 % above** the median (the table is not in the IR).
4. **Growth per unit (H3)** from `i1-temoin` to `i1-256b` (`.sym` code sections by unit): jeu 1 no unit above 32 KB;
   **jeu 2: its object unit above 32 KB (40 to 80 KB)**, so H3 fails on jeu 2 and the unit budget binds there. Per
   caller: at least one caller of jeu 2 grows by more than 8 KB; no function of jeu 1 reaches 16 KB.
5. **The IR criterion's set against the 256-byte set (H5, the count used only as a ratio)**: the plan's size rule takes
   **no one-site callee** (walk 1 keeps them), and the 256-byte set holds the declined ones. So the ratio of the counts
   (nested, lower bound) is **0.60 to 0.80** on each game, not the plan's ≥ 0.90; without the one-site callees on both
   sides, **≥ 0.90**.
6. **Coherence**: the trace's declined same-unit call sites (every call to a function of the unit still a call) equal
   the text's same-unit `jsl` + `jml` of today's compile; today's text on the `ref1114` IR gives S0's 928 + 18 (jeu 1)
   and 1 455 + 103 (jeu 2) sites, give or take the tail calls of the backend changed since; the executed same-file
   `jsl` on the `ref1114` traces stay 6 248 / 7 913 (A1 S0, I1 S0).

**The stop rule of S2 (from the mission)**: if the calibration shows that the plan's criterion cannot fit a game's ROM
budget (a unit beyond 32 KB with no refusal possible), the note says "the plan is to be revised" and proposes the change.
My prediction (point 4) is that the unit budget binds on jeu 2. The budget refuses by construction, so the question
becomes what the refusals cost: answered below with the figure, not assumed.

## Result (2026-11-14, measured after the prediction above was committed, `c3cae250`; qbe `7bd8684`)

**Verdict of S2's stop rule: the plan is to be revised, on its unit budget and on the S4 prediction.** The size rule can
fit the games' ROM (the budget refuses by construction, and the 8 KB per caller and 16 KB per function never bind), but
the default of 32 KB per unit binds on jeu 2 by a factor of two in a unit where the game has room, and with a priority
that does not know the profile it then throws away a third of jeu 2's gain. The measured ratio of the IR criterion's set
to S0's 256-byte set is 0.68 / 0.73, not the plan's 0.95. Section "What the plan changes" says how.

### What was built (no change to the output)

- `CC_TRACE_INLINE=1` prints, besides the existing lines, one `decline` line per call that `auto_inline_ok` or
  `splice_fn` refuses (reason = the first test that failed; `pass=1` before the callee's own inlining, `2` after; the
  temporaries counted and those still used, in caller and callee; `rest` = the reason with the site count left out) and
  one `final` line per function after its walk (its IR size once its own copies are in, sites, static, address taken,
  section). Byte-identical output with and without the variable, and before and after the change (gates below).
- `devtools/sframe/i1_calib.py` (`make count-i1-calib`): `compile` runs `bin/qbe` with the trace on every kept IR file
  (trace and text from the same compile), `report` reads them (reasons, room, bytes per IR instruction, separators, free
  copies, jump tables, the size rule's candidates and `--write-set` / `--write-final-set` for `i1_count.py --set`, the
  plan's per-unit estimate), `growth` reads two `.sym` per unit and per function.
- Private (the gauge's `i1s2` folder): the games' kept IR (`ref1114`) compiled by today's `bin/qbe`, the replay of S0's
  recompiled check at the IR level (`mark_ir.py`: the same 146 / 181 functions marked `inline`, `CC_INLINE_MAX_BIG=100000`),
  the counts of every set (`count_set.sh`, `i1_count.py` unchanged), `missing.py`, `budget.py`, `coherence.py`.
- The corpus and the library: the 104 kept IR units of the A1 S0 build, compiled by today's `bin/qbe`.

### (5) Coherence first

- S0 reproduced to the digit with today's tree: the 256-byte sets count **2.465 % and 2.350 %**, executed same-file `jsl`
  **6 248 / 7 913**, M1's rule 6 247 / 7 909.
- Today's text on the `ref1114` IR: same-file `jsl` + `jml` of the projects' units **788 (jeu 1) and 1 400 (jeu 2)**; with
  the library's 151 + 7 (S0's census), **946 = 928 + 18 and 1 558 = 1 455 + 103**, S0's figures exactly.
- The trace against the text: jeu 1, 543 declines in emitted functions + 245 in absorbed ones (their clones carry the
  calls; the absorber resumes after the clone) = **788 exactly**; jeu 2, 1 029 + 376 = **1 405 against 1 400**: one decline
  too many for four callees of one unit (each has one call fewer in the text than in the trace). Hypothesis, not
  checked: calls the optimizer removes after the walk.
- The one-site declines of static, address-not-taken callees: **10 and 101**, S0's figures.

### (1) Why the one-site callees are declined (H2, H8, the 1 800 margin)

| | jeu 1 | jeu 2 |
|---|--:|--:|
| one-site declines | 10 | 101 |
| reason `tmp room` | 7 | 97 |
| reason `alloc` (an alloc in the IR as parsed) | 3 | 4 |
| test that refused | all before the callee's own inlining (`pass=1`) | the same |
| declining callers | 4 (one unit), `ntmp` 759 to 1 570 | 4 (one unit); one caller has 81 of the declines at `ntmp` 1 719 to 1 735 |
| used / (counted - 64), callers | 0.956 to 0.981 | 0.965 to 0.980 |
| used / (counted - 64), callees | 0.94 to 0.99 | 0.93 to 1.00 |
| would pass with the callee's 64 register temporaries counted once | 4 of 7 | 39 of 97 |
| would pass with both caller and callee compacted to their used temporaries | 4 of 7 | 57 of 97 |

- **H2 holds: 104 of 111 (94 %) are the room.** The other 7 are an alloc in the IR as the inliner sees it (before
  promotion): S0 read "no eligible callee keeps an alloc" in the emitted side table, which is written after promotion.
  The same reason refuses 27 multi-site statics of jeu 1 (7 in the corpus) for the size rule.
- **H8 does not hold** (my prediction; the plan's was the opposite): 96 to 98 % of the counted temporaries are used. The
  inliner runs on the IR as parsed, where cproc gives each value its own temporary and a splice clones each once.
- **Found, an exact over-count**: the room test adds `callee->ntmp`, which includes QBE's 64 register temporaries
  (`Tmp0`), but `splice_fn` clones only from `Tmp0` up. A test counting `callee->ntmp - Tmp0` (what the splice adds)
  would have let 4 of 7 and 39 of 97 through at the moment of their test (a snapshot: the caller grows with each
  absorption, so the sequential figure is lower, not measured).
- **S0's margin hypothesis (0.81 / 0.75 % in one-site callees declined today) and the 0.73 ratio**: my prediction said
  the 0.73 measured/count ratio was mostly those callees. **Wrong.** None of jeu 1's ten is in its 256-byte set (all are
  above 256 bytes), and jeu 2's set holds 46 of the 101 for **0.172 %** of its 2.350 % count; 98 of the 101 still have
  their `jsl` in `i1-256b`. The count minus that share (2.47, 2.18) is not within 15 % of the measured 1.80 / 1.71.
- **What explains part of the ratio instead, measured**: S0's recompiled check **lost one-site absorptions that today's
  compiler makes**: 14 functions on jeu 1 and 42 on jeu 2 have a standalone in `i1-256b` that `i1-temoin` does not
  (23 206 and 56 370 bytes re-emitted). The IR replay of the check reproduces `i1-256b`'s emitted functions (225 against
  227, 294 against 292: the header helper and two cold functions apart) and gives all 56 the reason `tmp room`: in one
  walk, copies met earlier in a caller's block order took the room that the one-site absorption needed. That is exactly
  the plan's argument for walk 1 before walk 2 (1.2, H4). It is not priced (those functions have no `jsl` in the
  reference text to price): **H4 now has a measured object**, and S4's `order` fault has its expected direction.

### (2) The size threshold on the games (H1, H5, H11)

Bytes per IR instruction, functions that absorbed nothing (own size = final size; the text of today's backend):

| | n | quartiles (bytes / IR instruction) |
|---|--:|---|
| corpus + library | 464 | 2.56 / **3.36** / 4.40 |
| jeu 1 | 285 | 2.11 / **2.94** / 3.88 |
| jeu 2 | 327 | 2.06 / **2.63** / 3.41 |

Separator `own <= N` against "at most 256 bytes" (taken right / taken wrong / missed):

| N | corpus | jeu 1 | jeu 2 |
|--:|---|---|---|
| 60 | 353 / 1 / 38 | 199 / 0 / 36 | 161 / 2 / 74 |
| 72 | **376 / 5 / 15** | **210 / 5 / 25** | **191 / 7 / 44** |
| 80 | 380 / 10 / 11 | 218 / 7 / 17 | 203 / 8 / 32 |
| 90 | 385 / 20 / 6 | 221 / 10 / 14 | 208 / 11 / 27 |
| 100 | 387 / 28 / 4 | 228 / 16 / 7 | 218 / 16 / 17 |

On the size rule's candidates (static, address never taken, no section, two sites or more, no `inline`, not recursive,
every site declined for its count alone: 188 / 183 on the games, 139 in the corpus), against their FINAL bytes:

| N = 72 | own size: right / wrong / missed | final size: right / wrong / missed | own <= 72 < final |
|---|---|---|--:|
| corpus | 119 / 4 / 5 | 119 / 2 / 5 | 2 |
| jeu 1 | 122 / 5 / 21 | 118 / 1 / 25 | 8 |
| jeu 2 | 112 / 8 / 22 | 111 / 6 / 23 | 3 |

- **H1 holds on the corpus, not on the games**: 72 separates 256 bytes on the corpus (the plan's reading, again), but the
  games' code is denser per IR instruction (2.94 and 2.63 bytes, against my 3.0 to 3.6 and the plan's 2.8 to 3.8, jeu 2
  under both), and their best separator is **N = 80 to 90 (jeu 1), 90 to 100 (jeu 2)**, against my 60 to 90.
- **H5 settled, own size**: the final size refuses only 8 / 3 more callees, as predicted (≤ 10), but they weigh: the count
  of the final-size set is **1.362 % against 1.685 % (jeu 1, -19 %) and 1.512 against 1.726 (jeu 2, -12 %)**. Own size is
  the right criterion; the plan keeps it.
- **H11, free copies, contradicted**: of the functions with `own <= 6`, the copy (body without head, tail and `rtl`)
  fits a call sequence of 12 bytes for **15 / 24, 16 / 24 and 35 / 63** (56 to 67 %, against my ≥ 80 %). Median copy by
  own size: 1 → 3 to 4 bytes, 2 → 8, 3 → 10 to 16, 4 → 11 to 16, 5 → 12 to 22, 6 → 20 to 30.
- **H11, jump tables, contradicted**: the 6 / 5 / 6 functions holding a table have 2.62 / 2.84 / 2.70 bytes per IR
  instruction, not above the medians (my 30 to 60 % above was wrong): the tables are small next to the compare trees'
  instructions that the IR counts. No correction needed.

### (3) Code growth per unit, `i1-temoin` -> `i1-256b` (H3)

| | total `.text` | largest unit | second | functions grown > 8 KB | sections > 16 KB after | largest function after |
|---|--:|--:|--:|--:|--:|--:|
| jeu 1 | +47 177 | +22 588 | +14 810 | 0 | 0 | 5 046 |
| jeu 2 | +145 630 | **+124 410** | +12 471 | 0 | 0 | 6 074 |

- **H3 fails on jeu 2**: one unit grows by 124 KB (my prediction 40 to 80 KB, wrong upward). Jeu 1 stays under 32 KB (as
  predicted). The largest per-function growth is 5.1 KB, and it is a re-emitted standalone (above), so no caller of either
  game grows by 8 KB (my "at least one caller of jeu 2 over 8 KB": wrong). Nothing approaches 16 KB.
- The plan's estimate (1.3, `bpi` 3.24, nested, no room test) for the IR criterion at N = 72: jeu 1 at most 7.3 KB a unit
  (31.6 KB in all), **jeu 2's same unit 76.6 KB** (85.7 KB in all), corpus 23.0 KB over 52 units (the library 8.9 KB of it,
  counted once: H10's +27 KB is the same order). The count's nested ROM for that set: +48.9 / +96.8 KB (on S0's text;
  S0's measured/count ROM was 0.74 / 0.92).

### (4) The IR criterion's set against S0's 256-byte set (the count used only as a ratio)

| set (count of I1 S0, nested, lower bound) | jeu 1 | jeu 2 |
|---|--:|--:|
| S0's 256-byte set (147 / 181 callees) | 2.465 %, +63.9 KB | 2.350 %, +159.0 KB |
| **IR criterion, own <= 72** (127 / 120) | **1.685 %, +48.9 KB** | **1.726 %, +96.8 KB** |
| ratio to the 256-byte set | **0.683** | **0.734** |
| final size <= 72 (119 / 117) | 1.362 % | 1.512 % |
| own <= 100 (153 / 138) | 1.891 %, +140.8 KB | 1.968 %, +142.2 KB |
| own <= 128 (164 / 151) | 1.988 %, +165.6 KB | 2.603 %, +211.1 KB |
| own <= 200 (178 / 170) | 3.512 %, +373.3 KB | 3.440 %, +646.8 KB |

- The ratio falls in my range (0.60 to 0.80), against the plan's ≥ 0.90, **but for a reason I had not seen**. What the
  256-byte set holds and the IR criterion misses: jeu 1, 22 callees of own size above 72 for **0.943 %** (one of them,
  190 IR instructions for 253 bytes, holds **0.642 %** alone) and 3 refused for an alloc (0 %); jeu 2, 22 above 72 for
  **0.613 %** and the 46 one-site callees for 0.172 %. Without the one-site callees on both sides jeu 2 reads 0.79, not my
  ≥ 0.90. The IR criterion also takes 5 / 7 callees outside the 256-byte set (0.163 / 0.162 %).
- **The gain is concentrated in a few callees that an IR size does not find**: bytes per IR instruction run from 0.8 to 11
  per function, and the most valuable callees of both games are dense (1.3 bytes per IR instruction for the one above).
  Raising N buys them at a steep ROM price: N = 100 adds 0.21 / 0.24 points for +92 / +45 KB (count ROM).

### The budgets where they bind (H3, H7)

Jeu 2's candidates (N = 72) in its large unit cost about 68 KB nested (the count's one-level bytes per callee times the
set's nested / one-level ratio, 1.8: an estimate, not recompiled text). Admitted under a 32 KB unit budget:

| order of admission (`budget.py`) | jeu 2's unit | jeu 2, whole count |
|---|--:|--:|
| everything (no budget) | 0.832 % | 1.726 % |
| profile order (gain per byte; no compiler knows it) | 0.826 % | 1.721 % |
| size order (smallest cost first, a static proxy) | **0.225 %** | **1.119 % (-35 %)** |
| either order under 64 KB | 0.832 % | 1.726 % |

Jeu 1 never reaches the budget (largest unit +11 KB one level). The plan's score (loop depth / cost) sits somewhere
between the two orders; the trace has no loop depth, so **H7 is not settled here**, and the stake is now known: up to
0.6 points of jeu 2's count.

## What the plan changes (written here before S4, as S2's output requires)

1. **`UNIT_BUDGET`: 32 KB is to be revised.** Its premise (one LoROM bank) does not hold for code: each function is its own
   `SUPERFREE` section, a unit is never placed as a whole, and S0's check put +124 KB into one unit of jeu 2 with no
   function above 6.1 KB and the game at 1.58 MB free. **Proposal: 131 072 estimated bytes (four banks)**: it admits jeu 2's
   N = 72 set (76.6 KB estimated, about 68 KB by the count) with margin, and still refuses the runaway nesting of a large
   N (N = 200 estimates 357 KB in that unit). The per-caller 8 KB and the 16 KB per function stay: neither binds anywhere
   (largest growth 5.1 KB measured, 3.4 KB estimated).
2. **`N_SIZE` stays 72.** The games' separator for 256 bytes is 80 to 100, but 256 bytes was the proxy, and the gain per
   ROM byte beyond 72 is poor on both games (above). Their best callees are not reachable by any IR threshold under 190.
   A better byte estimate per instruction class (or a profile) would be a lot of its own, not this one.
3. **`CC_INLINE_BPI` stays 3.24** (the corpus's 3.36 today): on the games it overstates bytes by 10 to 25 %, which errs on
   the safe side for a budget.
4. **Free copies: `own <= 2`, not 6** (copy ≤ 12 bytes for own 1 and 2 on the three sets, two functions apart, 19 and 20
   bytes; from 3 up the median passes 12 somewhere).
5. **S4's prediction is rewritten with the measured ratio**: r = 0.683 / 0.734 instead of 0.95, so **-1.23 % (jeu 1) and
   -1.26 % (jeu 2)** busy per step (1.80 × 0.683, 1.71 × 0.734), range -0.9 to -1.6. Plus, hypothesis, walk 1 before walk 2
   gives back part of the 14 / 42 one-site absorptions that S0's check lost (positive, unpriced). With the 32 KB budget and
   a priority close to size order, jeu 2 would fall to about -0.8 % (1.26 × 1.119 / 1.726), under the 1.02 % bar; jeu 1
   would carry the lot alone. That is the case for change 1.
6. **S5's branch: H2 holds, H8 does not, the room is real.** S5 starts with the exact accounting (`callee->ntmp - Tmp0` in
   the room test: what a splice adds; 4 of 7 and 39 of 97 pass at their test), measured sequentially, before any raise
   of `MAX_ALLOC_TEMPS`. Its ceiling stays S0's 0.81 / 0.75 % (count), of which only 0.172 % (jeu 2) lies in callees of
   256 bytes or less.
7. **Left for later, not S4**: the alloc seen before promotion refuses 3 + 4 one-site callees and 27 multi-site candidates
   of jeu 1 (7 in the corpus) whose emitted table says `alloc=0`.

## Against the prediction

| point | predicted | measured | |
|---|---|---|---|
| 1. output | byte-identical | byte-identical (gates below) | held |
| 2. one-site declines | 10 / 101 | 10 / 101 | held |
| 2. H2 | ≥ 90 % room | 94 % | held |
| 2. H8 | used / counted 0.80 to 0.98 | 0.93 to 1.00 (0.96 to 0.98 in the callers) | held (the plan's H8 fails) |
| 2. the margin explains the 0.73 | count minus share within 15 % of measured | jeu 1 nothing to subtract, jeu 2 0.172 % | **wrong**; the lost one-site absorptions are a measured part instead |
| 3. bytes per IR | median 3.0 to 3.6 | 2.94 / 2.63 | **wrong** (low) |
| 3. N for 256 B | 60 to 90 | 80 to 90 / 90 to 100 | half wrong (jeu 2) |
| 3. N = 72 errors | wrong ≤ 5 %, missed ≤ 15 % | wrong 2.3 / 3.5 %, missed 10.6 / 18.7 % | jeu 2 misses more |
| 3. H5 | final refuses ≤ 10 | 8 / 3 (but -19 / -12 % of the count) | held, weight not foreseen |
| 3. free copies | ≥ 80 % | 56 to 67 % | **wrong** |
| 3. jump tables | 30 to 60 % above | not above | **wrong** |
| 4. jeu 2's unit | 40 to 80 KB | +124 KB | **wrong** (upward); H3 fails as predicted |
| 4. a caller over 8 KB (jeu 2) | yes | no (max 5.1 KB) | **wrong** |
| 5. ratio | 0.60 to 0.80 | 0.683 / 0.734 | held, reason wrong |
| 5. ratio without one-site | ≥ 0.90 | 0.79 (jeu 2) | **wrong** |
| 6. coherence | exact | exact (jeu 2's trace +5, explained as a hypothesis) | held |

## Gates

- **Byte-identical, corpus and library**: after `make clean && make` with qbe `7bd8684`, the 1 049 files of the
  reference taken before the change (65 `.sfc`, the examples' `.asm`, the `.asm` of the four library object sets) are
  `cmp`-identical, 0 differ. The same 5 warning lines as S1's build (clang `-rdynamic`, CMake deprecation, three `-j`).
- **Byte-identical, games**: today's `bin/qbe` on the 32 kept IR units of both games, before the change, after it without
  the trace and after it with `CC_TRACE_INLINE=1`: 32 / 32 `.asm` `cmp`-identical (the games' ROMs are not rebuilt by this
  step: their `.asm` are what the change could touch).
- `make tests`: `ALL CHECKS PASSED (luna)`. `make test-compiler`: `155 passed, 0 failed (34/34 refusals pinned)`.
  `make lint`: exit 0. `make verify-toolchain`: `OK` (PINS.md names `7bd8684`, 156 patches).
- **Sanitizers on the new code path**: QBE built with ASan + UBSan (`-fno-sanitize-recover=all`, `halt_on_error=1`) and
  run WITH the trace on the games' 32 units, the 104 corpus and library units and the replay of S0's check (10 units):
  0 failures, 0 reports, the games' 32 `.asm` identical to the normal build's. `make test-sanitizers` (which runs the
  corpus without the trace, so never reaches the new lines) and the differential hunts were not run: the output is
  byte-identical by the gates above. qbe `7bd8684` is on the submodule's local branch `i1s2`: it must reach
  the fork before `develop` can carry it.

## Replay

```
make clean && CC65816_KEEP_IR=$PWD/build/a1_ir make                # the corpus's and the library's IR
make count-i1-calib A1_IR=build/a1_ir                              # reasons, calibration and estimate, corpus + library
# the games (private, the gauge's i1s2 folder):
#   python3 devtools/sframe/i1_calib.py compile --qbe bin/qbe --ir-dir <a1s0 IR of a game> --out <dir> --short
#   python3 devtools/sframe/i1_calib.py report --dir <dir> --write-set 72 <set> [--write-final-set 72 <set>] [--nsize N]
#   python3 devtools/sframe/i1_calib.py growth --before-sym <i1-temoin .sym> --before-dir <i1-temoin build> \
#       --after-sym <i1-256b .sym> --after-dir <i1-256b build>
#   sh count_set.sh <game> <set> <out.json>          (i1_count.py --set on ref1114, as S0)
#   python3 mark_ir.py <set> <IR dir> <out>; CC_INLINE_MAX_BIG=100000 i1_calib.py compile ...   (S0's check, replayed on IR)
#   python3 missing.py <game> N; python3 budget.py <game> <set> <budget> [nesting factor]; python3 coherence.py <dir>
```
