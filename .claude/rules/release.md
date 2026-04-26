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

1. **Full rebuild**: `make clean && make` — zero warnings
2. **Full test suite**: `cd tools/opensnes-emu && node test/run-all-tests.mjs --allow-known-bugs` — all checks pass
3. **Mesen2 validation**: test key examples manually (at minimum: hello_world, one sprite example, one audio example)
4. **CHANGELOG.md updated**: new version section at top with all changes since last release
5. **`compiler/PINS.md` matches `compiler/{cproc,qbe,wla-dx}` HEADs**: `make verify-toolchain` exits 0

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

## Version Tagging

```bash
git tag -a vX.Y.Z -m "vX.Y.Z — brief description"
git push origin vX.Y.Z
```

Semantic versioning: MAJOR.MINOR.PATCH
- MAJOR: breaking API changes
- MINOR: new features (new modules, new examples, new tools)
- PATCH: bug fixes, documentation, build improvements
