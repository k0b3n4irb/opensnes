# Compiler Benchmark — OpenSNES vs PVSnesLib

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
the figure today is **about 30 %** on these 34 functions, and
44 % measured on eighteen whole workloads.

## Summary

| Metric | Value |
|--------|-------|
| **Total cycle reduction** | **−30.3 %** vs PVSnesLib + 816-opt (1380 vs 1981 cycles) |
| Functions compared | 34 (the May `helper` row has no standalone body any more: it is inlined into `call_chain`; `array2d_read` joined on 2026-10-08, it was in the gate's baseline but not on this page) |
| OpenSNES wins | 29 |
| PVSnesLib+opt wins | 5 (`array_read`, `array_write`, `array2d_read`, `pea_constant_args`, `mod_const_10`) |
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
  conditional                 81        65        40    -38.5%
  loop_sum                   208       185        84    -54.6%
  array_write                 74        65        75    +15.4%
  array_read                  68        60        70    +16.7%
  struct_sum                 111        86        55     -36.0%
  swap                       158       142        91    -35.9%
  call_add                    56        41         4    -90.2%
  mul_variable                55        48        45     -6.2%
  clamp                      142       124        66    -46.8%
  signed_shift_right_8        38        31        28     -9.7%
  signed_shift_right_1        28        25        19    -24.0%
  byte_store_loop            212       192       111    -42.2%
  global_increment            49        43        21    -51.2%
  zero_store_global           23        19        14    -26.3%
  compare_and_branch         136       129        60    -53.5%
  call_chain                  56        47        19    -59.6%
  pea_constant_args           36        33        37    +12.1%
  mul_const_24                45        42        32    -23.8%
  mul_const_48                45        42        34    -19.0%
  mul_const_20                45        42        32    -23.8%
  mul_const_40                45        42        34    -19.0%
  mul_const_96                45        42        36    -14.3%
  array2d_read               131       112       117    +4.5%
  ────────────────────  ────────  ────────  ────────  ────────
  TOTAL                     2284      1981      1380    -30.3%
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
| `array_read` | +16.7% | the index and the address are still stored and reloaded (101 cycles until its temporaries moved to the direct page, 2026-10-09) |
| `array_write` | +15.4% | same cause as `array_read` |
| `array2d_read` | +4.5% | same cause, twice: the row address then the column |
| `pea_constant_args` | +12.1% | `pea.w` constant push vs PVSnesLib's direct load |
| `mod_const_10` | +3.1% | Both use runtime `__mod16`; slight overhead difference |

The first three were wins in May (−48 %, −45 %, −14 %) and lost 34 to 70 %
after far pointers. That was put down to the pointers; read again on
2026-10-08, instruction by instruction, the dereference itself cost the
same as before. The 40 to 46 cycles lost per function were a stack frame
set up and torn down for the compiler's temporaries (22), values stored to
that frame and reloaded at once (about 40), a high word written and never
read (8) and the `rep #$20` every function starts with (3). The frame is
gone since 2026-10-09 — these functions call nothing, so their temporaries
live in the direct page, as PVSnesLib's do — and what is left of the gap is
the store-and-reload and the `rep #$20`, which an exported function keeps
on purpose: it is what lets assembly call it in either accumulator width.

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

*Measured 2026-10-08 with `make bench-sdk` — luna v1.34.0, PVSnesLib at
`fa758c9b 2025-12-28`, both ROMs LoROM SlowROM. The figures are the two files
committed in `devtools/sdkbench/`; CI re-measures the OpenSNES side at every
push.*

The same C file, `devtools/sdkbench/workloads.c`, is built unchanged by
both SDKs and run on luna, which is cycle-accurate. It holds eighteen small
workloads of the kind a game runs every frame. Three things are measured
for each: the **master cycles** it costs (an NTSC frame is about 357,370),
the **bytes of code** of its functions, and how deep the **stack** goes
while it runs (bytes below the initial stack pointer).

<!-- sdkbench:begin -->
| Workload | What it does | Cycles: PVSnesLib | OpenSNES | | Size: PVS | OSN | | Stack: PVS | OSN |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `sieve` | sieve of 1024 in a byte array | 4,199,010 | 2,021,434 | -51.9 % | 296 | 165 | -44.3 % | 31 | 23 |
| `sort` | insertion sort of 64 words | 2,620,372 | 1,645,356 | -37.2 % | 507 | 344 | -32.1 % | 43 | 43 |
| `physics` | 32 entities bouncing, 60 steps, by index | 10,031,542 | 5,294,296 | -47.2 % | 1174 | 676 | -42.4 % | 60 | 54 |
| `collide` | 496 box pairs tested, 8 rounds | 13,820,564 | 10,071,356 | -27.1 % | 983 | 675 | -31.3 % | 74 | 60 |
| `mul` | 2304 multiplies of two variables | 6,389,830 | 4,846,710 | -24.1 % | 188 | 107 | -43.1 % | 34 | 29 |
| `decimal` | 200 numbers to decimal digits (`/ 10`, `% 10`) | 8,994,226 | 2,821,866 | -68.6 % | 154 | 117 | -24.0 % | 34 | 29 |
| `long` | 300 steps of a 32-bit generator and hash | 5,944,048 | 2,633,336 | -55.7 % | 594 | 320 | -46.1 % | 76 | 33 |
| `bytes` | 512-byte fill, copy and compare, 4 passes | 7,862,640 | 4,818,064 | -38.7 % | 605 | 414 | -31.6 % | 41 | 23 |
| `calls` | recursive `fib(17)` | 5,041,934 | 2,827,702 | -43.9 % | 105 | 62 | -41.0 % | 159 | 159 |
| `switch` | 1280 operations of a `switch` interpreter | 2,697,252 | 1,670,370 | -38.1 % | 523 | 506 | -3.3 % | 34 | 23 |
| `crc` | CRC-16 of 256 bytes, bit by bit | 2,752,004 | 1,740,554 | -36.8 % | 233 | 149 | -36.1 % | 34 | 23 |
| `list` | a 64-node linked list walked 40 times | 2,858,640 | 2,061,014 | -27.9 % | 402 | 359 | -10.7 % | 37 | 23 |
| `tilemap` | a 32×16 tilemap written, then 1200 lookups | 2,916,886 | 1,455,356 | -50.1 % | 397 | 272 | -31.5 % | 33 | 23 |
| `grid` | a 16×32 byte grid, four neighbours of each cell | 5,262,594 | 2,436,360 | -53.7 % | 694 | 532 | -23.3 % | 39 | 63 |
| `entities` | the 32 entities again, through a pointer | 8,851,406 | 4,069,084 | -54.0 % | 1098 | 620 | -43.5 % | 62 | 54 |
| `copy` | word and byte copies as index loops | 4,485,144 | 2,020,012 | -55.0 % | 709 | 456 | -35.7 % | 31 | 23 |
| `strings` | `strlen`, `strcmp`, `strcpy` written by hand | 3,082,190 | 2,264,658 | -26.5 % | 1156 | 962 | -16.8 % | 48 | 58 |
| `state` | 600 steps of a `switch` state machine and a table of functions | 1,831,136 | 1,322,716 | -27.8 % | 392 | 372 | -5.1 % | 42 | 39 |
| **Total** | | **99,641,418** | **56,020,244** | **-43.8 %** | **10210** | **7108** | **-30.4 %** | | |

Of the 18 workloads OpenSNES is **faster on 18**, **no larger on 18**, and **no deeper in stack on 16**. Both ROMs leave the same checksum for every workload, so they computed the same thing.
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

- **Divide, modulo and 32-bit arithmetic are where OpenSNES wins most**
  (`decimal` −68 %, `long` −53 %, `grid` −50 %).
- **Every workload is faster, by 24 to 69 %, and none is larger.** Arrays,
  byte loops, tilemaps and entities are 39 to 55 % faster; `mul`,
  `strings`, `collide` and `state` are the closest, at 24 to 28 %.
- **Until 2026-10-08 three were slower** — `sort` +63 %, `collide` +13 %,
  `list` +7 % — and this page put it down to the 4-byte pointer. That was
  wrong: OpenSNES dereferences a plain pointer in 16 bits, it is PVSnesLib
  that does a 24-bit access. Read instruction by instruction, an iteration
  of `sort` spent 258 of its 496 cycles on 32-bit index and address values
  that were stored and never read, 66 on the 0 or 1 of each half of
  `j > 0 && arr[j - 1] > key`, stored and tested again, and about 95 on
  building each address in A before moving it to X. All three are gone:
  dead values are removed, a condition branches where it is decided, and
  `arr[i]` is `lda.l arr,x` with the index in X, `p->field` is
  `lda.l N,x` with the pointer in X. `&nodes[k]` with a 6-byte element no
  longer calls the 32-bit multiply either.
- **The stack is level or shallower on 16 workloads of 18**, by up to 43
  bytes (`long`); `grid` (64 bytes against 39) and `strings` (58 against
  48) are deeper. Until 2026-10-08 it was deeper on all 18, by 1 to 106
  bytes: each temporary of the compiler owned a stack slot, where
  PVSnesLib keeps its temporaries in direct-page pseudo-registers. Since
  then a function that calls nothing keeps its temporaries in the direct
  page too, promoted locals and values that never touch the stack have no
  slot, parameters are read where the caller pushed them, and an address
  takes one word. The two that remain are single large functions whose
  temporaries do not fit the 16 words of the direct-page frame.

What is left has named causes — a stack frame in every function that
calls another, X reloaded after every instruction that is not an access,
a 32-bit read through a pointer still staged in the direct page — and PVSnesLib is itself far from what a person would write (it never
uses X or Y as an index: the inner loop of `sort` is 255 cycles there and
27 by hand). The plan is `.claude/notes/chantiers/beat_pvsneslib.md`.

What this does not measure: the libraries (sprite, background, audio
engines) and a whole game.
