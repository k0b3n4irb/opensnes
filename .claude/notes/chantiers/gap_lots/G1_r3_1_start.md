# G1 R3-1, first step: the IR idiom passes and the threshold-2 reading (2026-11-15)

Decision of the conductor (thread `2026-11-15-g1-r3-quelle-ligne…`, 009). One conversation per lot; Sonnet executes, Opus reviews
(Class A: a worktree started with `git checkout -B work develop`, every gate replayed by the session before merging).

## Order (nothing of the allocator before step 3)
1. **The IR passes of the admitted idioms E, B1, C1, C2** (rules `G1_r3_paper_rules_v2.md` I1-I4), written once, behind one switch,
   for both emitters; each with its test and an injected fault; with the switch off the output is byte for byte the same.
   They stand on their own: if G1 stops, they go to the current generator.
2. **I5, the two-entry loop** (written here before any code): when the first table address is `base + 2·idx16` with `idx16` an
   unbounded 16-bit value (the 32-bit extension can cross the bank), the first iteration is peeled mechanically: the carry out of the
   scaling picks the second form of the first load, and the loop is entered after that load. Admitted only if the pass produces
   it from the IR alone, with no range fact. Otherwise it stays out of the paper, as F.
3. **Today's generator measured on the passes' output**, on the decided cell (real leaves' shape, "with idioms" line, static
   weight; the twin `path` for now): the gap to paper 2 (`g1_r3_0/path_paper2.asm`, 1.120 / 1.122) is threshold 2. **At least 0.05
   of ratio to the hand: R3-2 (allocator). Under 0.05: G1 stops, the conductor takes the report to the owner.** Nothing reopens
   at the sight of the figure.
Bench for the twins: rows `path`, `pathpp`, `pathp2` in a scratch copy (`g1_r3_0/` holds the C, hand and papers; the bench patch is
rows in `kernels.c/asm` named so, `run.py` pinned to the repo).
