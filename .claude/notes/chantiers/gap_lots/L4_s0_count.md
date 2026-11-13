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
