# C conformance on the target: gcc.c-torture/execute

`difftest.py` checks integer expressions and small programs against a model. This
runs the programs of a real conformance suite: each test of gcc's
`gcc.c-torture/execute` is a complete C program that calls `abort()` when the
compiler got something wrong and exits 0 otherwise. `run.py` compiles each with
`cc65816`, builds a ROM with the small runtime of `rt.c`, runs it on luna and reads
the result from WRAM.

```
python3 testing/torture/run.py --fetch      # clone the tests at the pinned commit (once)
make test-torture                           # run all, compare with expected.txt (about 25 min)
python3 testing/torture/run.py -v --only 20000113-1,pr58943
python3 testing/torture/run.py --update     # re-capture expected.txt after an intended change
python3 testing/torture/run.py --quick      # the frozen subset of quick.txt (about a minute); in `make tests`
```

The tests are GPL-3.0 and are **never committed**: they are fetched into `build/torture/`
at the commit pinned in `run.py` (`GCC_SHA`). What is committed is ours: `run.py`,
`rt.c` (abort, exit, mem*, str*), `prelude.h` (what the tests' libc and compiler would
provide), `shim/` (headers) and `expected.txt`, the status of each test.

## Reading the result

| Status | Meaning |
|---|---|
| PASS | built, ran, exited 0 (or returned 0 from `main`) |
| FAIL | built, then `abort()`, a non-zero exit, or no end in 150 M instructions |
| NOBUILD | compiled, then did not link, assemble or fit |
| REFUSED | `cc65816` refused it: a feature the SDK says it does not handle (`long long`, `va_start`, `asm`, struct by value, VLA, K&R, nested functions, floating point, GNU attributes it refuses). A refusal is loud: it is not a silent miscompilation. |

The ratchet: a test that `expected.txt` lists as PASS and that no longer passes fails the
run. A test that improved is reported (`--update` records it). The weekly workflow
`torture.yml` runs it.

## The failures, explained (third column of `expected.txt`)

Every FAIL is read, not assumed. At the pinned commit none is an arithmetic or control-flow
miscompilation: ten tests declare themselves `int32plus` (they need a 32-bit `int`; this target's
is 16), others ask for a gcc builtin the SDK does not have, a 500-deep call chain the stack of
the machine cannot hold, an evaluation order gcc chose among the unspecified ones, or a plain
pointer to a string literal (the documented bank limit). Two tests found real defects of
another kind: the GNU attributes `mode` and `vector_size` change a type and are accepted **and
ignored** (`sizeof` of a `mode(QI)` type is 2, of a `vector_size(8)` type 2), so such code
compiles to the wrong thing without a word; see `KNOWN_LIMITATIONS.md`.

## What this does not prove

It runs on luna only, like everything else here; no console has run it. It covers the C that
cproc accepts: more than half of the suite is refused up front, by design.

## The frozen subset, and the proof that it sees what it should

`make tests` runs `--quick`: the 26 tests of `quick.txt`, in about a minute. Here ANY difference from
`expected.txt` fails, in both directions (a pass that fails, a refusal that starts to compile): they are
the tests that pin behaviours we mean to keep. `own/` holds tests written for this SDK, in the same
format, for what no gcc test exercises.

Shown to fail with a fix undone, by building the cproc of the pin before it and running the subset on it
(`TORTURE_BIN=<a bin/ with that cproc-qbe>`):

- **`fdcda7c8`** (a string literal is `const char[]`; passed to a plain `char *` it was read from the wrong
  bank in silence): the 10 refusals it causes change: 6 compile and pass, 2 do not build, and 2
  (`20030920-1`, `961125-1`) fail at run time. Exit code 1.
- **`ee5b8636`** (a hex escape stops at the first non-hex character): no test of the gcc suite has a `\x` followed
  by a letter above `f`, so `own/hex-escape-stops.c` pins it: `FAIL abort` with the old cproc. Exit code 1.
