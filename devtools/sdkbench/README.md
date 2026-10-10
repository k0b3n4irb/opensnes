# sdkbench — the same C on OpenSNES and on PVSnesLib, measured on luna

`workloads.c` holds twenty small workloads in plain C89. `run.py` builds
one ROM per workload, runs it on luna and reports, per workload, the master
cycles it cost, the bytes of code of its functions and how deep the stack
went. The measured table of `docs/BENCHMARK.md` comes from it.

```bash
make bench-sdk                                   # OpenSNES against the committed PVSnesLib figures
PVSNESLIB_HOME=~/workspace/pvsneslib make bench-sdk   # measure PVSnesLib again
make bench-sdk CHECK=1                           # the CI gate: OpenSNES against baseline.json
PVSNESLIB_HOME=… make bench-sdk UPDATE=1         # rewrite both committed files and the table of docs/BENCHMARK.md
```

Two files are committed here:

- `baseline.json` — the OpenSNES figures. CI runs `--check` against it and
  fails on +5 % of cycles in total, +25 % on one workload, or a changed
  checksum. After a compiler change that moves the figures, `UPDATE=1` and
  say why in the commit.
- `pvsneslib_reference.json` — PVSnesLib's figures, with the commit they
  were measured at. CI has no PVSnesLib, so the comparison is printed from
  this file; refresh it with `PVSNESLIB_HOME` set to a PVSnesLib tree built
  for this machine (`816-tcc`, `816-opt`, its library objects, its own
  `wla-65816`). Nothing of PVSnesLib is copied into this repository: the
  runner takes the header, font and Makefile of its `hello_world` example
  at run time.

How a workload is timed, and why the subtraction is exact, is in the
docstring of `run.py`. A workload on which the two SDKs disagree is
reported and left out; add one by writing `w_name()` in `workloads.c`,
giving it a number in `main`, a name in `NAMES` / `WHAT` and its functions
in `FUNCS`.

This is the scoreboard of the plan in
`.claude/notes/chantiers/beat_pvsneslib.md`: every stage has an exit
criterion read on this table.
