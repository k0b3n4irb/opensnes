# Contributing to OpenSNES {#contributing}

Thank you for your interest in contributing to OpenSNES!

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).
SNES development is niche — every contributor matters.

## Reporting Bugs

1. Search [existing issues](https://github.com/k0b3n4irb/opensnes/issues) first
2. Use the **Bug Report** template — it asks the right questions
3. Include a **minimal reproduction** (a small `main.c` that triggers the bug)
4. Attach screenshots or a short video if it's a visual issue
5. Specify the emulator used (luna, Mesen2, bsnes)

## Suggesting Features

1. Check the [Roadmap](ROADMAP.md) first
2. Use the **Feature Request** template
3. Explain the **use case**, not just the feature

## Submitting Code

### Setup

```bash
gh repo fork k0b3n4irb/opensnes
git clone https://github.com/YOUR_NAME/opensnes
cd opensnes
git remote add upstream https://github.com/k0b3n4irb/opensnes
git fetch upstream develop
git checkout -b feature/my-feature upstream/develop
```

### Branching and release policy

The repository uses two long-lived branches with strict invariants. Knowing
which branch you are on tells you what guarantees apply.

| Branch | Invariant | When it advances |
|--------|-----------|------------------|
| **`main`** | Full test suite is green. Every commit is reachable from a tagged release or about to become one. The default branch — what `git clone` lands on. | Only via PR from `develop` (the "release PR"), or fast-forward when bumping for a new tag. |
| **`develop`** | CI build job is green on the latest commit. May contain in-progress optimisations not yet stabilised. | All feature/fix PRs land here. |

What this means in practice:

- A user who wants the most recent **stable** SDK clones `main` (or a `vX.Y.Z` tag).
- A user who wants the latest **work in progress** uses `develop`.
- Tags `vX.Y.Z` are cut **only from `main`** — `release.yml` rejects a tag whose
  commit is not on `main`.
- `main` is **never** force-pushed. If a release ships a regression, fix it
  forward (a `vX.Y.Z+1` tag) rather than rewriting history.

### Pull Request Rules

#### Target branch

All PRs must be submitted against the **`develop`** branch.
PRs targeting `main` are reserved for the release process (a single PR that
brings `develop`'s green tip into `main` when the maintainers cut a release).
Random PRs to `main` will be closed without review.

#### One topic per PR

Each PR must address **exactly one logical change**. Never mix:
- A bug fix with a refactor
- A new feature with a style cleanup
- A library change with an example change

If you need to do both, submit two PRs where the second depends on the first.

#### Keep PRs small

| Lines changed | Verdict |
|---------------|---------|
| **< 200** | Ideal. Gets reviewed fast. |
| **200–400** | Acceptable. Add a clear description. |
| **> 400** | Too large. Split it or explain why it can't be split. |

Asset files (`.pic`, `.pal`, `.map`) and generated assembly don't count
toward these limits — only human-written code.

**A 1000-line PR with no explanation will not be reviewed.** It's not
that we don't want your contribution — it's that we can't verify it's correct.

#### Commit messages

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types**: `feat`, `fix`, `perf`, `refactor`, `test`, `docs`, `chore`, `build`

**Scopes**: `lib`, `compiler`, `runtime`, `tools`, `examples`, `build`

#### Testing

Before submitting:

1. `make clean && make` — full rebuild must succeed
2. `make tests` — luna coverage + visual regression + probes must all pass
3. Test affected examples in [Mesen2](https://www.mesen.ca/)
4. New examples must include a working Makefile following the standard pattern

#### Self-review checklist

- [ ] `make clean && make` passes without warnings
- [ ] Commits follow Conventional Commits format
- [ ] No unrelated changes mixed in
- [ ] New code has Doxygen documentation (`/** @brief */`)
- [ ] Binary assets are from PVSnesLib or are original work

### Code Review

- All PRs require review before merge
- Expect 1–3 rounds of feedback — this is normal, not a rejection
- Maintainers aim to respond within 7 days
- **What reviewers check**: correctness, VBlank safety (VRAM writes only
  during blank), DMA budget, bank overflow risk, naming conventions

## AI-Assisted Contributions

We accept contributions that used AI tools (Copilot, Claude, ChatGPT, etc.)
under these conditions:

1. **You must understand every line you submit.** If asked during review
   to explain a design choice, "the AI suggested it" is not acceptable.
   AI is a tool — you are the author.

2. **Disclose AI usage in the PR description.** A simple note is enough:
   "Used Claude for initial boilerplate, manually reviewed and edited."

3. **Same quality bar.** AI-assisted code must pass all tests, follow code
   style, and handle SNES hardware constraints correctly (VBlank timing,
   DMA budgets, bank boundaries). No exceptions.

4. **Do NOT add `Co-Authored-By` trailers** for AI tools in commit messages.

### Project knowledge in `.claude/`

Two directories under `.claude/` capture knowledge that AI assistants and
human contributors share:

- [`.claude/rules/`](.claude/rules/) — **normative** rules (auto-loaded
  by Claude Code). Commit policy, debugging methodology, testing
  workflow, NMI audit checklist, etc. Treat these like project policy:
  follow them, change them only by PR with rationale.
- [`.claude/notes/`](.claude/notes/) — **observational** project knowledge.
  Lessons learned, technical references, active chantier handoffs, FIXED
  bugs preserved for context, structured project reviews. Read what's
  relevant; add to it when you learn something a future contributor (or
  your future self) will need.

When learning something project-specific, append to
`.claude/notes/<category>/`. The category README explains the layout.
Per-user cross-project preferences (your editor habits, etc.) belong in
your personal `~/.claude/projects/.../memory/`, not here. The convention
is documented in [`.claude/rules/memory_routing.md`](.claude/rules/memory_routing.md).

## Documentation

Every new feature or example must have:

1. **Doxygen comments** in source files (`/** @brief */` on functions,
   defines, and important variables)
2. **README** for new examples (with screenshot)
3. **Tutorial reference** if it covers a new SNES concept
   (see [docs/tutorials/](https://k0b3n4irb.github.io/opensnes/))

For code style conventions, see [Code Style Guide](https://k0b3n4irb.github.io/opensnes/code_style.html).

When using external code, add an entry to [ATTRIBUTION.md](ATTRIBUTION.md) with the
source, author, license, and description of modifications. Include a header
comment in the source file.

## Good First Issues

Look for issues labeled [`good first issue`](https://github.com/k0b3n4irb/opensnes/labels/good%20first%20issue).
These are small, self-contained tasks with clear instructions — designed
for your first contribution to the project.

## Links

- [Documentation](https://k0b3n4irb.github.io/opensnes/)
- [Open issues](https://github.com/k0b3n4irb/opensnes/issues)
- [Roadmap](ROADMAP.md)
- [Changelog](CHANGELOG.md)
