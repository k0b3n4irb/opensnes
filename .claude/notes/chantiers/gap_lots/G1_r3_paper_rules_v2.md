# G1 R3-0: paper rules, revision 2 (2026-11-15), frozen before the second paper of `path`

Decision of the conductor (thread `2026-11-15-g1-r3-quelle-ligne…`, 007). Committed alone, before any paper text. It **adds** to
`G1_r3_paper_rules.md` (every rule of that file stays) the exact IR idioms below; the first paper of `path` stays on file
(`g1_r3_0/path_paper.asm`), unchanged. The second paper is written by an agent that is not the author of these rules, from this
file, the first rules file and `g1_r3_0/path_ir.ssa` only (not the attribution, not the hand, not the first paper).

## Idioms admitted (each exact on the whole domain of the IR; applied once, before allocation, for both emitters)
- **I1 narrow sign-extension chain**: a 32-bit value that is the sign extension of a 16-bit one keeps its high word as the sign
  mask (`0` or `$FFFF`) built once; `sar` by 1 of it has that mask as its high word. No range fact used.
- **I2 scaled induction variable**: a variable only used as `2·v` (a table index) may be carried scaled, its step scaled
  and its wrap mask scaled, the entry value scaled with the IR's own 32-bit rules (no assumption on its range).
- **I3 loop rotation** of a loop whose trip count is a constant at least 1 (test at the bottom).
- **I4 induction reversal** of a counter used only for the trip count (counted down to zero).
- Already admitted: unsigned range test, `cond ? k : 0`.

## Not admitted (any rule that depends on a value range or on data the IR does not carry)
- `2·s` or any widening whose high word is taken as the sign mask because \|s\| is small: **F, refused** (decision 007).
- Replacing `hi | sext(lo)` by a plain copy because no `lo` is negative: **P, refused** (data-conditional; the first paper's
  select stays).

## What is written before the text (thresholds, decision 007)
1. Second paper of `path`: **at most 1.20 of the hand** (SlowROM and FastROM both reported, the decision reads the build speed of the games).
2. Gap with "today's generator + the same idioms" (the C of `path` rewritten once to say I1, I2, I3, I4 and compiled by the current
   build, checksum equal, measured on the same bench): **at least 0.05 of ratio to the hand**.
Both hold: R3-1. Above 1.20, or a gap under 0.05: G1 stops and the conductor takes the report to the owner.
Reference points from the attribution (not given to the agent): 1.18 with the idioms, 1.115 with the index in X.
