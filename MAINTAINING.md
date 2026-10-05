# Maintaining OpenSNES

What someone taking over the project needs and cannot guess from the
code: where each moving part lives, who owns it, and the procedure that
keeps it honest. Contributing a change is `CONTRIBUTING.md`; this page is
for whoever merges, releases and bumps.

## The repositories

| Repository | What it is | Upstream |
|---|---|---|
| `k0b3n4irb/opensnes` | this SDK | — |
| `k0b3n4irb/cproc` | C frontend fork (submodule `compiler/cproc`) | `https://git.sr.ht/~mcf/cproc` |
| `k0b3n4irb/qbe` | code generator fork with the 65816 backend (`compiler/qbe`) | `git://c9x.me/qbe.git` |
| `k0b3n4irb/wla-dx` | assembler/linker fork (`compiler/wla-dx`) | `https://github.com/vhelin/wla-dx.git` |
| `k0b3n4irb/luna` | the emulator the tests run on (a pinned release binary, not a submodule) | — |
| `jothepro/doxygen-awesome-css` | documentation theme (`docs/doxygen-awesome-css`) | — |

Merging and releasing need write access to `opensnes` and to the three
forks: a compiler fix is a commit on the fork's branch named in
`compiler/PINS.md`, pushed there, then a pin bump here.

## The pins

- **Compiler**: `compiler/PINS.md` holds the SHA of each fork and its
  count of local patches; `make verify-toolchain` (run by every build and
  by CI) fails when a submodule, or a count, does not match. The update
  procedure is at the bottom of that file. Every bump runs
  `make test-toolchain-suites` (the forks' own upstream suites, against
  known-fail ratchets) and the Class A proof of `.claude/rules/testing.md`.
- **luna**: `testing/luna.version`. `scripts/install-luna.sh`
  downloads that release for Linux, macOS or Windows and checks its
  SHA-256 (no token needed: the repository is public). A bump re-runs
  `make tests`, and any baseline it moves is explained in the commit.

## CI

Eight workflows in `.github/workflows/`. `opensnes_build.yml` (build on
four OS, functional tests on two Linux arches) and `lint.yml` gate every
push; `release.yml` runs on a `vX.Y.Z` tag on `main`; `fuzz.yml`,
`pal.yml` and `luna-bench.yml` are periodic; `deploy_docs.yml` publishes
the Doxygen site; `msys2_cproc_diagnostic.yml` is a Windows diagnostic.
The only secret they use is the automatic `GITHUB_TOKEN`: there is
nothing to hand over.

## Releases

`.claude/rules/release.md`: `develop` → release PR → `main` → tag on
`main`. `release.yml` refuses a tag that is not on `main`, builds the
four zips, builds and tests a project from each one (`make
release-smoke`, also runnable locally), and runs the corpus on luna on
the Linux legs before publishing.

## What is not in the repository

- **DSP-1 firmware** (`dsp1b.rom`) is copyrighted and never committed.
  With it in `~/.config/luna/firmware/`, the DSP-1 examples and fixture
  run; without it (CI) they are skipped, not failed.
- **The SNES reference corpus** (the `cartouche` MCP server, "snes-rag")
  arbitrates hardware claims (`.claude/rules/hardware_claims.md`). It is
  run outside this repository; building and testing do not need it, but
  writing a new hardware claim in the docs does. What it holds and the
  queries that check it: `.claude/notes/tech/cartouche_corpus.md`.
- **Exchanges with luna and snes-rag** are in the repository
  (`.claude/notes/partners/`): read the open lists before a luna bump.

## Where the decisions are

| Question | File |
|---|---|
| Why the API looks the way it does | `PHILOSOPHY.md` |
| What silently breaks, for users | `KNOWN_LIMITATIONS.md` |
| What is planned | `ROADMAP.md` |
| What is structurally wrong and how big it is | `.claude/STRUCTURAL_DEFECTS.md` |
| How to test a change of each kind | `.claude/rules/testing.md` |
| The latest state-of-the-project audit | `.claude/notes/reviews/2026-09-26_etat_des_lieux.md` |
