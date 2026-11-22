# G1 R3-1: the idioms I1 to I5 as IR passes, and the threshold-2 reading (2026-11-21)

Passes on `develop`: I1 to I4 `8abed7f7` (qbe `830854b`), I5 `b71d49c7` (qbe `b4b894f`), all behind `OPENSNES_G1_IDIOMS=1`, off by default,
byte-identical off. Gates replayed by the session on each: `make tests`, `diff_corpus` 66/66 at tolerance 0, `difftest` 16/16,
`difftest_stmt` gate and a 100-seed hunt, fixture `g1_idioms` (the agent's 2000-seed hunt of I5 and its five injected faults are
the agent's, not replayed).

## Threshold 2 (measured by an Opus agent, every figure run twice, equal checksums; NOT replayed by the session)
Twin `path`, master cycles per call, SlowROM / FastROM, bench copy `$CLAUDE_JOB_DIR/tmp/scratch5` (disposable, 62 rows + 3, `fillu` and `coli` rows dropped for the 64-row limit):

| | SlowROM | FastROM | ratio to the hand (Slow / Fast) |
|---|--:|--:|--:|
| C, switch off | 124 223 | 106 084 | 1.520 / 1.523 |
| C, I1-I4 | 137 308 | 118 113 | 1.680 / 1.695 |
| C, I1-I5 | 144 294 | 124 241 | 1.765 / 1.783 |
| paper 2 | 91 545 | 78 178 | 1.120 / 1.122 |
| hand | 81 742 | 69 671 | 1 |

Gap "today's generator + idioms" to paper 2: **0.645 / 0.661** (I1-I4: 0.560 / 0.573). By the letter of the rule (gap >= 0.05) R3-2 (the
allocator) is admitted. **The finding the rule did not foresee: the idioms as IR passes make this C SLOWER than without them**
(+13 085 cycles for I1-I4, +6 986 more for I5, code 314 -> 372 -> 442 bytes). I5 reserves 6 bytes of frame (`sta 2,s`, `lda 2,s` in
the loop); I1-I4 carry the index in 32 bits, masks on both words, `lsr/ror` to rebuild it, accumulators in slots. Hypothesis, not
attributed post by post: the passes are only worth what the allocator keeps in registers; without R3-2 they pay their cost and not
their gain. So the switch stays off and nothing of this reaches the corpus; the idioms "stand on their own" claim of step 1
(`G1_r3_1_start.md`) is NOT confirmed by this twin. The decision on R3-2 (and on keeping the idioms) is the conductor's / owner's.

## Replay and attribution by post (2026-11-21, decision of the conductor, condition 2)
The session replayed the bench: 124 223 off / 137 308 I1-I4 / 144 294 I1-I5 SlowROM, 106 084 / 118 113 / 124 241 FastROM, equal to the cycle.
An Opus agent then edited the "on" listing of `path` by hand, one post at a time, checksum 0x4af9 each time, every figure run twice
(not replayed by the session; the hand-edited listings are in `$CLAUDE_JOB_DIR/tmp/scratch6`, disposable). Regression = on - off = 20 071 Slow / 18 157 Fast.

| post (alone) | SlowROM gain | share of the regression |
|---|--:|--:|
| V1 the high word of the scaled index dropped (the final `and #126` keeps only the low word) | 17 288 | **86 % / 82 %** |
| V5 the end-of-loop phi copies (lf+4 -> lf+26) | 8 306 | 41 % (4 155 once V1 is in) |
| V2 stack frame of 6 bytes to direct-page slots | 939 | 5 % |
| V3 branch-free sign mask | 116 | 0 % (no 2-instruction exact form exists; proof exhaustive on u16) |
| V4 copies lf+32/34, lf+36/38 | 12 660 | not a regression post: the "off" listing has the same copies |
V1+V5: 122 851 / 105 632, under the C without idioms (1.503 of the hand). All five: 109 539 / 94 056 = 1.340 / 1.350: gap to paper 2 still 0.220 / 0.228.

Reading: ONE post explains the regression and does not need the allocator: a demanded-bits narrowing pass after I2 (the scaled index is read
only through `& 126` and a u16-truncated `>> 1`, so its high word, the high word of the doubled step, the `and #0` and the exit `ror` are dead; phi, add and shift go 16-bit).
V5 is a local phi-copy coalescing (hypothesis: no allocator needed, not written). Per the conductor's rule: correct V1 first, then re-read the threshold.
