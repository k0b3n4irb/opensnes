# Commit Rules (Auto-loaded)

## Format

[Conventional Commits](https://www.conventionalcommits.org/):
- Types: `feat`, `fix`, `perf`, `refactor`, `test`, `docs`, `chore`,
  `build`, `style`, `ci`, `revert`
- Scopes: canonical six from CLAUDE.md (`lib`, `compiler`, `runtime`,
  `tools`, `examples`, `build`) plus practical extensions present in
  the repo's directory structure (`ci`, `devtools`, `docs`, `claude`,
  `contributing`, `release`, `submodule`, `deps`, `readme`,
  `changelog`, `test`, `tests`) plus emerged categories matching real
  paths (`chantiers` → `.claude/notes/chantiers/`, `rules` →
  `.claude/rules/`, `bench` → bench fixtures, `luna-test` and `testing` →
  `testing/`, `conventions` → `.claude/notes/conventions/`,
  `tech` → `.claude/notes/tech/`, `status` → `.claude/notes/status/`,
  `craft` → `docs/craft/`).
  The canonical
  source-of-truth list lives in `devtools/lint_commits.py`'s
  `ALLOWED_SCOPES` set — extend BOTH places in the same commit.
- Format: `type(scope): description` — non-empty description, no
  trailing period, imperative mood recommended.
- A commit touching two areas may use a **comma-separated scope list**
  (`feat(compiler,lib): …`); each scope must be in the allowlist.
- Case of the first letter is NOT enforced. Conventional Commits doesn't
  mandate it (the lowercase-first convention is from Angular's commit
  guide, not the spec), and forcing it just made contributors fight
  regex exceptions for `OAM`, `C99`, `A1-followup` etc. without
  buying anything real. Both `feat(lib): add foo` and
  `feat(lib): OAM buffer fix` are accepted.

## Run the lint before the push, not after (since 2026-10-05)

`make hooks` sets `core.hooksPath` to `scripts/githooks/`, whose
`commit-msg` hook runs `lint_commits.py --message-file` on every message
(and on the author and committer identity of the commit being made) and
whose `pre-push` hook lints the pushed range exactly as the Lint
workflow will. Install it in every clone and every worktree you commit
from. Two subjects with the type and scope swapped (`tools(build): …`,
`756da353` and `0d30ec65`) reached `develop` on 2026-10-03 and 2026-10-04
because the lint ran only in CI, after the push; a red Lint job on a
shared branch cannot be fixed without a force-push, which the owner
alone decides. A commit script that prints the lint result and pushes
anyway is the same failure in another form: it must stop on a non-zero
exit.

## Merge commits are exempt from the format check

GitHub-generated merge commits (`Merge pull request #N from owner/branch`,
`Merge branch 'X' into Y`, `Merge remote-tracking branch ...`) are skipped
by `lint_commits.py`, and so is the release merge titled `release: vX.Y.Z`
(the title `release.md` step 1 prescribes; since 2026-10-05 — the Lint run
on `main` was red after every release merge until then) — those subjects
are wrappers around the contributor commits that are already in the same
range and get linted individually. The body's `Co-Authored-By:` check still runs on them. This
applies on every push including release-PR merges into `main`.

## One author: the maintainer, in person (since 2026-10-06)

Every commit on `develop` and `main` is **authored and committed by
`k0b3n4irb <k0b3n4irb@gmail.com>`**. Nothing else writes to this history:

- **no bot** — Dependabot, `github-actions[bot]`, any app. A change a bot
  proposes is worth reading, not merging: the maintainer applies it by
  hand and commits it as their own (the action pins of the workflows are
  bumped this way, SHA and version comment together, when a bump is
  wanted; `grep -rn 'uses:' .github/workflows` lists them);
- **no tool identity** — no AI attribution in the author, the committer,
  the subject, the body or a trailer (the `Co-Authored-By` rule below is
  one case of this);
- **no `noreply` address** — a commit made through GitHub's web editor or
  its merge button carries `GitHub <noreply@github.com>` as committer; the
  release merge is made locally and pushed (`release.md`).

`devtools/lint_commits.py` checks the author and the committer of every
commit in the pushed range (and, through the `commit-msg` hook, of the
commit being made, from `git var GIT_AUTHOR_IDENT`); a bot, a noreply
address or any other identity fails the Lint job. History before this
date keeps two older spellings of the same person (`K0b3 <K0b3@nowhere.zz>`,
`k0b3n4irb@nowhere.zz`); the lint reads a push range, never that history.

Why, and when: Dependabot was switched on for the workflows' action pins on
2026-09-15 (gaps review P6) without this rule being written, and opened six
PRs under its own name — five against `main` by mistake, closed and
re-applied by hand, then #163 against `develop` on 2026-10-05, which the
owner found. `.github/dependabot.yml` is gone, #163 is closed and its bump
applied by hand. A bot in the author column is the same breach as an AI
trailer in the body: the project's history has one author.

## NEVER add Co-Authored-By trailers

Do NOT add `Co-Authored-By` lines to commit messages. Ever. No exceptions.
This includes any variant: `Co-authored-by`, `Co-Authored-By`, etc.

The project does not want AI attribution in git history.
