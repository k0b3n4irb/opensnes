# cproc upstream, read against our fork (2026-10-11)

Our frontend is a fork of cproc (Michael Forney, `git.sr.ht/~mcf/cproc`,
mirror `github.com/michaelforney/cproc`, issues disabled there). The fork
left upstream at `7051114`. This note is the review of what upstream did
since, and what we took. Redo it when upstream has moved by a few dozen
commits; the command at the end lists them.

## What was read

97 upstream commits after `7051114`, up to upstream's tip as fetched on
2026-10-10.
Each was tried alone on our tree (`git cherry-pick -n`), then read.

| | Count |
|---|---:|
| Already carried (earlier backports) | 6 |
| Apply alone without conflict | 43 |
| Conflict with our changes | 48 |

"Applies cleanly" says nothing about whether we want it: most of the 43 are
build recipes for other systems, documentation and tests.

## What it found on our compiler

Each line was reproduced on our compiler before anything was changed.

| Defect on our side | Kind | Upstream | Our commit |
|---|---|---|---|
| `"\x7Fw"` is one byte, 0xC0: `hexval()` took a to z for digits | **silent**, wrong data in ROM | in with `19b6342` (which we had backported), fixed by `7e9a684` | `b2e4205` |
| `*"a"` stops on "unimplemented expression 2" | loud | `482c69c` + `d30bfcc` | `1071b97` (adapted: the `*&` collapse skips string literals) |
| a literal handed to a plain `char *` (`f("WXYZ")`, `void f(char *)`) is read from bank $00: 0x7E for the Y on a test ROM, link guard silent | **silent**, wrong read | not upstream, ours: found while writing the test of the line above, through `"abc"[i]` which the link refused | `7e6a4e5`: a literal is `const char[]`; the plain pointer is refused, `"abc"[i]` is a far read, a store to a literal is refused |
| `int y; y = a_struct;` stops on "internal error; unsupported conversion" | loud, bad message | `2a70719` | `9877cd2` |
| a flexible array member with an initialiser stops on an assertion in `emitdata` | loud, bad message | `3fcb352` | `d082fc0` |
| `int restrict r;` is accepted | accepts invalid C | `007035e` | `f8483cd` |
| `int a[const 3][const 4]` is accepted | accepts invalid C | `ac51461` | `4e8eb9c` |
| `for i = 0;` says "expected '(' after while" | message | `96367c3` | `9ff8eb3` |

The hex escape is the only upstream defect that produced a wrong ROM
without a word; the literal handed to a plain pointer is ours. Both are in `.claude/notes/status/silent_defects_log.md`. Checks:
`devtools/compiler-tests/cases/hex_escape_stops`, `deref_string_literal`,
and seven refusals under `cases/negative/`, each failing on the compiler
before its fix.

Checked and already right here: `8` is not an octal digit (`576b89c`; our
`octval()` stops at 7).

## What was left, and why

- **Not our target**: floating point conversions, `_BitInt` (`01ef4a3`,
  `b709f58`), variable-length arrays, the BSD / macOS / Alpine / Debian
  build recipes, `-undef` preprocessing (`37f6382`), `extern` for globals
  (`d1c53dd`, which goes with upstream's move to QBE 1.3; not tried on
  our QBE).
- **Features, not fixes** — to take only if a user asks: digit separators
  (`1d37e50`), `a ?: b` (`a558722`), `[[gnu::packed]]` on unions,
  `noreturn` and `section` attributes (`c5a3f28`, `aa7da0f`; we have our
  own section mechanism), `#error` / `#warning` handled by cproc's own
  preprocessor (we preprocess with the host `cc -E`).
- **Refactors** that conflict with our 46 patches and change no behaviour:
  `7ed7742`, `80ed174`, `5c85062`, `866d74a`, `0833cbb`, `3666542`,
  `6e8426c`.
- **More thorough `restrict` checking** (`5f90023`) and the better message
  for a bad struct assignment (`ee95238`): they sit on the refactors above.
  The simple forms are refused since this backport.

## The two optimisation series, measured (2026-10-11)

Two upstream series change the IR cproc hands to QBE. Each first commit
applies alone; both were built in a worktree and measured on
`make bench-sdk` (21 workloads, 71,772,676 master cycles before).

| Commit | Effect | Decision |
|---|---|---|
| `863e1e7` switch: no redundant `ceq` in a leaf | `switch` 1,100,558 -> 1,066,794 (-3.1 %), `state` 995,956 -> 968,636 (-2.7 %), nothing else moves; 85 bytes less | **taken** (46th patch), compiler check `switch_dense_leaf` |
| `84107ab` branches: `funcbranch`, a condition compiled as a branch | `strings` 2,007,864 -> 1,960,282 (-2.4 %), `depot` +0.008 %, nothing else; the compiler check `long_condition` fails (the pattern it pins for a 32-bit condition on the right of `&&` is no longer produced) | **not taken**: one workload gains, a pinned shape is lost, and its three follow-ups (`e9ee698`, `9afa9df`, `92ef417`) conflict with our changes. To reopen only with the `long_condition` shape understood |
| `f68a36f` switch: min/max in the case search | conflicts with our changes | not tried |

A first reading of these figures was void: the build had failed on the
patch count of `compiler/PINS.md` and the benches ran on the old binaries,
giving the reference to the cycle. A trial script stops on a failed build.

## Not recommended: a rebase onto upstream

48 of 97 conflict, most through the refactors, and nothing upstream added
since the merge-base is something the 65816 target is missing. The fork
keeps picking fixes one by one, each recorded in `compiler/PINS.md` with
its upstream sha.

## Redo it

```sh
git clone https://git.sr.ht/~mcf/cproc /tmp/cproc-upstream
git -C /tmp/cproc-upstream log --oneline 7051114..origin/master
```

A cherry-picked commit keeps its upstream author in the fork (the
one-author rule is about this repository's history, not the submodule's);
the committer is the maintainer.
