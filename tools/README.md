# tools/ — the SDK's tools

Each tool here does one job no other does. They are built by `make tools`,
installed in `bin/`, and shipped in every release zip. A game developer
rarely types their names: a settings file beside each asset
(`res/hero.png.toml`, `tool = "opensnes-sprite"`) tells the build which one
converts it. The user guide is `docs/tools/README.md`.

## The tools

| Tool | What it does | Doc |
|------|--------------|-----|
| [`opensnes/`](opensnes/) | the project: create, build, run, test, measure and release a game | `docs/tools/opensnes.md` |
| [`opensnes-sprite/`](opensnes-sprite/) | sprite sheets and Aseprite exports → tiles, palette, metasprites, animation clips | `docs/tools/opensnes-sprite.md` |
| [`opensnes-tileset/`](opensnes-tileset/) | background pictures → tileset, tilemap and palette | `docs/tools/opensnes-tileset.md` |
| [`opensnes-level/`](opensnes-level/) | Tiled levels → map, tile attributes, objects for the map engine | `docs/tools/opensnes-level.md` |
| [`opensnes-text/`](opensnes-text/) | bitmap fonts → tiles for the text module | `docs/tools/opensnes-text.md` |
| [`opensnes-palette/`](opensnes-palette/) | a project's palettes: the CGRAM plan, the quantization of RGB art | `docs/tools/opensnes-palette.md` |
| [`opensnes-image/`](opensnes-image/) | HiColor pictures and the Mode 7 perspective tables | `docs/tools/opensnes-image.md` |
| [`opensnes-sample/`](opensnes-sample/) | WAV → BRR samples for the DSP | `docs/tools/opensnes-sample.md` |
| [`opensnes-music/`](opensnes-music/) | Impulse Tracker modules → SNESMOD soundbank (the build runs it for `SOUNDBANK_SRC`) | `docs/tools/opensnes-music.md` |
| [`opensnes-rom/`](opensnes-rom/) | the post-link checks of every build, the cartridge header and checksum, the budgets | `docs/tools/opensnes-rom.md` |
| [`opensnes-save/`](opensnes-save/) | battery save files (`.srm`): create, read, patch, compare | `docs/tools/opensnes-save.md` |
| [`sa1-patch/`](sa1-patch/) | sets the SA-1 map-mode bits of the ROM header (the build runs it for `USE_SA1`) | [`sa1-patch/README.md`](sa1-patch/README.md) |

## What else is here

| Path | Role |
|------|------|
| [`common/`](common/) | linked into every `opensnes-*` tool: subcommands, long options, `--help`, `--json`, messages, exit codes, the TOML settings file (`cli.c`), the `.inc` / `_data.as` glue (`incfile.c`) |
| [`lib/`](lib/) | the converters of the earlier tools (gfx4snes and smconv from PVSnesLib, and six of the SDK's own), linked into the tools above, with the goldens their suites compare against. Their binaries are retired (`docs/tools/legacy.md`) |
| [`third_party/`](third_party/) | vendored lodepng, cmdparser, stb_image and cute_tiled, with their licences (`ATTRIBUTION.md`) |
| [`tests/`](tests/) | `golden.py`, the harness behind every `<tool>/tests/run_golden.py`; `make test-tools` runs them all |
| [`fuzz/`](fuzz/) | libFuzzer harnesses for the parsers (`make fuzz`, `make fuzz-replay`, `fuzz.yml`); not shipped |
| [`tool.mk`](tool.mk) | the one build recipe every tool's Makefile includes |

## Adding or changing a tool

The conventions every tool follows (subcommands, options, settings beside
the asset, `inspect`, `--json`, exit codes) are in
`docs/tools/CONVENTIONS.md`. A tool that absorbs another reproduces its
golden suite byte for byte before the old one is retired
(`.claude/rules/two_audiences.md`).
