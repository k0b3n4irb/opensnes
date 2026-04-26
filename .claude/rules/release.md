# Release Workflow (Auto-loaded)

## Pre-Release Checklist

1. **Full rebuild**: `make clean && make` — zero warnings
2. **Full test suite**: `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick` — 212 checks pass
3. **Mesen2 validation**: test key examples manually (at minimum: hello_world, one sprite example, one audio example)
4. **CHANGELOG.md updated**: new version section at top with all changes since last release

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
