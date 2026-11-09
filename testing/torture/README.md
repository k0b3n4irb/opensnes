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

## The refusals, triaged (2026-11-09)

The 952 tests `cc65816` refuses are tagged in `expected.txt` (third column). By tag, and what each means:

| Tag | Tests | What it is |
|---|---|---|
| `k-and-r` | 220 | Pre-C99 C: old-style function definitions, implicit `int`. cproc is a C11 front end and does not read it. By design. |
| `long-long` | 164 | `long long`: the widest integer of this target is `long`. Documented. |
| `float` | 76 | No FPU and no soft-float: refused, use the fixed-point types. Documented. |
| `struct-by-value` | 69 | Struct parameters and returns: refused. Documented. |
| `gnu-builtin` | 66 | `__builtin_*` functions and `__label__` the SDK does not have. |
| `varargs`, `asm`, `vla`, `nested-function` | 51, 44, 19, 13 | Refused, each with an entry in `KNOWN_LIMITATIONS.md`. |
| `syntax`, `front-end-gap` | 37, 20 | A construct the front end does not parse: GNU `typeof`, statement expressions, a call of an unprototyped function with arguments. Some are valid C11 (a gap to read one by one). |
| `gnu-attribute` | 32 | Refused attributes (`aligned`, and, once the lot of 2026-11-09 lands, `mode`, `vector_size`...). |
| `missing-header` | 29 | `<stdint.h>` and others the SDK does not provide, plus headers local to a gcc test. |
| `const-qualifier` | 24 | A string literal is `const char[]` (`fdcda7c8`): passed to a plain `char *`, it is refused. |
| `bit-field` | 24 | A field wider than the 16-bit `int`, or in a packed struct. |
| `index-swap` | 13 | `i[a]` for `a[i]`: valid C, the front end wants the pointer first. A gap. |
| `compiler-internal` | 14 | **The compiler failed on its own, on C it should take**: 8 assertions in `emitdata`, one `SIGSEGV` (`scope-1`), 3 "slot is read but never stored", one "ssa temporary used undefined", one function too large for the backend. Loud, but defects. |
| `type-strictness`, `test-assumption`, `other` | 3, 3, 30 | Not read one by one yet. |

The 14 `compiler-internal` tests: `20011126-2 20030404-1 memcpy-1 pr23135 pr28982b pr34099 pr34099-2 pr53645
pr53645-2 scope-1 simd-1 simd-2 simd-5 va-arg-22`. Most of the assertions come from `vector_size` and `mode` being
ignored (`vecint i = { 150, 100 }` is then a scalar with a two-element initialiser): refusing those attributes turns them into
clean refusals. What stays is the work: reduce each to a minimal case and fix or refuse it.
