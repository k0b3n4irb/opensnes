# benchmark/ — Compiler Benchmark

Compares code generation quality between OpenSNES (cc65816) and PVSnesLib
(816-tcc + 816-opt) by compiling the same C functions and measuring cycle
counts.

## When to use

- **After compiler optimizations** — verify improvements vs PVSnesLib baseline
- **Evaluating code generation** — identify functions where one compiler wins

## Dependencies

- OpenSNES toolchain built (`make compiler`)
- PVSnesLib toolchain installed (`PVSNESLIB_HOME` env var)
- `devtools/cyclecount/cyclecount.py` (no pip deps)

## Usage

```bash
# Run benchmark (PVSNESLIB_HOME defaults to ~/workspace/pvsneslib)
devtools/benchmark/compare_compilers.sh

# With custom PVSnesLib path
PVSNESLIB_HOME=/path/to/pvsneslib devtools/benchmark/compare_compilers.sh
```

## What it does

1. Compiles `tests/benchmark/bench_functions.c` with both toolchains
2. Runs `cyclecount.py --json` on each output
3. Displays a side-by-side comparison table with per-function cycle counts

## Benchmark source

`tests/benchmark/bench_functions.c` contains isolated functions that each
exercise one compiler code generation pattern (loops, shifts, multiplies,
struct access, etc.).
