# sdkbench — the same C on OpenSNES and on PVSnesLib, timed on luna

`workloads.c` holds twelve small workloads in plain C89. `run.py` builds one
ROM per workload with each SDK, runs them on luna for the same number of
frames and reports the master cycles each workload cost. The figures on
`docs/BENCHMARK.md` ("Measured on luna") come from it.

```bash
PVSNESLIB_HOME=~/workspace/pvsneslib make bench-sdk
PVSNESLIB_HOME=~/workspace/pvsneslib python3 devtools/sdkbench/run.py --only sort,crc --json out.json
```

It needs a PVSnesLib tree built for this machine (`816-tcc`, `816-opt`, the
library objects, its own `wla-65816`); nothing of PVSnesLib is copied into
this repository, the runner takes the header, font and Makefile of its
`hello_world` example at run time. That is why it is not part of any gate
and not run by CI.

How a workload is timed, and why the subtraction is exact, is in the
docstring of `run.py`. A workload on which the two SDKs disagree is reported
and left out of the comparison; add one by writing `w_name()` in
`workloads.c`, giving it a number in `main` and a name in `NAMES` / `WHAT`.

After a compiler change that moves performance, re-run it and update the
table of `docs/BENCHMARK.md` in the same commit.
