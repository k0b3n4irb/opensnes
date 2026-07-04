# GitHub credentials: source `.env` before gh / install-luna

The repo root has a git-ignored `.env` carrying `GH_TOKEN`. The ambient
shell does NOT load it, so without sourcing it:

- `gh run list` / `gh run view` / `gh api` fail with `HTTP 401: Bad credentials`
- `scripts/install-luna.sh` fails to fetch the pinned luna binary
  (private release on `k0b3n4irb/luna`)

Pattern:

```sh
set -a && source .env && set +a && gh run list --branch develop
```

`git push`/`pull` are unaffected (SSH key auth). A previously installed
`tools/luna-test/bin/luna` keeps working offline — only the download
needs the token.

Cost of not knowing this: a 2026-07-04 session burned three dead ends
(luna install 401, two gh 401s) before the user pointed at `.env`.
