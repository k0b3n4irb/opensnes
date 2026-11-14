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
