# Compiler Benchmark — OpenSNES vs PVSnesLib

*Last updated: 2026-05-10 (re-baselined post-v0.17.0; toolchain pins qbe `9878b9f`, cproc `cceac4b`).
Run: `cd tools/opensnes-emu && node test/run-benchmark.mjs`*

## Summary

| Metric | Value |
|--------|-------|
| **Total cycle reduction** | **-29.1%** vs PVSnesLib + 816-opt |
| Functions tested | 34 |
| OpenSNES wins | 31 |
| PVSnesLib+opt wins | 3 |
| Ties | 0 |

OpenSNES's cc65816 compiler (cproc + QBE w65816 backend with 13 optimization phases)
produces code that is **29% faster** than PVSnesLib's tcc816 + 816-opt peephole
optimizer on our benchmark suite of 34 isolated functions.

## Methodology

Each function in `tools/opensnes-emu/test/fixtures/benchmark/bench_functions.c` isolates one code generation
pattern (arithmetic, shifts, loops, struct access, function calls, etc.). Both
compilers process the same source file:

- **PVSnesLib**: `816-tcc` (C89 compiler) + `816-opt` (38 peephole rules)
- **OpenSNES**: `cc65816` = `cproc` (C11 frontend) + `qbe -t w65816` (13 optimization phases)

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
  loop_sum                   208       185       119    -35.7%
  array_write                 74        65        36    -44.6%
  array_read                  68        60        31    -48.3%
  struct_sum                 111        86        74    -14.0%
  swap                       158       142       107    -24.6%
  call_add                    56        41         4    -90.2%
  mul_variable                55        48        45     -6.2%
  clamp                      142       124        66    -46.8%
  signed_shift_right_8        38        31        28     -9.7%
  signed_shift_right_1        28        25        19    -24.0%
  byte_store_loop            212       192       138    -28.1%
  global_increment            49        43        21    -51.2%
  zero_store_global           23        19        14    -26.3%
  compare_and_branch         136       129        60    -53.5%
  helper                      25        22        16    -27.3%
  call_chain                  56        47        62    +31.9%
  pea_constant_args           36        33        37    +12.1%
  mul_const_24                45        42        32    -23.8%
  mul_const_48                45        42        34    -19.0%
  mul_const_20                45        42        32    -23.8%
  mul_const_40                45        42        34    -19.0%
  mul_const_96                45        42        36    -14.3%
  ────────────────────  ────────  ────────  ────────  ────────
  TOTAL                     2178      1891      1341    -29.1%
```

## Analysis

### Biggest Wins (>40% improvement)

| Function | Improvement | Why |
|----------|-------------|-----|
| `call_add` | -90.2% | Tail call optimization — call forwarded directly |
| `compare_and_branch` | -53.5% | Comparison+branch fusion eliminates redundant CMP |
| `global_increment` | -51.2% | `.l` to `.w` shortening + `inc` instruction |
| `array_read` | -48.3% | Leaf function frame elimination + param aliasing |
| `clamp` | -46.8% | Comparison chain fusion + dead jump elimination |
| `array_write` | -44.6% | Frame elimination + direct store pattern |
| `sub_u16` | -44.7% | Leaf optimization + A-register cache |

### Where PVSnesLib Wins

| Function | Delta | Why |
|----------|-------|-----|
| `mod_const_10` | +3.1% | Both use runtime `__mod16`; slight overhead difference |
| `pea_constant_args` | +12.1% | `pea.w` constant push vs PVSnesLib's direct load |
| `call_chain` | +31.9% | Tail-call optimisation trade-off — see note below |

The `pea_constant_args` regression is a trade-off: `pea.w` is smaller in code size
(2 bytes vs 3) but costs 1 extra cycle. PVSnesLib's `lda #imm; pha` is faster but
larger. Both approaches are valid.

The `call_chain` (`helper(helper(x))`) regression is a tail-call-optimisation
trade-off. Pre-TCO codegen for this shape was a frameless `lda 4,s; pha; jsl;
plx; pha; jsl; plx; rtl` at 48 cycles. When TCO landed (qbe `ed840fb`), the
chained-call path acquired a small frame to handle the general case (intermediate
value preserved across the inner call, tail-jump to the outer helper after frame
deallocation). For this specific shape the new codegen is +14 cycles vs the old
frameless form, but **TOTAL benchmark cycles dropped from 1420 (pre-TCO) to 1341
(post-TCO)** — a net **-5.6 % improvement** earned by TCO's wins on the rest of
the suite. A future "smart-path" optimisation could detect when the chained-call
intermediate doesn't survive the inner call and re-emit the frameless pattern,
reclaiming the 14 cycles; not yet scheduled.

### Key Optimization Phases Contributing

| Phase | Impact | Example functions |
|-------|--------|-------------------|
| Dead jump elimination | ~5% | conditional, clamp, compare_and_branch |
| A-register cache | ~8% | add_u16, sub_u16, bitwise_and/or |
| Leaf function optimization | ~10% | array_read, array_write, struct_sum |
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
`tools/opensnes-emu/test/fixtures/benchmark/bench_functions.c`. A PR can
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
- 2026-05-09: promoted to hard gate (chantier E2). Catalogue entry in
  `.claude/STRUCTURAL_DEFECTS.md`. Decision rationale: the soft gate
  had operated long enough to confirm the threshold design is workable;
  the override mechanism gives a clean path for deliberate trade-offs.
