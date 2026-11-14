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
