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
- **Ported so far** (have a `.checks`, all passing): `const_data`,
  `section_directives`, `tail_call`, `nonleaf_frameless`, `arg_push_order`,
  `shift_right`, `word_extend`, `multiply`, `return_value`, `static_vars`.
- **TODO**: the remaining ~50 fixtures (`run.py --list`). Port each by reading its
  original check in the opensnes-emu `compiler-tests.mjs`
  (`gh api repos/k0b3n4irb/opensnes-emu/contents/test/phases/compiler-tests.mjs?ref=feat/functional-probes`)
  and translating its assertions into a `.checks` file. A few checks are bespoke
  (ordered sequences, epilogue tax/txa proximity); extend the DSL in `run.py` if a
  rule doesn't fit `present`/`absent`/`count`/`in`/`section`.
