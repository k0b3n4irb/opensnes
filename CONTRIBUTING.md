# Contributing to OpenSNES {#contributing}

Thank you for your interest in contributing to OpenSNES!

## Code of Conduct

Be respectful, constructive, and welcoming to newcomers.
SNES development is niche — every contributor matters.

## Reporting Bugs

1. Search [existing issues](https://github.com/k0b3n4irb/opensnes/issues) first
2. Use the **Bug Report** template — it asks the right questions
3. Include a **minimal reproduction** (a small `main.c` that triggers the bug)
4. Attach screenshots or a short video if it's a visual issue
5. Specify the emulator used (Mesen2, snes9x, bsnes)

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
git checkout -b feature/my-feature
```

### Pull Request Rules

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
2. `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick` — 212 checks must pass
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
