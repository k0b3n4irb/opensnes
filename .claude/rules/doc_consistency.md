# Doc Consistency Rules (Auto-loaded)

CRITICAL: Three classes of doc/code drift have caused hard-to-trace problems
in this project's history. They are now mechanically blocked by
`devtools/check_doc_drift.py`, which is wired into `make lint-docs` and the
`doc-drift` job in `.github/workflows/lint.yml`. Read this file before
touching any doc that names a version, an examples count, or the framework
opt-in list.

## What the sentinel watches

1. **`lib/include/snes.h` version macros** must match the head version in
   `CHANGELOG.md`. Ship-blocker: a public macro that lies. Caught
   historically as `OPENSNES_VERSION_STRING "0.1.0-dev"` while the project
   was at v0.16.0 — every release shipped wrong macros for months.

2. **`ROADMAP.md` "Current Status: post-vX.Y.Z" line** must match the
   `CHANGELOG.md` head. Caught historically as ROADMAP saying
   "post-v0.13.0 (developing toward v0.14.0)" while the project was at
   v0.16.0 — three minor versions stale.

3. **Examples count claims in active docs** (`ROADMAP.md`, `README.md`,
   `.claude/rules/*.md`) must match `find examples -name 'main.c' | wc -l`.
   Caught historically as the pre-v0.16.0 count (one off the current
   total) sticking around in `testing.md` and `nmi_audit.md` after
   v0.16.0 shipped the new `scene_stack` example. `CHANGELOG.md` is
   exempt — historical entries freeze a past state on purpose.

## Mandatory workflow

Before committing any change that touches one of those classes:

```sh
make lint-docs
```

It exits 0 on green, 1 on drift with an actionable message per finding.

## When to update each anchor

- **Version macros (`lib/include/snes.h:30-40`)**: in the same commit as
  the new `## [X.Y.Z]` heading in `CHANGELOG.md`. The pre-release checklist
  in `.claude/rules/release.md` enforces this.
- **ROADMAP "Current Status"**: at release-PR time, when CHANGELOG gets
  the new heading. One line, one commit.
- **Examples count claims**: prefer not to hard-code numbers in active
  rules. The canonical pattern is "every example" / "the full suite" —
  see `testing.md:14` and `nmi_audit.md:57` for the form. Hard-coded
  numbers belong in changelog snapshots and release notes only.

## Adding a new anchor

If a new class of drift causes you pain ("test count moved and three docs
disagreed", "module list rotted", etc.), extend
`devtools/check_doc_drift.py` with a new `check_*` function, add a section
to this file, and add an entry to the workflow under `doc-drift`. Don't
solve a class of drift twice — solve it in the sentinel.

## What NOT to add to the sentinel

- Anything inherently dynamic (test counts that move every chantier; the
  full corpus is covered by `tools/luna-test/luna_runner.py --coverage`).
- Pure prose (commit messages, README narrative). The lint should catch
  drift in *anchored claims*, not in writing.
- Anything CHANGELOG-frozen by design.

## When this rule does NOT apply

- Editing CHANGELOG entries for past releases.
- Files under `docs/` that intentionally describe a past state (release
  notes, migration guides).
- This file itself.
