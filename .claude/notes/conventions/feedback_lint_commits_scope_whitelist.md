---
name: lint_commits.py scope whitelist (no `tutorials`, no `roadmap`)
description: Conventional Commits scope must be in the project's whitelist or CI lint fails. Twice burned by `docs(roadmap):` and `docs(tutorials):` — both rejected.
type: feedback
originSessionId: 7cf8aa6f-20b3-4e2f-81b4-25748e91b08f
---
`devtools/lint_commits.py` enforces a strict scope whitelist:

> `build, changelog, ci, claude, compiler, contributing, deps, devtools,
> docs, examples, lib, readme, release, runtime, submodule, test, tests,
> tools`

Commits with non-whitelisted scopes (`docs(roadmap):`, `docs(tutorials):`,
etc.) **fail the `commit-messages` job** in `.github/workflows/lint.yml`
and require a force-push to fix in develop's history.

**Why:** The project's recent log uses bare `docs:` (no scope) for
documentation commits — see `6897fd0 docs: mention KNOWN_LIMITATIONS
refresh in v0.16.0 changelog`, `cb709ba docs: refresh KNOWN_LIMITATIONS
for v0.16.0`, etc. Specific scope only when documenting a whitelisted
area: `docs(lib)`, `docs(tools)`, `docs(examples)`.

**How to apply:** When writing a documentation commit, pick the scope by
asking "what whitelisted area is the doc *of*?":

- `docs/tutorials/hdma.md` → `docs:` (it's a tutorial — no whitelisted
  scope fits exactly; bare form is safest)
- `lib/include/snes/X.h` Doxygen edit → `docs(lib):`
- `tools/README.md` → `docs(tools):`
- `examples/*/README.md` → `docs(examples):`
- `ROADMAP.md` → `docs:` (no `roadmap` scope)
- `KNOWN_LIMITATIONS.md` → `docs:` (no `known-limitations` scope)
- `CHANGELOG.md` → `docs:` or `docs(changelog):` (changelog IS in the
  whitelist)

Run `python3 devtools/lint_commits.py origin/develop..HEAD` locally before
pushing if the scope is anything other than the obvious whitelisted ones.
The check takes < 1 second.
