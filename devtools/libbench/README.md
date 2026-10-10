# libbench — the libraries, call for call

`devtools/sdkbench` compares the two compilers on C that calls no library.
This compares the two libraries: `scene.c` asks each SDK for what a game
asks of it every frame, and `run.py` reports the master cycles each request
costs on luna.

| File | What it is |
|---|---|
| `scene.c` | the rows: the same requests under each SDK's names (`PVS`), with (`CALLS = 1`) or without (`CALLS = 0`) the library calls |
| `run.py` | builds each row twice per SDK, profiles both on luna, subtracts |
| `baseline.json` | OpenSNES's figures; `--check` (CI) refuses +10 % on a row |
| `pvsneslib_reference.json` | PVSnesLib's figures and the commit they were measured at |

```sh
make bench-lib                       # against the committed reference
PVSNESLIB_HOME=~/pvsneslib make bench-lib UPDATE=1    # measure both, rewrite the files and docs/PERF.md
python3 devtools/libbench/run.py --only oamxy --detail
```

A row that calls nothing but setters does not include the frame boundary;
`text` and `frame` do, because that is where each SDK's handler sends what
the calls prepared. `idle` is that handler alone.

The first run, on 2026-10-09, had OpenSNES behind on seven rows of eight —
the compiler had been measured for months, the library never against
PVSnesLib's. What changed is in the CHANGELOG of 1.0.0.
