# G1 R3-0 result (2026-11-15, in progress)

## Corpus grep: IRQ handlers and the direct page (R2.8)
Checked 2026-11-15 over `examples`, `lib/source`, `templates`, `testing/fixtures`: two raw IRQ handlers exist,
`examples/color/hicolor_1792/irq_stream.asm` and `testing/fixtures/stress/rt32_nmi/irq.asm`. Neither touches D
(`tcd`/`pld`/`phd` absent; no `.b` direct-page access): the first sets DBR itself (`phb`, `plb`) and uses `.w` on
`$21xx/$42xx`, the second uses `.l` only. The contract in `lib/include/snes/interrupt.h` (l.158-172) already says an IRQ
handler must save what it touches and set its own data bank, and that a C pointer must not be passed to `irqSet`. So a
main-thread function that moves D or DBR is not seen by any handler of the corpus. A user handler that assumed D = 0
would contradict the documented contract. Hypothesis closed for the corpus, not proven for user code.

## Paper allocation, `soa`
IR given to the second agent: `qbe -t w65816 -d I` on the cproc IR of `k_soa_c` (SSA, before instruction selection output).
Rules file `G1_r3_paper_rules.md` (`9c848c73`). Written by a Sonnet agent that read only the rules and the IR
(`g1_r3_0/soa_ir.ssa`, `g1_r3_0/soa_paper.asm`); eight assignments tried, no block weights given (none claimed).
Measured by me in a scratch copy of the bench (paper added as a row `k_soa_pp` against the same C checksum; the repository's
bench untouched), master cycles per call:

| speed | C | hand | paper | paper / hand | C / hand |
|---|---|---|---|---|---|
| SlowROM | 36 377 | 24 865 | 26 223 | **1.055** | 1.463 |
| FastROM | 30 624 | 20 809 | 21 714 | **1.043** | 1.472 |

