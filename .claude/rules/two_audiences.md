# Two audiences: game developers and contributors (Auto-loaded)

Owner decision, 2026-10-05 (`.claude/notes/reviews/2026-10-05_tools_devtools_refactoring.md`, §9 to §11).

OpenSNES has two populations, and they do not want the same thing.

| | The game developer | The contributor |
|---|---|---|
| Who | an indie studio or a solo developer making a game, possibly commercial, with this SDK | someone changing the SDK itself |
| Gets | the release zip: compiled tools in `bin/`, `lib/`, `make/`, `templates/`, `starter/`, luna | the repository |
| Installs | the zip, `make`, luna. **Nothing else: no Python, no uv, no runtime** | Python 3, uv, ortools, clang, whatever the work needs |
| Runs | `opensnes init / build / run / test / doctor`, the `opensnes-*` asset tools, luna | `make tests`, `make lint`, the sentinels, the fixtures, the audits |

## The rule

1. **Nothing a user project's build executes is an interpreted script.**
   Every step `make/common.mk` runs on a user's `make` is a binary from
   `bin/` (or luna). A Python script on that path is a defect, not a
   convenience. **Zero since 2026-10-06**: of the ten `python3` calls
   `common.mk` had on the morning of 2026-10-05, the post-link checks
   became `opensnes-rom check`, a project's `make test` became `luna test`
   on the project's own manifests, and the 0.x-name hint on a compile
   error became `opensnes upgrade` (the compiled CLI). The zip ships no
   Python at all, and `check_doc_drift.py` anchor 17 fails the lint if a
   call comes back.
2. **Tools for the game developer are compiled, one per function, under one
   prefix**: `opensnes` (the project), `opensnes-sprite`, `-tileset`,
   `-level`, `-text`, `-palette`, `-image`, `-sample`, `-music`, `-rom`,
   `-save`. Same conventions everywhere: named subcommands, long options,
   `--help`, `--json`, `inspect`, a TOML settings file beside each asset,
   same error style. They arrive in 1.x; the 0.x tools stay shipped for one
   version with a pointer. A fused tool must reproduce the absorbed tool's
   golden suite byte for byte before the old one goes.
3. **`devtools/` is contributor-only and never shipped.** Scripts that the
   user build still needs live there only until their compiled replacement
   proves the same verdict on the corpus. None is left on that path since
   2026-10-06, and the release recipe copies nothing from `devtools/`.
4. **The release zip is light**: no built examples, no Doxygen HTML; those
   ship as a separate examples archive and as the online docs. The judge is
   `devtools/release_smoke.py` — and, once the Python is gone, it runs in
   a container without `python3`.
5. **Bash is tolerated, Python is not.** `make` on Windows means MSYS2,
   which has bash; `bin/cc65816` and `install-luna.sh` may stay shell for
   now. `bin/opensnes` is a compiled program since 2026-10-06
   (`tools/opensnes`). Turning `cc65816` into a real binary is wanted, as
   its own lot.

## When writing a change, ask

- Does it add a step to a user build? Then it is a binary, or it waits.
- Does it add a tool for the game developer? Then it follows the prefix
  and the conventions, and it ships its golden tests.
- Does it add a contributor script? Then it goes under `devtools/`, is
  named by a `make` target or a workflow (no orphans), and is listed in
  `devtools/README.md`.

## Cross-references

- `.claude/rules/luna_tooling.md` — luna stays the single test backend;
  a user's `make test` is `luna test` on native manifests.
- `.claude/rules/release.md` — the release flow; the zip content is
  changing under lot 8 of the report.
- `.claude/rules/testing.md` — the contributor gate, unchanged.
