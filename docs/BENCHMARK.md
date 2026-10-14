# Compiler Benchmark — OpenSNES vs PVSnesLib {#benchmark}

*Last updated: 2026-10-08. Both compilers were run that day on the same
machine: `python3 devtools/cyclecount/bench.py` for OpenSNES (the code the
CI cycle gate compares against `devtools/cyclecount/bench_baseline.json`),
and PVSnesLib's `816-tcc` + `816-opt` from a local PVSnesLib tree for the
other two columns. The PVSnesLib figures had been frozen since 2026-05-13;
the re-run gives the same numbers, to the cycle.*

This page has two benchmarks. The first, below, is a **static estimate** on
34 isolated functions: the cost of the instructions each compiler emits,
added up. The second, @ref measured "measured on luna", runs the same C
program built by both SDKs and counts the master cycles the machine
really spent.

**What changed since the May figure (−32.2 %).** On 2026-05-15 pointers and
`u32` became 4-byte values (far pointers, v0.19.0, so data can live in any
bank). The functions that index an array or follow a pointer —
`array_read`, `array_write`, `array2d_read`, `struct_sum` — went from wins
to losses; the rest held. The cause is not the pointer width itself (see
"Reading it" at the end of this page): it is a stack frame and stack
slots for values PVSnesLib keeps in the direct page, and code the
optimizer leaves dead. The May text claimed "32 % faster";
the figure today is **about 38 %** on these 34 functions, and
61 % measured on 23 whole workloads (below).

## Summary

