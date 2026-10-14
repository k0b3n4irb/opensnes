# Legacy tools {#tools_legacy}

The SDK's `bin/` still holds the converters a project written before the
`opensnes-*` family uses: `gfx4snes`, `tmx2snes`, `img2snes`, `font2snes`,
`wav2brr`, `palplan` and `aseprite2snes`. They work as they always did, and
the build still runs `gfx4snes` for a `GFXSRC` list and `wav2brr` for a
`.wav` whose `.brr` is `.incbin`'d. This page is for someone holding such a
project: what each tool did, and the `opensnes-*` command that replaces it,
option by option. A new project starts from @ref tools.

Moving an asset over is one settings file beside it (`res/hero.png.toml`,
with a `tool` line and one table, @ref tools_conventions): the build then
converts it on its own, and the old Makefile line goes. Each new tool's page
carries the same table, with the outputs it writes.

`smconv` is not on this page: the build still drives it through
`USE_SNESMOD` and `SOUNDBANK_SRC`, and it keeps its own page, @ref tools_smconv.

# gfx4snes {#tools_gfx4snes}

Converts an indexed PNG or BMP into tiles (`.pic`), a palette (`.pal`) and,
with `-m`, a tilemap (`.map`); with `-T` it also writes a metasprite table.
A background is now `opensnes-tileset convert` (@ref tools_opensnes_tileset),
a sprite sheet `opensnes-sprite sheet` (@ref tools_opensnes_sprite).

| gfx4snes | opensnes-tileset convert / opensnes-sprite sheet |
|---|---|
| `-i FILE` | the input, positional |
| `-m` (a background) | `opensnes-tileset convert` writes the map |
| no `-m` (a sprite sheet) | `opensnes-sprite sheet` |
| `-s N` | `--size N` (tileset 8 or 16, sprite 8 to 64) |
| `-u 4` / `-u 16` / `-u 256` | `--bpp 2` / `--bpp 4` / `--bpp 8` |
| `-o N` | `--colors N` |
| `-p` | the palette is written by default; `--no-palette` skips it |
| `-c FILE` | `--palette FILE` |
| `-e N` | `--palette-entry N` |
| `-a` | `--rearrange` (tileset) |
| `-d` | `--round` |
| `-b`, `-k`, `-z` | `--blank`, `--pack`, `--lz` |
| `-f N`, `-g` | `--offset N`, `--priority` (tileset) |
| `-y`, `-R`, `-F` | `--pages`, `--no-reduce`, `--flip` (tileset); `-F` is `--flip` on a sheet too |
| `-M N` | `--mode N` (tileset; 7 writes the `.pc7` / `.mp7` pair) |
| `-T -X W -Y H` | `--metasprite W H` (sprite) |
| `-P N` | `--priority N` (sprite) |
| `-t png\|bmp` | no equivalent: the format is read from the file |
| `-W`, `-H`, `-S` | no equivalent |
| `-q` | `-q` |

In a Makefile, `GFXSRC := hero.png` becomes `res/hero.png.toml` with
`tool = "opensnes-sprite"` and a `[sheet]` table (or `opensnes-tileset` and
`[convert]` for a background).

# tmx2snes {#tools_tmx2snes}

Converts a Tiled map exported as JSON, against the `.map` of its tileset,
into the tilemap per layer (`.m16`), the tile tables (`.t16`, `.b16`) and
the objects (`.o16`). Now `opensnes-level convert` (@ref tools_opensnes_level).

| tmx2snes | opensnes-level convert |
|---|---|
| `map.tmj` (first argument) | the input, positional |
| `tileset.map` (second argument) | `--tileset tileset.map` |
| `-e` | `--entities` (writes `<stem>_entities.inc`) |
| `-Q` | `--quadrant` |
| `-C` | `--collision` |
| `-q` | `-q` |

The explicit `tmx2snes` line of a Makefile becomes `res/map.tmj.toml` with
`tool = "opensnes-level"` and a `[convert]` table naming the tileset; the
build converts the level after its tileset.

# img2snes {#tools_img2snes}

Quantizes RGB or RGBA art into an indexed PNG of a chosen number of
colours, the input the picture tools need. Now `opensnes-palette quantize`
(@ref tools_opensnes_palette).

| img2snes | opensnes-palette quantize |
|---|---|
| `-i FILE` | the input, positional |
| `-o FILE` | no equivalent for the name: `--out DIR` writes `<stem>_indexed.png` there |
| `-c N` | `--colors N` |
| `--round-snes` | `--round` |
| `-p ref.png` | `--palette ref.png` (or a `.pal`) |
| `-s FACTOR` | `--scale PERCENT` (`-s 2.0` is `--scale 200`) |
| `-t N` | `--align N` |
| `--batch` | several inputs on the command line |
| `-q`, `-v` | `-q`, `-v` |

# font2snes {#tools_font2snes}

Packs a picture of the 96 glyphs of ASCII 32..127 into 2 bpp or 4 bpp text
tiles. Now `opensnes-text font` (@ref tools_opensnes_text), which also writes
a grey palette and the `.inc` that declares the tiles.

| font2snes | opensnes-text font |
|---|---|
| `input.png output` | the input, positional; the outputs are named after it (`--out DIR` to place them) |
| `-b N` | `--bpp N` |
| `-c` (a C header) | no equivalent: the tiles are `.incbin` data declared by the `.inc` |
| `-v` | `-v` |

# wav2brr {#tools_wav2brr}

Encodes a PCM `.wav` into a `.brr` sample for the DSP. Now
`opensnes-sample encode` (@ref tools_opensnes_sample), which writes the same
bytes and a `.h` with the sample's sizes and loop point.

| wav2brr | opensnes-sample encode |
|---|---|
| `input.wav [output.brr]` | the input, positional; the output is named after it (`--out DIR` to place it) |
| `--loop START END` | `--loop START END` (`--save` keeps it in `input.wav.toml`) |
| `-v` | `-v`; `opensnes-sample inspect` gives the figures |

A looping sample no longer has to be built by hand: the loop lives in the
settings file, and the build encodes it.

# palplan {#tools_palplan}

Plans a project's `.pal` files into the 8 background and 8 sprite palette
slots of CGRAM and writes the `PAL_<NAME>_CGRAM` offsets. Now
`opensnes-palette plan` (@ref tools_opensnes_palette), which the build runs
after the pictures' conversions.

| palplan | opensnes-palette plan |
|---|---|
| a manifest of `name type file` lines | a settings file with a `[plan]` table: `bg = [...]`, `sprite = [...]`, in slot order; the name is the file's stem |
| `-o FILE` (the header) | `<stem>.inc`, beside the settings file or in `--out DIR` |
| `-b FILE` (the CGRAM image) | `--cgram` (`cgram = true`) |
| `-t N` | `--hints N` |
| `-q` | `-q` |

# aseprite2snes {#tools_aseprite2snes}

Turns the JSON Aseprite writes with `--data --list-tags` into `AnimClip`
tables, one per tag, for the `anim` module. Now `opensnes-sprite anim`
(@ref tools_opensnes_sprite).

| aseprite2snes | opensnes-sprite anim |
|---|---|
| `export.json` | the input, positional |
| `-o FILE` (stdout by default) | `<stem>_anim.h`, beside the input or in `--out DIR` |
| `-p PREFIX` | `--prefix NAME` |
| `-t STRIDE` | `--stride N` |
| `-f FPS` | `--fps N` |
