# QBE upstream: we keep our fork, we take their changes, we do not contribute

Owner decision, 2026-10-13: "on reste sur notre fork, on le fait vivre,
backport leur changes mais on ne contribue pas avec eux".

## What it means

- **Nothing is sent upstream**: no mail to `~mpu/qbe@lists.sr.ht`, no ticket
  on `todo.sr.ht/~mpu/qbe`, no issue on a mirror. The report on
  `defwidthle()` prepared on 2026-10-11 and rewritten on 2026-10-13
  (`2026-10-13_to_qbe_defwidthle.txt`) is **not sent** and stays here as
  the record of the defect and of its reproducer. A session does not
  reopen this on its own: it is the owner's to change.
- **The fork lives**: `git@github.com:k0b3n4irb/qbe.git`, branch
  `feat/b2-far-qualifier`, pinned in `compiler/PINS.md` with one line a
  patch. It is ours to maintain, tests and all (`make test-toolchain-suites`
  runs upstream's own suite on it).
- **Their changes are read and taken when they serve us**: a cherry-pick,
  adapted if needed, with its line in `compiler/PINS.md` naming the
  upstream commit, under the Class A protocol like any compiler change.

## Where upstream is

Canonical repository `git://c9x.me/qbe.git` (remote `upstream` in
`compiler/qbe`). Patches and discussions go to the mailing list
`~mpu/qbe@lists.sr.ht`, which is active; a tracker exists at
`todo.sr.ht/~mpu/qbe` ("issues & wishes", six open tickets, the latest two
years old on 2026-10-13); `github.com/8l/qbe` is an unofficial mirror
stopped in 2017. Checked on 2026-10-13.

## The watch: once a month

    git -C compiler/qbe fetch upstream
    git -C compiler/qbe log --oneline <last tip read>..upstream/master

Read each new commit by the method of
`.claude/notes/tech/qbe_upstream_review.md`: only the passes our pipeline
runs can matter (promote, ssa, loadopt, coalesce, gvn with `copy.c`, gcm,
simpl, the parser); `spill`, `rega` and the other targets do not. For a
candidate: apply it alone in a worktree, measure (`make bench`,
`make bench-sdk`), run the hunts. Write the verdict in that note, taken
or not, and why. A defect of theirs that we find is fixed here and written
down here.

| Date | Upstream tip read | New commits | Taken | Next check due |
|---|---|---|---|---|
| 2026-10-11 | `e786f06` | 41 since our base `120f316` | `863e1e7` (switch, via cproc's twin); `b58e2e6` already carried | |
| 2026-10-13 | `e786f06` (unchanged) | 0 | — | 2026-11-13 |

## Known difference from upstream that is a defect of theirs

`defwidthle()` since `e01207f`: a phi whose width proof failed keeps its
mark and is then taken for proven; an extension is removed that is needed
(`f(0x1234, 1)` returns 0x1234 instead of 0x34 on their `e786f06`, arm64,
re-run on 2026-10-13). We never took `e01207f`. If a later upstream commit
depends on it, read `qbe_upstream_review.md` before cherry-picking.
