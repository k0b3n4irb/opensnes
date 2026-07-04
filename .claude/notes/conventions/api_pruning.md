# API pruning: unused = removed, not deprecated

Maintainer directive (2026-07-04, oamSetVisible removal): **an unused or
never-functional public API gets removed, not deprecated.** The SDK stays
simple and fluid — no superfluous surface, no compatibility shims for
functions nobody calls.

## How to apply

1. Prove non-usage first: grep the whole tree (`lib/`, `examples/`,
   `docs/`, `compiler/ABI.md`, tutorials) — not just examples. Constants
   or types that only served the removed function go with it.
2. Leave a short tombstone comment at the removal site in the header when
   the *absence* is itself informative (e.g. "there is deliberately no
   oamSetVisible — visibility is Y-based"), so the API isn't reinvented.
3. Update any doc that taught the removed call (TROUBLESHOOTING checklists,
   tutorials) to teach the real model instead.
4. Full validation: visual compare (bit-shifts move layouts), WRAM
   rebaseline if lib object sizes changed, libtest, ABI lint.
5. Removal is a breaking change → mention in CHANGELOG at the next release
   (MINOR bump pre-1.0).

## Precedent

- `oamSetVisible` + `OBJ_SHOW`/`OBJ_HIDE` (commit 8e8e89f2): the show
  direction was a structural no-op, the corpus's single call site was that
  no-op, the constants served nothing else. First deprecated (50f8932d),
  then the maintainer chose removal over deprecation.

Cross-ref: `PHILOSOPHY.md` non-goals (no superfluous surface).
