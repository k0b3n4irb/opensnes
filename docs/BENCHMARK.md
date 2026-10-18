# Compiler Benchmark — OpenSNES vs PVSnesLib {#benchmark}

*OpenSNES figures: `make bench` (`devtools/cyclecount/bench.py`, which CI
compares against `devtools/cyclecount/bench_baseline.json` at every push).
PVSnesLib figures: its `816-tcc` + `816-opt` on the same C file.*

This page has two benchmarks. The first, below, is a **static estimate** on
34 isolated functions: the cost of the instructions each compiler emits,
added up. The second, @ref measured "measured on luna", runs the same C
program built by both SDKs and counts the master cycles the machine
really spent.

On these 34 functions OpenSNES is **about 38 %** faster; measured on 23
whole workloads, 62 % (below). What made the difference on code that
indexes an array or follows a pointer was not the pointer width: it was the
stack frame and stack slots for values PVSnesLib keeps in the direct page,
and code the optimiser left dead. Both are gone (see "Reading it" at the
end of this page).

## Summary

| Metric | Value |
|--------|-------|
| **Total cycle reduction** | **−37.5 %** vs PVSnesLib + 816-opt (1238 vs 1981 cycles) |
| Functions compared | 34 (`helper` has no standalone body: it is inlined into `call_chain`) |
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
| `call_add` | -90.2% | Tail call optimisation — call forwarded directly |
| `compare_and_branch` | -53.5% | Comparison+branch fusion eliminates redundant CMP |
| `global_increment` | -51.2% | `.l` to `.w` shortening + `inc` instruction |
| `clamp` | -46.8% | Comparison chain fusion + dead jump elimination |
| `sub_u16` | -44.7% | Leaf optimisation + A-register cache |

### Where PVSnesLib Wins

| Function | Delta | Why |
|----------|-------|-----|
| `pea_constant_args` | +12.1% | `pea.w` constant push vs PVSnesLib's direct load |
| `mod_const_10` | +3.1% | Both use runtime `__mod16`; slight overhead difference |

`array_read`, `array_write` and `array2d_read` are wins (47 cycles against
60, 52 against 65, 86 against 112): a function that calls nothing keeps its
temporaries in the direct page, and values stored and reloaded at once, or
written and never read, are removed. Every function still starts with a
`rep #$20` (3 cycles), on purpose: it is what lets assembly call an exported
function in either accumulator width.

The `pea_constant_args` regression is a trade-off: `pea.w` is smaller in code size
(2 bytes vs 3) but costs 1 extra cycle. PVSnesLib's `lda #imm; pha` is faster but
larger. Both approaches are valid.

**Inlining.** A function marked `inline` that is short (linear flow, ≤ 8 IR
instructions, no nested calls, no allocas) is spliced into its callers, and
when every direct call has been spliced its standalone body is not emitted:
`helper(helper(x))` in `call_chain` becomes arithmetic. A second inliner
takes what that one declines: a `static` function with exactly one call
site in its file, whose address is not used, is inlined with its control
flow and not emitted (no keyword needed: the code exists once either way);
a `static inline` function called from several places is copied at each, up
to 160 IR instructions (`CC_INLINE_MAX_BIG=N`). A leaf function, unless it
is tiny, stays a function when its caller still calls something else:
apart, it keeps the direct-page frame that makes it fast.
`QBE_NO_AUTO_INLINE=1` turns the second inliner off, to measure.

(†) `helper` appears as 0 cycles because its standalone is suppressed
post-inline. The PVSnesLib values (25 raw, 22 opt) remain the baseline
for comparison.

### Key Optimisation Phases Contributing

| Phase | Impact | Example functions |
|-------|--------|-------------------|
| Dead jump elimination | ~5% | conditional, clamp, compare_and_branch |
| A-register cache | ~8% | add_u16, sub_u16, bitwise_and/or |
| Leaf function optimisation | ~10% | array_read, array_write, struct_sum |
| Frame elimination | ~5% | empty_func, helper, global_increment |
| Comparison+branch fusion | ~8% | conditional, clamp, compare_and_branch |
| Inline multiply | ~3% | mul_const_13/24/48/20/40/96 |
| Tail call optimisation | ~2% | call_add |
| Dead store elimination | ~3% | loop_sum, swap |
| `.l` to `.w` shortening | ~2% | global_increment, zero_store_global |

## Reproducing

```bash
make bench                                   # the 34 functions (cycle counts of the generated code)
make bench-sdk                               # the 23 workloads on luna, against PVSnesLib's recorded figures
PVSNESLIB_HOME=/path/to/pvsneslib make bench-sdk   # the same, PVSnesLib rebuilt from that tree
PVSNESLIB_HOME=/path/to/pvsneslib make bench-twins   # whole examples both SDKs have, side by side on luna
```

## Caveats

- Cycle counts are **estimates** based on instruction timing tables, not hardware measurements
- Functions are tested in isolation — real-world code has different calling patterns
- Loop-heavy functions (loop_sum, byte_store_loop) benefit more from optimisation
  because the overhead reduction is multiplied by iteration count
