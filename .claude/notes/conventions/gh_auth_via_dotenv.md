# `gh` CLI auth lives in `.env`, not the keyring

In this repo the GitHub CLI (`gh`) is authenticated via a **`GH_TOKEN` exported
from `.env`** at the repo root — *not* via the system keyring.

## Symptom if you forget

`gh auth status` reports **"The token in keyring is invalid"** and `gh api user`
returns **`Bad credentials`**, even when auth is perfectly fine. This is a false
alarm: the keyring entry is stale/unused; the working credential is the
`GH_TOKEN` in `.env`. Do **not** conclude `gh` is broken from `gh auth status`
alone — test after sourcing `.env`.

## How to use `gh` (and any tool needing the token)

The shell does **not** persist between tool calls, so source `.env` in the *same*
command as the `gh` invocation:

```sh
set -a; . ./.env; set +a
gh pr create ... / gh run list ... / gh release view ...
```

## Notes

- `.env` is **gitignored** (verified: `git check-ignore .env` → ignored). It is a
  local, per-machine secret store — never commit it, never print its contents.
- Always redact any token in command output:
  `sed -E 's/(gh[pous]_|github_pat_)[A-Za-z0-9_]+/\1***/g'`.
- Plain **git over SSH** (push/fetch/tag) does **not** need this — the submodule
  and superproject remotes are SSH (`git@github.com:...`) and use the SSH key.
  Only the `gh` *API* surface (PRs, runs, releases) needs `GH_TOKEN`.
- Discovered 2026-06-22 during the v0.21.3 release, after wrongly assuming `gh`
  was broken because `gh auth status` flagged the keyring.
