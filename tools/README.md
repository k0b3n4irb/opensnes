# tools/ — the compiled asset tools (shipped) and the SDK's test harness (not shipped)

Two families live here today; the review of 2026-10-05
(`.claude/notes/reviews/2026-10-05_tools_devtools_refactoring.md`) and the
rule `.claude/rules/two_audiences.md` set the direction: what the **game
developer** gets is compiled, one tool per function, with no Python; what
the **contributor** uses stays in the repository and never enters the zip.

## Shipped: the asset tools

Built by `make tools`, installed in `bin/`, copied into every release zip.
`make/common.mk` calls them on a user's `make`; each one prints `--help`,
has a page under `docs/tools/`, and a golden-output suite under `tests/`
(`make test-tools` runs the eight suites; `tools/fuzz/` fuzzes their
parsers).

| Path | Role | Version | In your build | Doc |
|------|------|---------|---------------|-----|
| [`gfx4snes/`](gfx4snes/) | indexed PNG/BMP → tiles, palettes, tilemaps, metasprite tables (2/4/8 bpp, LZ77, dedup) | 2.0.0 | automatic via `GFXSRC` | `docs/tools/gfx4snes.md` |
| [`smconv/`](smconv/) | Impulse Tracker `.it` → SNESMOD soundbank for the SPC700 | 2.1.0 | automatic via `USE_SNESMOD` | `docs/tools/smconv.md` |
| [`wav2brr/`](wav2brr/) | PCM `.wav` → `.brr` sample (one-shot or looping) | 1.0.0 | automatic for `res/*.wav` | `docs/tools/wav2brr.md` |
| [`tmx2snes/`](tmx2snes/) | Tiled `.tmj` map → tilemap, collision, objects | 1.0.1 | one line in the project Makefile | `docs/tools/tmx2snes.md` |
| [`aseprite2snes/`](aseprite2snes/) | Aseprite JSON export → `AnimClip` tables, one per tag | 1.0.0 | one line per sprite | `docs/tools/aseprite2snes.md` |
| [`font2snes/`](font2snes/) | 96-glyph font PNG → 2/4 bpp text tiles | 1.0.0 | by hand, once | `docs/tools/font2snes.md` |
| [`img2snes/`](img2snes/) | RGB/RGBA PNG → indexed PNG (quantize, BGR555 rounding, scale) | 1.0.0 | by hand, before committing art | `docs/tools/img2snes.md` |
| [`palplan/`](palplan/) | plans a project's `.pal` files into the 8 + 8 CGRAM slots, emits a C header | 1.0.0 | by hand, project-level | `docs/tools/palplan.md` |
| [`sa1-patch/`](sa1-patch/) | post-link: sets the SA-1 map-mode bits in the ROM header | 1.0.0 | automatic for `USE_SA1=1` | [`sa1-patch/README.md`](sa1-patch/README.md) |

Every tool's Makefile is a few variables over [`tool.mk`](tool.mk) (one
build recipe, one version macro `TOOL_VERSION`); the vendored parsers
(lodepng, cmdparser, stb_image, cute_tiled) live in
[`third_party/`](third_party/) with their licences (`ATTRIBUTION.md`).

These are the 0.x tools. In 1.x they are superseded by the `opensnes-*`
family (one tool per function, common conventions, TOML settings beside
each asset — see the rule); each new tool must reproduce the golden suite
of the tool it absorbs before the old one is retired.

## Not shipped: what else lives here

| Path | Role | Run by |
|------|------|--------|
| [`fuzz/`](fuzz/) | libFuzzer harnesses for the asset tools' parsers | `make fuzz`, `make fuzz-replay`, `fuzz.yml` |
| `valgrind-static.supp` | suppressions for valgrind on the static binaries (not wired anywhere) | by hand |

The luna-backed test harness moved to [`testing/`](../testing/) on
2026-10-05 (lot 5 of the review) so that `tools/` means "shipped" again;
only the files a user project's `make test` imports (`project_test.py`, `luna_runner.py`, `lib/`, `luna.version`) leave `testing/`
for the zip, and that path moves to native `luna test` under the
two-audiences rule.

## See also

- [`devtools/README.md`](../devtools/README.md) — the contributor-only
  scripts: sentinels, lints, fixtures, benches.
- [`make/common.mk`](../make/common.mk) — how a project Makefile reaches
  the tools.
- `docs/tools/README.md` — the user-facing guide to the pipeline.
