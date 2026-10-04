# GitHub credentials: two tokens in `.env`, and what each is for

State checked on 2026-10-03. `.env` at the repo root is git-ignored, is not
loaded by the ambient shell, and must never be printed or committed.

## What `.env` holds

| Variable | Kind | Use it for | Cannot do |
|---|---|---|---|
| `GH_PAT_TOKEN` | fine-grained (`github_pat_…`), owner `k0b3n4irb` | the `gh` API on the owner's repositories (runs, releases, PRs, reading any public repository), HTTPS push to `k0b3n4irb/opensnes` | write anywhere the owner does not own: creating an issue on another account's repository answers 403 "Resource not accessible by personal access token", whatever boxes are ticked |
| `GH_GHP_TOKEN` | classic (`ghp_…`), owner `k0b3n4irb` | writing to a repository of another account (the five issues on `mukunda-/snesmod`, 2026-10-03) | - |

**There is no `GH_TOKEN` in `.env` any more** (renamed on 2026-10-03). `gh`
only reads `GH_TOKEN`, so set it per command, in the same command (the shell
does not persist between tool calls):

```sh
set -a; . ./.env; set +a
GH_TOKEN="$GH_PAT_TOKEN" gh run list --branch develop
```

Without it `gh` falls back to a stale keyring entry and answers
`Bad credentials` / "The token in keyring is invalid": a false alarm, not a
broken `gh`.

## The classic token is over-scoped: use it for nothing else

On 2026-10-03 `GH_GHP_TOKEN` carried every scope (`repo`, `delete_repo`,
`admin:org`, `workflow`, …) where `public_repo` was enough. The owner was
advised to revoke it and, if one is kept for answering upstream issues, to
recreate it with `public_repo` only. Until then:

- use it **only** for a write to another account's repository that the owner
  asked for, one command at a time (`GH_TOKEN="$GH_GHP_TOKEN" gh api …`), never
  exported for a whole command line that also does other things;
- never for push, releases or anything `GH_PAT_TOKEN` or SSH can do;
- if it is gone from `.env`, that is expected: ask the owner, do not look for
  another way.

## What needs no token at all

- **`scripts/install-luna.sh`**: luna is a public repository and the script
  downloads with `curl` first (HTTP 200 with no credential, checked
  2026-10-03). An older version of this note said the release was private.
- **`git push` / `fetch`**: `origin` is SSH (`git@github.com:k0b3n4irb/opensnes.git`)
  and the machine's key is accepted (`ssh -T git@github.com` greets
  `k0b3n4irb`). The HTTPS form is a fallback for a sandbox without SSH:

  ```sh
  set -a; . ./.env; set +a
  git -c credential.helper='!f(){ echo username=x-access-token; echo "password=$GH_PAT_TOKEN"; }; f' \
      push -q https://github.com/k0b3n4irb/opensnes.git develop:develop
  ```

  It needs the token's "Contents: Read and write" permission on `opensnes`
  (a token without it answers 403 on push while the API still reports
  `permissions.push = true`, which describes the user, not the token; test
  with `git push --dry-run`).

## Hygiene

- Redact tokens in any output:
  `sed -E 's/(gh[pous]_|github_pat_)[A-Za-z0-9_]+/\1***/g'`.
- To inspect `.env`, print variable names, kinds and lengths, never values.
- Everything posted with these tokens appears under the owner's name: an
  outward-facing write needs the owner's go, and the text is read back after
  posting.

## History

- 2026-06-22: `gh auth status` flagged the keyring during the v0.21.3 release
  and `gh` was wrongly thought broken.
- 2026-07-04: three dead ends (two `gh` 401s, one luna install) before
  `.env` was sourced.
- 2026-10-03: `GH_TOKEN` split into the two variables above when the
  fine-grained token could not file issues upstream.