- PVSnesLib's 816-opt is designed for tcc816's output patterns; a different C compiler
  might benefit differently from its peephole rules

## The CI gate

Every push runs `make bench` and the luna workloads below on the OpenSNES
side and fails on a regression: more than 5 % in total, or more than 25 %
and 50 cycles on one function (25 % on one workload). The benchmark is
narrow — 34 functions and 24 workloads — so it is one check among the
others (`make tests`, the compiler tests).

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
| `physics` | 32 entities bouncing, 60 steps, by index | 10,031,542 | 2,203,644 | -78.0 % | 1174 | 399 | -66.0 % | 60 | 30 |
| `collide` | 496 box pairs tested, 8 rounds | 13,820,564 | 5,243,496 | -62.1 % | 983 | 405 | -58.8 % | 74 | 30 |
| `mul` | 2304 multiplies of two variables | 6,389,830 | 4,358,600 | -31.8 % | 188 | 91 | -51.6 % | 34 | 29 |
| `decimal` | 200 numbers to decimal digits (`/ 10`, `% 10`) | 8,994,226 | 2,243,024 | -75.1 % | 154 | 73 | -52.6 % | 34 | 28 |
| `long` | 300 steps of a 32-bit generator and hash | 5,944,048 | 2,354,368 | -60.4 % | 594 | 289 | -51.3 % | 76 | 33 |
| `bytes` | 512-byte fill, copy and compare, 4 passes | 7,862,640 | 3,157,202 | -59.8 % | 605 | 307 | -49.3 % | 41 | 23 |
| `calls` | recursive `fib(17)` | 5,041,934 | 2,561,730 | -49.2 % | 105 | 57 | -45.7 % | 159 | 159 |
| `switch` | 1280 operations of a `switch` interpreter | 2,697,252 | 911,554 | -66.2 % | 523 | 284 | -45.7 % | 34 | 23 |
| `crc` | CRC-16 of 256 bytes, bit by bit | 2,752,004 | 1,342,978 | -51.2 % | 233 | 108 | -53.6 % | 34 | 23 |
| `list` | a 64-node linked list walked 40 times | 2,858,640 | 1,323,922 | -53.7 % | 402 | 267 | -33.6 % | 37 | 23 |
| `tilemap` | a 32×16 tilemap written, then 1200 lookups | 2,916,886 | 813,208 | -72.1 % | 397 | 181 | -54.4 % | 33 | 23 |
| `grid` | a 16×32 byte grid, four neighbours of each cell | 5,262,594 | 1,997,894 | -62.0 % | 694 | 444 | -36.0 % | 39 | 44 |
| `entities` | the 32 entities again, through a pointer | 8,851,406 | 2,991,200 | -66.2 % | 1098 | 489 | -55.5 % | 62 | 30 |
| `copy` | word and byte copies as index loops | 4,485,144 | 1,006,290 | -77.6 % | 709 | 302 | -57.4 % | 31 | 23 |
| `strings` | `strlen`, `strcmp`, `strcpy` written by hand | 3,082,190 | 1,734,876 | -43.7 % | 1156 | 749 | -35.2 % | 48 | 48 |
| `state` | 600 steps of a `switch` state machine and a table of functions | 1,831,136 | 944,974 | -48.4 % | 392 | 270 | -31.1 % | 42 | 29 |
| `place` | 19 sprites placed 200 times: a loop over parallel tables with an on-screen test (issue #166) | 14,391,130 | 5,172,382 | -64.1 % | 831 | 350 | -57.9 % | 52 | 23 |
| `dist` | 81 distances between two groups of points, 40 times: a nested loop, two absolute values and a call (issue #166) | 27,816,270 | 7,147,052 | -74.3 % | 1018 | 452 | -55.6 % | 61 | 29 |
| `depot` | eighteen agents deciding where to go, 30 bursts of 16 decisions: parallel arrays, helpers called from loops, values live across calls (issue #166) | 37,827,628 | 12,947,774 | -65.8 % | 7722 | 3224 | -58.2 % | 93 | 56 |
| `div32` | 200 steps of 32-bit division: `u32 / u16` twice a step, `u32 / u32` every eighth | 11,361,754 | 4,644,986 | -59.1 % | 888 | 452 | -49.1 % | 94 | 71 |
| `farfill` | a screen of 96 glyphs and 24 bars composed at the pixel into a far 2bpp layer, 4 times, then on a near buffer | 134,854,318 | 55,691,046 | -58.7 % | 5132 | 2933 | -42.8 % | 84 | 63 |
| `stepseq` | a four-channel step sequencer ticked 2000 times: byte code, effect tables, a 32-bit division per period change | — | 58,359,746 | ours only | — | 3168 | | — | 62 |
| **Total** | | **325,892,518** | **122,719,436** | **-62.3 %** | **25801** | **12464** | **-51.7 %** | | |

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
