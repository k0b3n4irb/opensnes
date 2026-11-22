# I1 S3: `noinline`, the checks that need a function to stay, the gapbench twins

Step S3 of `I1_inline_plan.md` (section 1.4, S3 in section 5, H9 in section 7). Branch `i1-s3`, from `develop` `c20a4e7e`,
cproc `82e1c1d` -> `18eecca`, qbe `7bd8684` -> `0f52456`. Pair: Opus writes and measures (no sub-agent on this step).
Names neutral: jeu 1, jeu 2, the games. The S4 size rule is out of this step: the inlining decision is not touched.

## Prediction (the plan's, written before S1 and committed in `c20a4e7e`; copied here, not rewritten)

- Every `.c.asm` of the library and the corpus byte-identical (no code writes `noinline`; the checks' output is unchanged
  while two sites still block).
- `callst` / `chainst` read about the same as `call` / `chain` today: the same code with a `static`, so within 2 %, since a
  static leaves out its `rep #$20`.
- Privately: the number of `noinline` in the games, expected 0 (H9).
- Proof: the checks pass; with `noinline` stripped out by a fault (`QBE_INLINE_FAULT=noinline`), the new `noinline` check
  fails.

## What was built

- **cproc** (`attr.c`, `cc.h`, `decl.c`, `qbe.c`): the GNU attribute `noinline` (`__attribute__((noinline))`,
  `__noinline__`, `[[gnu::noinline]]`) sets a flag that `decl()` takes per declarator: one among the specifiers holds for
  every declarator of the declaration, one in a declarator for that declarator, one on a parameter or a structure member
  for none (`parameter()` and `structdecl()` restore the flag). It is merged across the declarations of a function (a
  prototype's holds for the definition, as `__ramcode` is) and emitted as the `noinline` linkage keyword before
  `function`. On an object or a type it is ignored, as GCC does (GCC warns; cproc has no located warning, so nothing is
  printed, as for the other hints it skips).
- **QBE** (`parse.c`, `all.h`, `inline.c`): the keyword `noinline` (`Tnoinline`; the lexer's perfect hash took it with no
  collision, `lexinit`'s assertion holds) sets `Lnk.noinline`. `inline_record` copies it to the record; the small inliner's
  `eligible` requires its absence, `auto_inline_ok` refuses with the reason `noinline` (after `self`, before `exported`:
  the trace names it first), and the leaf rule's guess (off by default) does not count such a callee as absorbable.
  `inline_fully_consumed` needs nothing: a declined call of an `inline` function is counted declined and the standalone is
  emitted. `CC_TRACE_INLINE` appends ` (noinline)` at the end of a `record` line (after the existing fields, so
  `i1_calib.py`'s patterns still match). `QBE_INLINE_FAULT=noinline` forgets the keyword (the provoked fault).
- **Check `noinline`** (`devtools/compiler-tests/cases/noinline.{c,checks}`): a one-site static marked `noinline` stays a
  function and its caller calls it (`jml` in tail position); the same body unmarked is absorbed; the C23 spelling; the
  attribute on the prototype only; a `static inline` small enough for the small inliner and marked `noinline` stays a
  call; a `static inline` of two sites marked `noinline` keeps both `jsl`; a parameter carrying the attribute does not
  mark the function (absorbed); on an object the attribute is ignored (an ordinary initialised global, read with
  `lda.w obj`).
- **The checks and fixtures that need a static to stay**: see "What the plan said about the eight checks" below.
- **gapbench**: rows `callst` and `chainst` (`kernels.c`, the driver table of `kernels.asm`, the assembly twin is the same
  label as `call`'s / `chain`'s). `k_callst_f` and the five functions of the chain (`chs_update`, `chs_test`,
  `chs_clamp`, `chs_anim`, `chs_decay`) are `static` and each has two call sites in the unit (the second in
  `callst_site2` / `chainst_site2`, global, never run); `CC_TRACE_INLINE` shows every call to them declined for `sites`
  with `rest=ok` and nothing absorbed. Same setup, data, checksum and assembly as `call` / `chain`. `run.py`: the rows
  ending in `st` whose base is a row are "another way in C" (shown with `*`, not in the means, as `fillu`, `coli`,
  `rowi`, `entsi`), and their assembly bytes are the base row's. `baseline.json` gained the four new entries only (the
  other rows' figures are kept: the new code moved some routines in the ROM, by 2 or 3 master cycles in tens of
  thousands, under the 0.2 % tolerance). `docs/BENCHMARK.md`'s tables do not list the gapbench rows: unchanged (anchors 13
  and 22 green).
- `compiler.md` (the inlining entry), `KNOWN_LIMITATIONS.md` (`noinline` is no longer a skipped hint),
  `docs/BENCHMARK.md` (one sentence in the inlining paragraph), `devtools/gapbench/README.md`, CHANGELOG.

## What the plan said about the eight checks, and what holds

The plan (section 0) lists eight compiler checks and the fixture `d_quals` as keeping a static function a function by its
two call sites. **Read against the trace, the list does not hold for six of the eight.** Every case and every compiler
fixture was compiled with `CC_TRACE_INLINE=1` (and the case's `env` lines), keeping the calls declined for `sites` alone
(`rest=ok`: a size rule could copy them):

| case / fixture | static functions held by their site count | marked `noinline` |
|---|---|---|
| `loop_x_counter_home`, `mul_kl_const_narrow`, `peep_late`, `test_acache_pha`, `test_variable_shift_bug` | none: no `static` function at all | no (nothing to mark) |
| `ramcode_not_inlined` | none: `wait_in_ram` is refused for its section, `twice` has one site and is absorbed on purpose | no |
| `auto_inline`, `auto_inline_leaf_rule` | `twice` (and `down`, recursive) | **no**: `twice` IS the subject (the line "two call sites: stays a function" pins today's rule). A size rule changes that rule, and the check then fails loudly (`raw present \ntwice\.`, `jsl twice` twice), which S4 must answer on purpose |
| `static_no_rep` | `helper` | **yes**: the case is about the entry of a static function |
| `static_tu_suffix` | `bump` | **yes**: the case is about a static function's name |
| fixture `d_quals` | `rdp` (and `dirty`, held by its alloc) | **yes, both**: `dirty` must fill the stack that `zarr` / `zstruct` then use, `rdp` must read through a pointer it cannot see the target of |
| fixtures `c_features`, `jump_table`, `leaf_split`, `inline_label_cut` | `sparse`, the recursive pair, `fact`, `fib`; `fold`, `sweep_small`; `same`; `sweep` | no: values checked against a host model, whatever the inliner does (and `jump_table`'s static switch helper of two sites is the very case S4 wants copied, plan section 4) |

The marked sources compile to the same text as before (`cmp` of the `.asm` of each before and after the attribute, the
same compiler: identical): the second call sites are kept, the attribute only says what they used to imply.

## Result (measured 2026-11-14, after the code above)

| point | predicted | measured | |
|---|---|---|---|
| `.c.asm` and `.sfc`, library and corpus | byte-identical | **1 087 files, 0 differ** (65 `.sfc`, every `.asm` under `examples/` and `lib/`, the four object sets), no file appears or disappears | held |
| `callst` against `call` | within 2 % | **-2.25 % SlowROM** (63 043 / 64 494), **-2.12 % FastROM** (54 928 / 56 115) | **missed by a hair**, direction right |
| `chainst` against `chain` | within 2 % | **-2.20 % SlowROM** (81 380 / 83 210), **-2.09 % FastROM** (70 280 / 71 777) | **missed by a hair**, direction right |
| checksums of the twins | the same as `call` / `chain` | 13 401 and 28 225, both rows, both speeds | held |
| `noinline` in the games (H9) | 0 | 0 (private grep of both games' C sources) | held |
| the new check under the fault | red | `QBE_INLINE_FAULT=noinline`: `noinline` fails (10 assertions), the 155 other checks pass | held |

- **Why 2.1 to 2.25 % and not under 2 %**: the whole difference is the `rep #$20` a static function leaves out. `callst`
  runs 1 451 master cycles under `call` SlowROM and 1 187 FastROM for its 64 calls: **22.7 and 18.5 a call**, one
  `rep #$20` (two opcode fetches and an internal cycle: 8 + 8 + 6 SlowROM, 6 + 6 + 6 FastROM). The plan named the
  mechanism; its bound was too tight for rows that are almost nothing but calls. The two figures were measured twice
  (two full runs of `run.py`, before and after a fix of the assembly byte count of the twins), the same to the cycle.
- The rows are the baseline S4 must move: today they are `call` / `chain` minus one `rep` a call. S4's prediction for
  them (plan, H12) is unchanged by this.

## Gates

- `make clean && make`: exit 0; the same two warning lines as the reference build of `c20a4e7e` (CMake deprecation,
  clang `-rdynamic`).
- **Byte-identical**: the 1 087 `.sfc` and `.asm` of `examples/` and `lib/` from `make clean && make` on `c20a4e7e`
  (before any change, `/tmp`), against the same after the change: 0 differ.
- `make test-compiler`: `156 passed, 0 failed (+0 compile-only; 0/0 unchecked ratchet; 34/34 refusals pinned)`.
  Under `QBE_INLINE_FAULT=noinline`: `155 passed, 1 failed` (the `noinline` case).
- `make tests`: `ALL CHECKS PASSED (luna)` (coverage 63 OK / 2 INPUT-DEP / 0 DEAD / 0 FAIL of 65, the same from random RAM; `d_quals` `r_done == 0xBEEF`; gapbench `OK, nothing above baseline.json` with the two new rows)
- `make lint`: exit 0. `make lint-docs`: `OK: no doc drift detected.`
- `make verify-toolchain`: `OK: toolchain pins match (3 submodule(s) verified)` (PINS.md names cproc `18eecca`, 51 patches, and qbe `0f52456`, 157 patches)
- Differential hunts: `difftest: 2000/2000 seeds clean, 48000 expressions in 4 shapes`; `difftest-stmt: 2000/2000 seeds clean, 20000 programs`
- Not run: `make test-sanitizers` (the new code is a flag read and two comparisons; the output is byte-identical on the
  corpus by the gate above) and `diff_corpus.py` (the ROMs are `cmp`-identical, so the A/B at equal frame has nothing to
  compare).
- The submodule commits are local (branch `i1s3` in `compiler/cproc` and `compiler/qbe`): not pushed to the forks. They
  must reach `k0b3n4irb/cproc` and `k0b3n4irb/qbe` (with S2's `7bd8684`, also local) before `develop` can carry this
  step.

## Replay

```
git checkout c20a4e7e && make clean && make                     # the reference
find examples lib -type f \( -name '*.sfc' -o -name '*.asm' \) | sort > list; tar cf ref.tar -T list
git checkout i1-s3 && make clean && make                        # then cmp each file of list against ref.tar
python3 devtools/compiler-tests/run.py --only noinline
QBE_INLINE_FAULT=noinline python3 devtools/compiler-tests/run.py --only noinline      # must fail
python3 devtools/gapbench/run.py --json after.json              # callst / chainst against call / chain
CC_TRACE_INLINE=1 bin/cc65816 -Ilib/include devtools/gapbench/kernels.c -o /tmp/k.asm  # the twins' callees: declined, `sites`, rest=ok
```
