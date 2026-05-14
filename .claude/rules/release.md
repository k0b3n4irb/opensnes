# Release Workflow (Auto-loaded)

## Branch model (see CONTRIBUTING.md for the full policy)

- `develop` = active development. PRs land here. CI build must pass.
- `main` = stable. Only updated via a release PR from `develop`. Full test
  suite must be green at every commit.
- Tags `vX.Y.Z` are cut **only from `main`**. `release.yml` rejects any tag
  whose commit is not on `main`.

## Release flow

```
develop  ─●─●─●─●─●─●  (active work, all PRs land here)
                    │
                    ●  release PR — bring develop's green tip into main
                    │
main     ──────────●─●  (fast-forward; tag is the second ●)
                      │
                      v0.X.Y  (tag pushed → release.yml runs)
```

1. Open a release PR `develop → main` titled `release: vX.Y.Z`. Body lists
   the CHANGELOG entries.
2. CI must be green on `develop`'s tip (build + functional-tests).
3. After merge, tag the new `main` head: `git tag -a vX.Y.Z -m "..."` then
   `git push origin vX.Y.Z`.
4. `release.yml` runs the tag-on-main guard, then builds the per-OS
   release zips and creates a GitHub Release with the assets.

## Pre-Release Checklist

1. **Full rebuild**: `make clean && make` — zero C-side warnings; bank $00
   nearly-full warnings tolerated only above `BANK0_FAIL_THRESHOLD` (see
   `.claude/rules/bank0_budget.md`).
2. **Full test suite**: `cd tools/opensnes-emu && node test/run-all-tests.mjs` — all checks pass. The `--allow-known-bugs` escape was retired in chantier A3 (2026-05-09); the optimisations it gated had all shipped, the markers were stale.
3. **Mesen2 validation**: test key examples manually (at minimum: hello_world, one sprite example, one audio example)
4. **CHANGELOG.md updated**: new version section at top with all changes since last release
5. **Version macros bumped**: `lib/include/snes.h:30-40`
   (`OPENSNES_VERSION_{MAJOR,MINOR,PATCH,STRING}`) updated to match the
   new `## [X.Y.Z]` heading. **In the same commit as the CHANGELOG bump.**
6. **`ROADMAP.md` "Current Status: post-vX.Y.Z" line bumped** to match
   the new release.
7. **`make lint-docs` exits 0** — version macros + ROADMAP status line +
   examples count consistency. See `.claude/rules/doc_consistency.md`
   for the full policy.
8. **`compiler/PINS.md` matches `compiler/{cproc,qbe,wla-dx}` HEADs**: `make verify-toolchain` exits 0
9. **`make lint` exits 0** — runs the aggregate lint (docs + ASM markers + commits).

## CHANGELOG Format

Follow the existing format in `CHANGELOG.md`:

```markdown
## [vX.Y.Z] — YYYY-MM-DD

### Added
- feat(scope): description

### Changed
- refactor(scope): description

### Fixed
- fix(scope): description

### Performance
- perf(scope): description
```

Group changes by type. Each entry uses Conventional Commits prefix with scope.

## Commit Conventions

See `.claude/rules/commits.md` for format and restrictions.

## PR Rules (from CONTRIBUTING.md)

- One topic per PR — never mix a bug fix with a refactor
- Keep PRs small: < 200 lines ideal, 200-400 acceptable, > 400 needs justification
- Asset files (.pic, .pal, .map) don't count toward line limits
- New code must have Doxygen documentation (`/** @brief */`)
- Binary assets must be from PVSnesLib or original work (track in ATTRIBUTION.md)

## Working branches (wip/*) policy

Internal multi-day chantiers ship via short-lived `wip/<name>` branches:

1. **Create** from current develop tip: `git checkout -b wip/<name>`.
2. **Work** with as many WIP commits as needed for safety/restore points.
3. **Validate** on the wip branch: `make clean && make`, full quick suite,
   Mesen2 spot check on representative examples.
4. **Squash-merge** into develop as ONE coherent commit with a clear
   conventional-commit subject summarising the chantier's outcome.
5. **Delete** the wip branch immediately (local AND `origin`) after the
   merge lands. Don't keep it as an archive — the commit message on
   develop is the archive.

The squash-merge keeps develop history scannable (one chantier = one
commit). WIP commits' details remain accessible via `git reflog` and
the wip branch's history in `git log <sha>` for as long as `origin`
keeps the SHA (until garbage collection).

Anti-pattern: leaving `wip/*` branches lying around after merge. This
project's history was polluted with 5 stale `worktree-agent-*` branches
+ a `backup/develop-pre-reword-*` until cleanup on 2026-05-12 — be the
person who deletes their wip branches.

## Version Tagging

```bash
git tag -a vX.Y.Z -m "vX.Y.Z — brief description"
git push origin vX.Y.Z
```

Semantic versioning: MAJOR.MINOR.PATCH
- MAJOR: breaking API changes
- MINOR: new features (new modules, new examples, new tools)
- PATCH: bug fixes, documentation, build improvements
