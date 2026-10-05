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
written as a table of cases over [`tests/golden.py`](tests/golden.py)
(`make test-tools` runs every suite; `tools/fuzz/` fuzzes their
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
| [`opensnes-sample/`](opensnes-sample/) | **the first of the 1.x family** (`docs/tools/CONVENTIONS.md`): WAV → BRR with `encode` and `inspect`, settings beside the asset, `--json`; same bytes as wav2brr, whose golden suite it reproduces | 1.0.0 | by hand today; the build's generic rule comes with the family | `docs/tools/opensnes-sample.md` |
| [`opensnes-music/`](opensnes-music/) | 1.x family: Impulse Tracker → SNESMOD soundbank with `bank`, `spc` and `inspect` (SPC RAM per module); same bytes as smconv, whose golden it reproduces | 1.0.0 | by hand today | `docs/tools/opensnes-music.md` |
| [`opensnes-rom/`](opensnes-rom/) | 1.x family: `check` runs the post-link checks of a user build (bank $00 ratchet, C RAM band, data-init sentinel, bank-blind reads, NMI / WRAM-port race, asset inventory) — the compiled successor of five Python scripts, same verdicts on the 99 built ROMs | 1.0.0 | `make/common.mk` after every link | `docs/tools/opensnes-rom.md` |
| [`opensnes-sprite/`](opensnes-sprite/) | 1.x family: `sheet` (sprite sheet → tiles in OBJ order, palette, metasprite table), `anim` (Aseprite export → AnimClip header), `inspect`; gfx4snes's and aseprite2snes's converters linked as libraries, their goldens reproduced | 1.0.0 | by hand today | `docs/tools/opensnes-sprite.md` |
| [`opensnes-tileset/`](opensnes-tileset/) | 1.x family: `convert` (picture → tileset, tilemap, palette; Modes 1/5/6/7, pages, flips, palette rearrangement) and `inspect`; gfx4snes's map path linked as a library, its golden and pixel oracle reproduced | 1.0.0 | by hand today | `docs/tools/opensnes-tileset.md` |
| [`opensnes-level/`](opensnes-level/) | 1.x family: `convert` (Tiled JSON level + the tileset's `.map` → `<layer>.m16`, `.b16`, `.t16`, `.o16`, optional `.q16`, `.c16`, entities header, and the `.inc` / `_data.as` glue), `inspect`; tmx2snes's converter linked as a library (`tools/tmx2snes/src/level.c`) | `tests/run_golden.py` (data byte-identical to tmx2snes's goldens) |
| [`opensnes-text/`](opensnes-text/) | 1.x family: `font` (a 96-glyph picture → tiles in glyph order, a grey palette, the `.inc` / `_data.as` glue; indexed, grey or RGB sources), `inspect`; font2snes's tile packer linked as a library, its tiles reproduced byte for byte | 1.0.0 | by the build's generic rule | `docs/tools/opensnes-text.md` |
| [`opensnes-palette/`](opensnes-palette/) | 1.x family: `plan` (a project's `.pal` files → CGRAM slots, `PAL_<NAME>_CGRAM` / `_SLOT` macros, an optional 512-byte CGRAM image; a composed asset), `quantize` (RGB art → indexed PNG), `inspect`; palplan's planner and img2snes's quantizer linked as libraries, their goldens reproduced | 1.0.0 | `plan` by the build's generic rule, after the pictures; `quantize` by hand | `docs/tools/opensnes-palette.md` |
| [`opensnes-image/`](opensnes-image/) | 1.x family: `hicolor` (a 256x224 picture → 896 sequential 4 bpp tiles + 112 segment palettes for the per-scanline CGRAM stream), `perspective` (the Mode 7 perspective-rotation HDMA tables, krom's bytes), `inspect`; the compiled successor of `hicolor64.py` and `m7ptables.py` | 1.0.0 | by the build's generic rule | `docs/tools/opensnes-image.md` |
| [`opensnes-save/`](opensnes-save/) | 1.x family: battery save files (`.srm`) — `new` (the blank save a ROM's header declares: `$FFD8`, or the Super FX expansion RAM of `$FFBD`), `inspect`, `get`, `set`, `diff`; a prepared save for a luna manifest's `srm_in`, a reading of its `srm_out` | 1.0.0 | by hand | `docs/tools/opensnes-save.md` |

Every tool's Makefile is a few variables over [`tool.mk`](tool.mk) (one
build recipe, one version macro `TOOL_VERSION`); the 1.x tools share
[`common/cli.c`](common/cli.c) (subcommands, long options, `--help`,
`--json`, messages, exit codes, the TOML settings file); the vendored parsers
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
only `luna.version` leaves `testing/` (a user project's `make test` is `luna test` on the project's own manifests, no runner)
for the zip, and that path moves to native `luna test` under the
two-audiences rule.

## See also

- [`devtools/README.md`](../devtools/README.md) — the contributor-only
  scripts: sentinels, lints, fixtures, benches.
- [`make/common.mk`](../make/common.mk) — how a project Makefile reaches
  the tools.
- `docs/tools/README.md` — the user-facing guide to the pipeline.
