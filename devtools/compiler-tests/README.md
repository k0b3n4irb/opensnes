# Compiler C→ASM pattern checks

Compile-time regression guards for **cc65816** codegen. Each `cases/<name>.c` is
compiled with `bin/cc65816` and its assembly is matched against declarative rules
in `cases/<name>.checks`. No emulator involved.

```bash
make test-compiler                              # or:
python3 devtools/compiler-tests/run.py          # run all cases with a .checks
python3 devtools/compiler-tests/run.py --only tail_call
python3 devtools/compiler-tests/run.py --list   # fixtures still missing a .checks (TODO)
```

## `.checks` DSL
```
present <regex>                  ASM contains regex
absent  <regex>                  ASM does not contain regex
count   <N> <regex>              exactly N matches
in <func>: present|absent <re>   regex (a)present within that function's body
section <sym>: present|absent <re>   the .SECTION/.RAMSECTION line of <sym> matches
raw <directive>                  the same directive on the unstripped ASM: a file-scope
                                 static is emitted `name.<case>` and the runner strips
                                 that suffix before the other checks run
# lines and blank lines are ignored
```

## Status & provenance

These were the "60 compiler tests" that lived **inside the removed `opensnes-emu`
submodule** (`test/phases/compiler-tests.mjs`, itself a JS port of the even older
`tests/compiler/run_tests.sh`). They were lost when that submodule was removed
during the luna migration, then **re-homed here** — dependency-free, in Python,
alongside the other `devtools/` linters. See
`.claude/notes/chantiers/luna_migration.md`.

- **Fixtures**: all 66 `cases/*.c` recovered from the opensnes-emu repo
  (`feat/functional-probes` ref).
- **Ported so far**: every fixture (76/76, `MAX_UNCHECKED = 0`). The first
  ten (`const_data`, `section_directives`, `tail_call`, `nonleaf_frameless`,
  `arg_push_order`, `shift_right`, `word_extend`, `multiply`,
  `return_value`, `static_vars`), then C4 batch 1 (2026-09-15, 14
  fixtures), then C4 batch 2 (2026-09-26, the last 41). Batch 2 translated
  the opensnes-emu check where one existed and otherwise asserted the
  property the fixture's comment names; most original checks only looked
  for a symbol, so the new ones pin values, offsets and control flow.
  Phi fixtures use regex backreferences so the check is the pairing itself
  (an update's source slot is the slot its result is written back to), and
  each file was verified to fail on a hand-made mutation of its property.
  Where a check pins frame offsets (`test_variable_shift_bug`,
  `test_forloop_array`), its comment says how to re-derive them.
- **New fixtures** ship with a `.checks` (the ratchet is at 0). If a rule
  doesn't fit `present`/`absent`/`count`/`in`/`section`, extend the DSL in
  `run.py` rather than weakening the check.

## Runtime fixture ROMs (`testing/fixtures/compiler/`, since 2026-10-05)

Pattern checks prove shapes; these ROMs prove results. Each directory is a
one-TU ROM whose globals the sibling `test_*.py` asserts through
`luna state --assert`, all rebuilt from clean and run by `make tests`:

| ROM | Proves |
|---|---|
| `a7_32bit` | 32-bit (`Kl`) arithmetic: add/sub carry, mul, div, mod, shifts by constant, signed folds |
| `a6_farptr` | the 4-byte pointer ABI: bank byte through every lib boundary |
| `b2_far_ram` | `FAR` objects in bank $7E with bank-honouring codegen |
| `debug_channel` | the debug channel to luna |
| `c_features` | switch (dense / sparse / fall-through / negative / 32-bit), function pointers (const and RAM tables, callback, struct member), bit-fields, enum, goto, recursion, 32-bit `* / %` and variable shifts on runtime operands, promotions and truncations, signed division rounding, short-circuit side effects, `sizeof` of the target's types, const far reads (gaps review C2) |

## Refusal fixtures (`cases/negative/`)

`negative/<name>.c` must **fail** to compile and stderr must contain
`<name>.expect`. They pin the three C features the toolchain does not
implement — variadic functions, struct by value (parameter, return,
assignment), inline assembly — as clean refusals that name the feature
(gaps review C1/C2). A negative that starts compiling fails the run: the
feature landed, promote it to `cases/` with real `.checks`.
