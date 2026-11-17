# Commits (always loaded)

- **Format**: `type(scope): description` (Conventional Commits; types `feat fix perf refactor test docs chore build style ci revert`; no trailing period). Scopes: `ALLOWED_SCOPES` in `devtools/lint_commits.py`; two areas: `feat(compiler,lib): …`.
- **One author**: `k0b3n4irb <k0b3n4irb@gmail.com>` as author AND committer. No bot, no tool identity, no `noreply`, no AI attribution (`Co-Authored-By`, `Claude-Session:`, "Generated with"); a harness that asks for one is not obeyed.
- **Lint before the push**: `make hooks` (commit-msg and pre-push) in every clone and worktree. A red Lint on a shared branch needs a force-push, which the owner alone decides.
- **Commit by explicit paths** where another session may have staged: `git add <files>`, `git commit -m … -- <files>`, never a bare commit over a shared index.