The paper's checksum equals the C's (same work). The `soa` row of C reads 36 377 here, the same as R0 (36 377), not the 35 813 of the probe (R2.2 question left open).
**Reading**: far under the 1.2 stop and under the 1.12 warning on this twin; the paper is a floor of what the prototype
reaches. **This is half of the stop test**: `path` (leaf A's neutral twin) is not written yet, so the stop is not decided.
Caveats: `soa` is leaf B's shape, not leaf B; no static-weight cell yet (the loop is the whole function here);
the agent hand-picked a cheap rebuild of X inside the loop (`tya/lsr/tax`), legal under the rules.

## Neutral twin of leaf A, `path` (2026-11-15)
Twin written by a Sonnet agent in the scratch bench (C `g1_r3_0/path_c.c`, hand `g1_r3_0/path_hand.asm`): four 32-bit positions
(high/low halves), a ROM sine table, 64 sub-steps, a leaf. Checksums equal (`run.py` enforces). C / hand **1.52** SlowROM and
FastROM (124 223 / 81 744 and 106 084 / 69 671); rather harder than the real leaf (1.35 measured by hand): the twin's loop has no
clamp, the 32-bit work dominates. Replayed by me.

Paper of `path`: second agent (Sonnet, rules file + IR `g1_r3_0/path_ir.ssa` only), 28 assignments tried, no D moved, no DBR set
(the table is ROM, rule 4), accumulators in slots (rule 5). `g1_r3_0/path_paper.asm`. Measured in the scratch bench as a row
`pathpp` (a second copy of the C row against the paper, checksum equal to the C):

| speed | C | hand | paper | paper / hand | C / hand |
|---|---|---|---|---|---|
| SlowROM | 124 220 | 81 744 | 107 551 | **1.316** | 1.52 |
| FastROM | 106 084 | 69 671 | 91 870 | **1.319** | 1.52 |

**Reading (fact)**: on this twin the paper is above the 1.2 stop and above the 1.12 warning. It is whole-function and unweighted
(one loop), not the "static weight" cell of the decision; and the twin is not the leaf. Gap to the hand is 25 800 cycles per call:
the paper's 32-bit sign extension (`cond ? k : 0` select, 5 uses of M) and the slots-only accumulators are where the hand
differs (hand 188 bytes, paper 244): **hypothesis**, not attributed yet (R2.2/R2.3 would). Together with `soa` (1.055 / 1.043)
the two twins disagree widely: the stop depends on leaf A's shape and is **for the owner's rule to read**, not mine.
Open: whether the hand's 32-bit trick (sign built once per sub-step) is a pass an IR idiom may carry; that decides if the
paper rules were too strict for 32-bit composition, which only a rules change (new dated file, paper void) can test.
Scratch: kernels/run.py patch not kept (disposable): rows `path` (C+hand, the files above) and `pathpp` (C copy + paper).

## Attribution of the `path` gap (2026-11-15, outcomes frozen in `G1_r3_0_attribution_rules.md`, `39cf05f3`)
Method: from the paper, one post at a time moved to the hand's way (forward chain f0..f7) and the reverse from the hand
(c0..c7, r_*), every variant checksum-equal; I replayed f0, f3, f7 (107 551 / 90 778 / 81 764 SlowROM). Posts are additive
within 3 cycles in both directions; f7 (the hand's way everywhere) = 81 764, the hand 81 744, the 20 left are the paper's `rep #$20`.
Files: `g1_r3_0/attrib/` (gen.py, measure.py, results.tsv, variants).

| Post | Paper vs hand | SlowROM | FastROM | Class |
|---|---|--:|--:|---|
| P prologue composition | `hi \| sext(lo)` select vs plain copy | 157 | 122 | idiom, data-conditional (equal here only because no `lo` is negative) |
| E acc3 += s sar 1 | 32-bit pair rotated vs low rotate + M as high word | 7 252 | 6 332 | idiom, **exact on all s16** (high word of a sign-extended sar is the sign) |
| F acc2 += s*2 | 32-bit pair shifted vs low shift + M as high word | 9 364 | 8 173 | **range-conditioned**: exact only for \|s\| < 16384; the table is 3 000, the IR does not carry it |
| B1 index scaling | idx doubled each turn vs 2·idx carried | 881 | 755 | idiom (scaled induction variable) |
| C1 loop rotation | test at top vs bottom | 1 109 | 834 | idiom (shape) |
| C2 count down | `cpy #64` vs `dey/bne` | 1 580 | 1 188 | idiom (induction reversal) |
| B2 idx in X | idx in a slot (`lda/tax/sta`) vs in X | 5 444 | 4 780 | **allocation** |
| residual | `rep #$20` | 20 | 15 | |

Shares of the 25 807 SlowROM gap: allocation (B2) **21.1 %**; idioms without F **42.5 %** (P+E+B1+C1+C2 10 979); F **36.3 %**
(9 364). Neither outcome of the frozen file holds as written: (a) needs more than half in idioms (true only if F counts), (b) needs
the gap mostly in the allocation (21 %: false on every reading). The figures at hand: paper + the uncontested idioms 96 572 =
1.18 of the hand; + B2 (the allocation the paper did not make, idx kept in a slot) 91 128 = 1.115; + F 81 764.
Caveat on the paper rules: E, F, P, B1, C1, C2 are not in the rules' idiom list (range test, `cond ? k : 0` only), so the paper
could not use them: that is the rules' narrowness, not the allocator's. The one the allocator itself loses is B2, and the paper's
own agent chose it (idx in a slot, 28 assignments).

## Second paper of `path` (2026-11-15; rules `G1_r3_paper_rules_v2.md` `f025ec9c`/`71b39109`, thresholds frozen before the text)
Written by a third Sonnet agent (not the rules' author; rules v1, v2 and the IR only); 5 assignments costed by hand after pruning;
idioms I1 (mask built once), I2 (idx carried scaled in X, `$7E` mask), I3, I4; F and P refused; no D, no DBR.
`g1_r3_0/path_paper2.asm`. Measured in the scratch bench (row `pathp2`: C copy against the paper), checksum equal to the C:

| speed | C | hand | paper 2 | paper 2 / hand |
|---|---|---|---|---|
| SlowROM | 124 223 | 81 744 | 91 545 | **1.120** |
| FastROM | 106 084 | 69 671 | 78 178 | **1.122** |

**Threshold 1 (≤ 1.20): met**, both speeds. What is left above the hand is F (0.115, refused) and P (0.002): the remainder equals
the attribution's refused posts, as predicted. One construct outside I1-I4 for the conductor: the IR's first table address is
`path_sin + 2·idx16` with `idx16` arbitrary (17 bits); the paper takes the carry out of the entry `asl` to pick a second form of the
first load (`path_sin+$10000,x`), entering the loop after it: exact, no per-iteration cost, but a loop with two entry points.
**Threshold 2 (gap ≥ 0.05 against "today + the same idioms"): not measurable at paper stage.** The idioms said in C
(`g1_r3_0/path_idioms_c.c`, checksum equal) are *slower* with today's generator: 171 480 / 81 742 = 2.10 against 1.52 of the
plain C, because carries and masks by C comparisons are not an IR pass. The column needs the idioms as IR passes (written once for
both emitters, R3-1's first step), then today's generator on them. Only the allocation part measured by substitution is known:
B2, 0.067 of ratio, on the paper's own text, not on today's output.
