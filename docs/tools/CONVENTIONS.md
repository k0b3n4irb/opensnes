# The opensnes-* tools: conventions {#tools_conventions}

OpenSNES 1.x replaces the 0.x converters (`gfx4snes`, `smconv`, `wav2brr`,
`tmx2snes`, `font2snes`, `img2snes`, `palplan`, `aseprite2snes`,
`sa1_patch`) with a family of tools written for a studio shipping a game:
**one tool per function, the same conventions everywhere, settings that
live beside the asset, no interpreter to install**. This page is the
contract every tool in the family follows; a tool that does not is a bug.

The 0.x tools stay in the zip for one more release, each printing where
its function went.

## The family

| Tool | Function |
|------|----------|
| `opensnes` | the project: `init`, `build`, `run`, `test`, `doctor`, `budget`, `release` |
| `opensnes-sprite` | sprite sheet or Aseprite export → tiles, palette, metasprite table, animation clips |
| `opensnes-tileset` | PNG → tileset and tilemap, deduplicated, with per-tile palettes and flips |
| `opensnes-level` | Tiled (later LDtk) → map, objects, collision |
| `opensnes-text` | bitmap font → text tiles; string tables, per language |
| `opensnes-palette` | plan a project's palettes into CGRAM; quantize RGB art; preview |
| `opensnes-image` | full-screen images: Mode 7, HiColor, pseudo-hires |
| `opensnes-sample` | WAV → BRR |
| `opensnes-music` | Impulse Tracker → soundbank, with the samples it shares |
| `opensnes-rom` | finalize and check a ROM: header, size, mapper, checksum, the post-link checks, the credits |
| `opensnes-save` | read, write and verify a save file |

luna is not in the family: it is the emulator, debugger and test runner
the family calls (`opensnes run`, `opensnes test`), documented on its own
page (@ref tools_luna).

## Invocation

```
opensnes-<tool> <subcommand> [options] <input>…
opensnes-<tool> --help | --version
opensnes-<tool> <subcommand> --help
```

- **Subcommands are nouns or plain verbs**, one per thing the tool does:
  `opensnes-sprite sheet`, `opensnes-sprite aseprite`, `opensnes-music
  bank`, `opensnes-rom check`. Every tool has `inspect`, which prints what
  a file holds and what it costs (bytes, VRAM words, CGRAM entries, ARAM)
  without converting anything.
- **Options are long** (`--size 32`, `--bpp 4`, `--out build/`), with a
  short alias only for the three or four used on every call (`-o`, `-q`,
  `-v`). An option never changes meaning between tools: `--out` is always
  the output directory, `--bpp` always the bit depth, `--palette` always
  a `.pal` or a palette name.
- **`--help` says everything**: each option with its default and its
  range, one example per subcommand. No usage line that sends you to a
  page.
- **`--json`** writes the result as one JSON object on stdout: the
  outputs produced (path, size, what it is), the measurements `inspect`
  would give, the warnings. It is how the build system and other tools
  read a tool. Without `--json`, stdout carries only the result a human
  asked for, and progress goes nowhere unless `-v`.
- **Exit codes**: 0 done; 1 the input was refused (the message names the
  limit); 2 usage (unknown option, missing input); 3 the file system
  (cannot read or write). A refused input writes nothing.

## Messages

Every message goes to stderr in one shape:

```
opensnes-sprite: res/hero.png: 2 palette banks in tile (3,1) — one 16-colour palette per 8x8 tile; see --palette
```

The tool, the file (and the tile, row, sample or frame when there is one),
what is wrong, then what to do. A warning starts with `warning:` after the
file. Nothing is said twice, nothing is said in uppercase.

## Settings beside the asset

The import settings of an asset live in a TOML file next to it, named
after it: `res/hero.png` → `res/hero.png.toml`. The tool writes it the
first time it runs with `--save` (recording the effective settings), reads
it on every later run, and command-line options override it for that run.
The artist sees the settings in their folder, under version control, and
never opens the Makefile.

```toml
tool = "opensnes-sprite"
[sprite]
size = 32          # 8, 16, 32 or 64
bpp = 4
palette = "shared" # a .pal, a palette name from opensnes-palette, or "own"
clips = "tags"     # Aseprite tags become animation clips
```

**A composed asset** — a soundbank made of several modules, a string
table, a project palette, the ROM header — is not made from one source, so
its settings file is named after **what it produces** and lists its
sources: `music/soundbank.toml` with `tool = "opensnes-music"` and
`inputs = ["theme.it", "jingle.it"]` under `[bank]`, paths relative to the
file. `opensnes-music bank music/soundbank.toml` then builds it beside the
file, and `--save` on a command-line run writes that file for you. One
rule, two spellings: an asset with one source is named after its source
(`hero.png.toml`), an asset with several is named after its product
(`soundbank.toml`).

The first key is always `tool`: the build system's one generic rule reads
it to know which tool converts the file. **That rule exists** (`make/common.mk`,
`ASSET_TOML`): every `*.toml` and `res/*.toml` that names an `opensnes-*`
tool is converted before the first C or ASM object (the tool's subcommand
is the file's one table), and every `<stem>_data.as` the tools write is
included by `assets_gen.asm` and assembled with the project. A fragment
carries one `ASSET_SECTION` per blob (tiles, map, palette, sample), so the
linker places each where it fits; a blob above 32 KB, the size of a bank,
is cut in parts (`<name>_tiles`, `<name>_tiles_1`, …, each with `_end`). A project with settings files has no
hand-written `data.asm` and no conversion rule in its Makefile: the
`starter/` is built this way. The rest is one table named after
the subcommand, keys spelled exactly as the long options (`--size` is
`size`). Unknown keys are refused (exit 1) so a typo cannot silently fall
back to a default.

## Outputs

- **Deterministic**: the same input and settings give the same bytes, on
  every OS. That is what makes the golden suites meaningful and the build
  reproducible. No timestamps, no paths, no host names inside an output.
- **Named after the asset**, in `--out` (default: beside the input):
  `hero.png` gives `hero.tiles`, `hero.pal`, `hero.meta`, `hero.clips`,
  and a `hero.h` declaring every symbol with its size. The build includes
  the `.h`; the game never hard-codes a size.
- **Listed in `--json`** so a build can depend on exactly what was
  produced.

## Inputs

The formats the creator's tools write, as they write them: indexed or RGB
PNG, Aseprite's JSON + sheet export, Tiled's `.tmj`, PCM WAV, Impulse
Tracker `.it`. A tool quantizes or converts on the way in when the
settings say so (`opensnes-palette quantize` is the explicit step for art
that must be reviewed), and refuses, with the limit named, what the SNES
cannot hold rather than silently masking it.

## Implementation rules

For contributors (the rule itself is `.claude/rules/two_audiences.md`):

- C11, one binary, statically linked where the platform allows, built by
  `tools/tool.mk`; no runtime, no Python, no script on the user's path.
- The command-line, help, JSON and message code is shared
  (`tools/third_party` holds vendored parsers, `tools/common` the family's
  own `cli.c`), so the conventions above are enforced by the code, not by
  review.
- Every tool ships its golden suite (`tests/run_golden.py` over
  `tools/tests/golden.py`), its page under `docs/tools/`, and its fuzz
  harness for every parser it feeds with a user file.
- A tool that absorbs a 0.x tool reproduces that tool's golden suite byte
  for byte before the old one is retired.