| Metric | Value |
|--------|-------|
| **Total cycle reduction** | **−37.5 %** vs PVSnesLib + 816-opt (1238 vs 1981 cycles) |
| Functions compared | 34 (the May `helper` row has no standalone body any more: it is inlined into `call_chain`; `array2d_read` joined on 2026-10-08, it was in the gate's baseline but not on this page) |
| OpenSNES wins | 32 |
| PVSnesLib+opt wins | 2 (`pea_constant_args`, `mod_const_10`) |
| Ties | 0 |

OpenSNES's cc65816 compiler (cproc + QBE w65816 backend) wins on arithmetic,
comparisons, branches, calls and globals. It loses on code that goes through
pointers, where the 4-byte pointer ABI costs more than the optimisations
recover. Cycle counts are static estimates, not measurements on hardware.

## Methodology

Each function in `devtools/cyclecount/bench_functions.c` isolates one code generation
pattern (arithmetic, shifts, loops, struct access, function calls, etc.). Both
compilers process the same source file:

- **PVSnesLib**: `816-tcc` (C89 compiler) + `816-opt` (38 peephole rules)
- **OpenSNES**: `cc65816` = `cproc` (C11 frontend) + `qbe -t w65816`

Cycle counts are estimated by `devtools/cyclecount/cyclecount.py`, which assigns
per-instruction cycle costs based on the 65816 datasheet (assuming 16-bit A/X/Y,
no page-crossing penalties).

## Full Results

```
  FUNCTION               PVS_RAW   PVS_OPT  OPENSNES    vs_OPT
  ────────────────────  ────────  ────────  ────────  ────────
  empty_func                  16        13        12     -7.7%
  add_u16                     41        34        21    -38.2%
  sub_u16                     41        38        21    -44.7%
  mul_const_13                45        42        36    -14.3%
  mul_const_8                 32        21        20     -4.8%
  div_const_10                41        36        33     -8.3%
  mod_const_10                37        32        33     +3.1%
  shift_left_3                32        21        20     -4.8%
  shift_right_4               34        23        22     -4.3%
  bitwise_and                 39        32        19    -40.6%
  bitwise_or                  39        32        19    -40.6%
  conditional                 81        65        37    -43.1%
  loop_sum                   208       185        71    -61.6%
  array_write                 74        65        52    -20.0%
  array_read                  68        60        47    -21.7%
  struct_sum                 111        86        49    -43.0%
  swap                       158       142        83    -41.5%
  call_add                    56        41         4    -90.2%
  mul_variable                55        48        45     -6.2%
  clamp                      142       124        60    -51.6%
  signed_shift_right_8        38        31        28     -9.7%
  signed_shift_right_1        28        25        19    -24.0%
  byte_store_loop            212       192        93    -51.6%
  global_increment            49        43        21    -51.2%
  zero_store_global           23        19        14    -26.3%
  compare_and_branch         136       129        49    -62.0%
  call_chain                  56        47        19    -59.6%
  pea_constant_args           36        33        37    +12.1%
  mul_const_24                45        42        32    -23.8%
  mul_const_48                45        42        34    -19.0%
  mul_const_20                45        42        32    -23.8%
  mul_const_40                45        42        34    -19.0%
  mul_const_96                45        42        36    -14.3%
  array2d_read               131       112        86    -23.2%
  ────────────────────  ────────  ────────  ────────  ────────
  TOTAL                     2284      1981      1238    -37.5%
```

## Analysis

### Biggest Wins (>40% improvement)

| Function | Improvement | Why |
|----------|-------------|-----|
| `call_add` | -90.2% | Tail call optimization — call forwarded directly |
| `compare_and_branch` | -53.5% | Comparison+branch fusion eliminates redundant CMP |
| `global_increment` | -51.2% | `.l` to `.w` shortening + `inc` instruction |
| `clamp` | -46.8% | Comparison chain fusion + dead jump elimination |
| `sub_u16` | -44.7% | Leaf optimization + A-register cache |

### Where PVSnesLib Wins

| Function | Delta | Why |
|----------|-------|-----|
| `pea_constant_args` | +12.1% | `pea.w` constant push vs PVSnesLib's direct load |
| `mod_const_10` | +3.1% | Both use runtime `__mod16`; slight overhead difference |

`array_read`, `array_write` and `array2d_read` were wins in May (−48 %,
−45 %, −14 %), lost 34 to 70 % after far pointers, and are wins again since
2026-10-10 (47 cycles against 60, 52 against 65, 86 against 112). The loss
had been put down to the pointers; read instruction by instruction on
2026-10-08, the dereference itself cost the same as before. The 40 to 46
cycles lost per function were a stack frame set up and torn down for the
compiler's temporaries (22), values stored to that frame and reloaded at
once (about 40), a high word written and never read (8) and the `rep #$20`
every function starts with (3). The frame went on 2026-10-09 (a function
that calls nothing keeps its temporaries in the direct page), the reloads
and the dead high word on 2026-10-10 (a peephole over the emitted text).
The `rep #$20` stays, on purpose: it is what lets assembly call an exported
function in either accumulator width.

The `pea_constant_args` regression is a trade-off: `pea.w` is smaller in code size
(2 bytes vs 3) but costs 1 extra cycle. PVSnesLib's `lda #imm; pha` is faster but
larger. Both approaches are valid.

The previous `call_chain` regression (+31.9%) was resolved by the 2026-05-12
function-inlining work (qbe commits `13d9b14`, `29b0941`, `77d07a0`). With
`helper` marked `static inline`, the qbe inline pass collapses `helper(helper(x))`
to inlined arithmetic, dropping the function from 62 cycles to 19 (-59.6 % vs
PVSnesLib+opt's 47).

The 2026-05-13 follow-up (qbe `988073d`, cproc `42a5c46`) added deferred
asm emission and consumption-aware standalone suppression: when every direct
caller of an inline-marked function in a TU has been spliced and no
indirect reference remains, the QBE emit stage drops the standalone body
entirely. The bench `helper` ends up "0 cycles" in the table marker (†)
because its body now lives exclusively inside `call_chain` — the
standalone is dead-code-eliminated at compile time. Cycle-count semantics
are unchanged (the work always happened inside `call_chain`); the win is
ROM-size (≈ 6 bytes per such helper).

The pre-inlining historical note is preserved for context: TCO had landed
in qbe `ed840fb` and lowered TOTAL from 1420 (pre-TCO) to 1341 (post-TCO),
trading +14 cycles on `call_chain` for wins elsewhere. The function-inlining
work took TOTAL from 1341 to 1298 by recovering the `call_chain`
regression entirely (and going further: -28 cycles below PVSnesLib+opt rather
than +14 above the pre-TCO baseline). The deferred-emit follow-up dropped
TOTAL another 16 cycles to 1282 via the `helper` dead-code elimination.

Inlining is opt-in via the `inline` keyword and gated by a conservative
heuristic (linear flow, ≤ 8 IR instructions, no nested calls, no allocas).
Functions that don't qualify fall back to JSL/JML and incur no overhead.

Since 2026-10-10 a second, whole-function inliner takes what that one
declines. A `static` function with exactly one call site in its file, whose
address is not used, is inlined with its control flow and not emitted (no
keyword needed: the code exists once either way). A `static inline`
function called from several places is copied at each, up to 160 IR
instructions (`CC_INLINE_MAX_BIG=N`). A function that calls nothing (a leaf), unless it is tiny, is left a
function when its caller still calls something else: apart, it keeps the
direct-page frame that makes it fast. `QBE_NO_AUTO_INLINE=1` turns it off, to measure.
Lib helpers using the C99 inline pattern (`inline` body in header +
force-emit anchor in canonical `.c`) participate automatically — 7 lib
symbols retrofitted as of v0.17.0+ (`setScreenOff`, `getBrightness`,
`mosaicInit`, `hdmaWaveSetSpeed`, `scopeCalibrate`, `colorMathInit`,
`colorMathDisable`).

(†) `helper` appears as 0 cycles because its standalone is suppressed
post-inline. The PVSnesLib values (25 raw, 22 opt) remain the baseline
for comparison.

### Key Optimization Phases Contributing

| Phase | Impact | Example functions |
|-------|--------|-------------------|
| Dead jump elimination | ~5% | conditional, clamp, compare_and_branch |
| A-register cache | ~8% | add_u16, sub_u16, bitwise_and/or |
| Leaf function optimization | ~10% (May) | array_read, array_write, struct_sum — no longer fires on pointer code since A6 |
| Frame elimination | ~5% | empty_func, helper, global_increment |
| Comparison+branch fusion | ~8% | conditional, clamp, compare_and_branch |
| Inline multiply | ~3% | mul_const_13/24/48/20/40/96 |
| Tail call optimization | ~2% | call_add |
| Dead store elimination | ~3% | loop_sum, swap |
| `.l` to `.w` shortening | ~2% | global_increment, zero_store_global |

## Reproducing

```bash
# Requires both OpenSNES and PVSnesLib toolchains
PVSNESLIB_HOME=/path/to/pvsneslib devtools/benchmark/compare_compilers.sh
```

## Caveats

- Cycle counts are **estimates** based on instruction timing tables, not hardware measurements
- Functions are tested in isolation — real-world code has different calling patterns
- Loop-heavy functions (loop_sum, byte_store_loop) benefit more from optimization
  because the overhead reduction is multiplied by iteration count
- PVSnesLib's 816-opt is designed for tcc816's output patterns; a different C compiler
  might benefit differently from its peephole rules

## CI gate (hard, with override trailer)

Every pull request to `main` or `develop` runs `.github/workflows/benchmark.yml`,
which:

1. Builds `cc65816` at the PR HEAD and runs `bench_functions.c` through it.
2. Builds `cc65816` at the PR base ref and runs the same benchmark.
3. Compares the two assembly outputs via
   `devtools/cyclecount/cyclecount.py --compare --fail-on-regression`.
4. Posts (or updates, idempotently) a comment on the PR with the per-function
   delta, total, and the gate verdict (clean / overridden / blocking).
5. **Fails the job** if the comparison breaches a threshold AND no override
   trailer is present in the PR's commit messages.

### Thresholds

The gate is **two-armed** so each arm catches a class of regression the
other misses. A breach on either arm fails the gate.

| Arm | Trips when | Reason |
|---|---|---|
| **Total** | Total cycles regress > **5 %** | Catches "many small regressions add up" — broad-drift detection across the whole bench surface. |
| **Per-function** | A single function regresses > **25 %** AND > **50 cycles** absolute | Catches pathological per-function regressions. The combined percent + absolute floor avoids false positives on small routines (a 4-instruction function going +3 cycles is +30 % but doesn't matter in practice; both knobs must trip together). |

Thresholds are tunable via flags on `cyclecount.py --fail-on-regression`
(`--total-pct-limit`, `--fn-pct-limit`, `--fn-abs-limit`); the workflow
uses the script defaults.

### Override mechanism

When a regression is **deliberate** (a correctness fix that costs cycles,
a refactor for code clarity, a trade-off for compiler-stack
maintainability), include this trailer in any commit in the PR's range:

```
Cycle-Regression-OK: <one-line reason for the regression>
```

The workflow scans `git log <base>..<head> --format=%B` for the trailer.
If present, the gate exits 0 even on a breached threshold; the
comparison is still posted as a comment (with the `regression
overridden` header) so the regression is recorded for reviewer
awareness.

Why a trailer rather than a PR label or special comment marker:

- **Audit trail in git history.** The reason for the trade-off lives in
  the commit message forever. `git log --grep='Cycle-Regression-OK:'`
  enumerates every deliberate regression the project has accepted, with
  its rationale.
- **Self-service.** Anyone with push access to the PR's branch can add
  the trailer. PR labels would require the override step to depend on
  who has the right to set labels.
- **Survives rebase / squash-merge.** The trailer is in the commit
  message, not metadata around the PR.

### What the gate does NOT cover

The benchmark surface is narrow: 34 isolated functions in
`devtools/cyclecount/bench_functions.c`. A PR can
regress on real-world code without regressing on the benchmark, and vice
versa. The gate is one input among several when evaluating
compiler-touching changes — the visual-regression suite, runtime tests,
and ASM-pattern checks in `compiler-tests.mjs` are the other arms.

Direct pushes to `develop` or `main` don't trigger the workflow (no
obvious base ref to compare against). The gate runs on
`pull_request` events only.

### History

- 2026-05-08: shipped soft / comment-only (commit `98d5014`) as the
  audit response to §14 of the external review.
- 2026-05-09: promoted to hard gate. Catalogue entry in
  `.claude/STRUCTURAL_DEFECTS.md`. Decision rationale: the soft gate
  had operated long enough to confirm the threshold design is workable;
  the override mechanism gives a clean path for deliberate trade-offs.

## Measured on luna {#measured}

*Measured with `make bench-sdk` on luna (the pin in `testing/luna.version`),
PVSnesLib at `fa758c9b 2025-12-28`, both ROMs LoROM SlowROM. The figures are
the two files committed in `devtools/sdkbench/`; CI re-measures the OpenSNES
side at every push.*

The same C file, `devtools/sdkbench/workloads.c`, is built unchanged by
both SDKs and run on luna, which is cycle-accurate. It holds 24 small
workloads of the kind a game runs every frame (23 that both SDKs build the
same way, and one measured for OpenSNES alone). Three things are measured
for each: the **master cycles** it costs (an NTSC frame is 357,368),
the **bytes of code** of its functions, and how deep the **stack** goes
while it runs (bytes below the initial stack pointer).

<!-- sdkbench:begin -->
| Workload | What it does | Cycles: PVSnesLib | OpenSNES | | Size: PVS | OSN | | Stack: PVS | OSN |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `sieve` | sieve of 1024 in a byte array | 4,199,010 | 1,137,028 | -72.9 % | 296 | 107 | -63.9 % | 31 | 23 |
| `sort` | insertion sort of 64 words | 2,620,372 | 790,208 | -69.8 % | 507 | 231 | -54.4 % | 43 | 25 |
| `physics` | 32 entities bouncing, 60 steps, by index | 10,031,542 | 2,207,688 | -78.0 % | 1174 | 415 | -64.7 % | 60 | 30 |
| `collide` | 496 box pairs tested, 8 rounds | 13,820,564 | 5,262,508 | -61.9 % | 983 | 417 | -57.6 % | 74 | 30 |
| `mul` | 2304 multiplies of two variables | 6,389,830 | 4,358,600 | -31.8 % | 188 | 91 | -51.6 % | 34 | 29 |
| `decimal` | 200 numbers to decimal digits (`/ 10`, `% 10`) | 8,994,226 | 2,243,024 | -75.1 % | 154 | 73 | -52.6 % | 34 | 28 |
| `long` | 300 steps of a 32-bit generator and hash | 5,944,048 | 2,354,368 | -60.4 % | 594 | 289 | -51.3 % | 76 | 33 |
| `bytes` | 512-byte fill, copy and compare, 4 passes | 7,862,640 | 3,697,616 | -53.0 % | 605 | 323 | -46.6 % | 41 | 23 |
| `calls` | recursive `fib(17)` | 5,041,934 | 2,561,730 | -49.2 % | 105 | 57 | -45.7 % | 159 | 159 |
| `switch` | 1280 operations of a `switch` interpreter | 2,697,252 | 911,554 | -66.2 % | 523 | 284 | -45.7 % | 34 | 23 |
| `crc` | CRC-16 of 256 bytes, bit by bit | 2,752,004 | 1,342,978 | -51.2 % | 233 | 108 | -53.6 % | 34 | 23 |
| `list` | a 64-node linked list walked 40 times | 2,858,640 | 1,698,250 | -40.6 % | 402 | 278 | -30.8 % | 37 | 23 |
| `tilemap` | a 32×16 tilemap written, then 1200 lookups | 2,916,886 | 813,208 | -72.1 % | 397 | 181 | -54.4 % | 33 | 23 |
| `grid` | a 16×32 byte grid, four neighbours of each cell | 5,262,594 | 1,997,894 | -62.0 % | 694 | 444 | -36.0 % | 39 | 44 |
| `entities` | the 32 entities again, through a pointer | 8,851,406 | 3,252,782 | -63.3 % | 1098 | 521 | -52.6 % | 62 | 30 |
| `copy` | word and byte copies as index loops | 4,485,144 | 1,006,290 | -77.6 % | 709 | 302 | -57.4 % | 31 | 23 |
| `strings` | `strlen`, `strcmp`, `strcpy` written by hand | 3,082,190 | 1,937,314 | -37.1 % | 1156 | 797 | -31.1 % | 48 | 48 |
| `state` | 600 steps of a `switch` state machine and a table of functions | 1,831,136 | 944,974 | -48.4 % | 392 | 270 | -31.1 % | 42 | 29 |
| `place` | 19 sprites placed 200 times: a loop over parallel tables with an on-screen test (issue #166) | 14,391,130 | 5,173,638 | -64.0 % | 831 | 354 | -57.4 % | 52 | 29 |
| `dist` | 81 distances between two groups of points, 40 times: a nested loop, two absolute values and a call (issue #166) | 27,816,270 | 7,150,890 | -74.3 % | 1018 | 460 | -54.8 % | 61 | 28 |
| `depot` | eighteen agents deciding where to go, 30 bursts of 16 decisions: parallel arrays, helpers called from loops, values live across calls (issue #166) | 37,827,628 | 13,911,792 | -63.2 % | 7722 | 3387 | -56.1 % | 93 | 56 |
| `div32` | 200 steps of 32-bit division: `u32 / u16` twice a step, `u32 / u32` every eighth | 11,361,754 | 4,644,986 | -59.1 % | 888 | 452 | -49.1 % | 94 | 71 |
| `farfill` | a screen of 96 glyphs and 24 bars composed at the pixel into a far 2bpp layer, 4 times, then on a near buffer | 134,854,318 | 56,433,270 | -58.2 % | 5132 | 2959 | -42.3 % | 84 | 63 |
| `stepseq` | a four-channel step sequencer ticked 2000 times: byte code, effect tables, a 32-bit division per period change | — | 58,359,746 | ours only | — | 3168 | | — | 62 |
| **Total** | | **325,892,518** | **125,832,590** | **-61.4 %** | **25801** | **12800** | **-50.4 %** | | |

Of the 23 workloads both SDKs build OpenSNES is **faster on 23**, **no larger on 23**, and **no deeper in stack on 22**. Both ROMs leave the same checksum for each of them, so they computed the same thing.

`stepseq` is measured for OpenSNES alone and stays out of the totals: built by PVSnesLib's compiler it leaves another checksum than the native build of the same file and than ours, so its time there measures another computation.
<!-- sdkbench:end -->

**How a workload is timed.** `luna profile` credits every master cycle to
the symbol being executed. For each SDK the runner builds one ROM per
workload and one that runs none, and runs each for the same 900 frames. The
two differ only by the workload: its symbols gain cycles, the idle loop
loses exactly as many (checked: the two sums are equal), and the NMI
handler stays the same. The figure is the sum of the gains — the function,
everything it calls, the runtime's multiply and divide — with nothing of
either SDK's boot or per-frame handler in it.

**Reading it.**

- **Every workload is faster, by 32 to 78 %, and none is larger.**
  Divide, modulo and 32-bit arithmetic gain most (`decimal` −75 %, `long`
  −60 %, `div32` −59 %), and so do index loops over arrays (`physics`
  −78 %, `copy` −78 %, `sieve` −73 %). The closest are `mul` (−32 %),
  `strings` (−37 %) and `list`
  (−41 %).
- **Why it is faster.** The compiler removes values that are computed and
  never read, branches where a condition is decided instead of storing a
  0 or 1 and testing it again, reads `arr[i]` as `lda.l arr,x` with the
  index in X and `p->field` as `lda.l N,x` with the pointer in X, and keeps
  the temporaries of a function in the direct page when no call crosses
  them.
- **The stack is level or shallower on 22 workloads of 23.** `grid` is the
  exception (44 bytes against 39): a single large function whose
  temporaries do not fit the 16 words of the direct-page block.

What is left has named causes — a stack frame in every function that
calls another, X reloaded after every instruction that is not an access,
a 32-bit read through a pointer still staged in the direct page — and PVSnesLib is itself far from what a person would write (it never
uses X or Y as an index: the inner loop of `sort` is 255 cycles there and
27 by hand).

What this does not measure: the libraries (sprite, background, audio
engines) and a whole game.
