# The OpenSNES toolbox {#tools}

Your game is written in C, but the SNES does not eat PNGs, Tiled maps, or WAV
files — it eats tiles, tilemaps, BGR555 palettes, BRR samples, and SPC700
soundbanks. The `opensnes-*` tools are the converters that bridge the two: each
takes an artist- or musician-friendly source format and writes the exact binary
the hardware (and this SDK's runtime) expects. They are compiled programs in the
SDK's `bin/`; a game's build needs nothing else installed.

## Start from your asset

You rarely type a tool's name. Put the asset in `res/`, a settings file
beside it naming the tool, and `make` converts it; the generated `.inc`
(next to the asset) lists the symbols your C uses.

| I have… | Beside it I write… | In C I get… |
|---------|--------------------|-------------|
| a background picture `res/bg.png` | `res/bg.png.toml`: `tool = "opensnes-tileset"`, `[convert]` | tiles, tilemap and palette, and a `BgAsset` for `bgLoad()` |
| a sprite sheet `res/hero.png` | `res/hero.png.toml`: `tool = "opensnes-sprite"`, `[sheet]` with `size` | sprite tiles and palette (and metasprites with `metasprite`) |
| an Aseprite export `res/hero.json` | `res/hero.json.toml`: `tool = "opensnes-sprite"`, `[anim]` | one `AnimClip` per tag, for `animPlay()` |
| a Tiled level `res/level.tmj` | `res/level.tmj.toml`: `tool = "opensnes-level"`, `[convert]` with `tileset` | the map data `mapLoad()` reads |
| a font picture `res/font.png` | `res/font.png.toml`: `tool = "opensnes-text"`, `[font]` | text tiles for the text module |
| a sound `res/jump.wav` | `res/jump.wav.toml`: `tool = "opensnes-sample"`, `[encode]` | a BRR sample for `audioLoadSample()` |
| music `music/song.it` | no settings file: `USE_SNESMOD := 1` and `SOUNDBANK_SRC := music/song.it` in the Makefile | `soundbank.h`, one constant per module and effect |

The starter project (`opensnes init my-game --template game`) has one:
`res/player.png` and `res/player.png.toml`. Each tool's page lists its
settings, and its `--save` option writes the settings file from a command
you tried by hand.

## The tools at a glance

| Tool | Turns… | …into |
|------|--------|-------|
| @subpage tools_opensnes_tileset | an indexed picture of a background | tileset + tilemap + palette (Mode 7 included) |
| @subpage tools_opensnes_sprite | a sprite sheet; an Aseprite export | OBJ tiles + palette + metasprite table; `AnimClip` tables |
| @subpage tools_opensnes_level | a Tiled JSON map | tilemaps, tile tables, entities, collision |
| @subpage tools_opensnes_palette | the project's `.pal` files; RGB art | a CGRAM plan with named offsets; an indexed PNG |
| @subpage tools_opensnes_text | a picture of 96 glyphs | 2 bpp or 4 bpp text tiles |
| @subpage tools_opensnes_image | a full-screen picture; perspective settings | HiColor tiles and palettes; Mode 7 perspective tables |
| @subpage tools_opensnes_sample | a PCM `.wav` | a `.brr` sample + its sizes |
| @subpage tools_opensnes_music | Impulse Tracker `.it` modules | a SNESMOD soundbank; a standalone `.spc` |
| @subpage tools_opensnes_rom | a linked ROM | the post-link checks, its header, its budget |
| @subpage tools_opensnes_save | a ROM or a `.srm` | a battery save file, read or patched |
| @subpage tools_opensnes | your project | `init`, `build`, `clean`, `run`, `test`, `budget`, `release`, `doctor`, `upgrade` |

Every tool follows one contract — subcommands, long options, `--help`,
`--json`, `inspect`, the same exit codes: @subpage tools_conventions.

## What is in `bin/`

Two kinds of programs. The ones you type: `opensnes` (the project) and the
`opensnes-*` tools above, each with `--help`. The ones the build runs for
you and you never type: the C compiler (`cc65816`, which drives
`cproc-qbe` and `qbe`), the assemblers and linker (`wla-65816`,
`wla-spc700`, `wla-superfx`, `wlalink`) and `sa1_patch`.

## The pipeline

```
  RGB art ──► opensnes-palette quantize ──► indexed PNG
                                              │
              ┌───────────────────────────────┤
              ▼                               ▼
  opensnes-tileset convert          opensnes-sprite sheet
  (.pic + .map + .pal)              (.pic + .pal + metasprites)
              │                               │
              ▼                               │
  level.tmj ──► opensnes-level convert        │   hero.json ──► opensnes-sprite anim
  (tilemaps, tile tables, entities)           │                 (AnimClip tables)
                                              ▼
              every .pal ──► opensnes-palette plan ──► CGRAM slots (PAL_<NAME>_CGRAM)

  font.png  ──► opensnes-text font    ──► text tiles
  sound.wav ──► opensnes-sample encode ──► .brr sample
  music.it  ──► opensnes-music bank   ──► SNESMOD soundbank

  game.sfc  ──► opensnes-rom check    ──► the post-link checks, at every link
```

Each tool does one conversion and the build chains them; `inspect` on any of
them tells you what an asset costs (VRAM, CGRAM, ARAM) before you convert it.

## How the build runs them

An asset carries its settings in a TOML file beside it, named after it
(`res/hero.png` → `res/hero.png.toml`); an asset made of several sources is
named after what it produces (`res/palettes.toml`). The
file's `tool` line names the converter and its one table the subcommand:

```toml
tool = "opensnes-sprite"

[sheet]
size = 16
colors = 16
```

`make/common.mk` finds every `*.toml` and `res/*.toml` with a
`tool = "opensnes-…"` line (the list is `ASSET_TOML`, which a Makefile may set
by hand), runs `bin/<tool> <subcommand>` on the asset once (a `.done` stamp
beside the settings file), before any C or assembly is built, and again when the
asset or its settings change. Levels and palette plans convert after the rest,
so the tileset `.map` and the `.pal` files they read are fresh. The build then
gathers the `<stem>_data.as` fragment each tool wrote into `assets_gen.asm` and
assembles it with the project, so the data lands in the ROM's asset banks and C
sees it through the `.inc` the tool wrote. Nothing to add to the Makefile.

`--save` on any conversion writes that settings file for you, from the options
of the run; a key the tool does not know is refused, so a typo cannot fall back
to a default in silence.

## Your Makefile

The build's own knobs — cartridge mapping, save RAM, coprocessor, ROM size,
the link-time checks — are on @subpage tools_build, each with its default.

## Music

Music is the one asset the build takes from the Makefile rather than from a
settings file: `USE_SNESMOD := 1` and `SOUNDBANK_SRC := music/song.it` run
@ref tools_opensnes_music and link the soundbank the SNESMOD driver plays.
The earlier converter of the same files, `smconv`, is on @ref tools_legacy.

## The emulator: luna

luna is the cycle-accurate emulator behind `opensnes run` and `opensnes test`,
and `luna mcp` is the debugger. @subpage tools_luna is the pinned release's own
`--help`, one section per subcommand, generated from the binary.

## Free assets to start from

Art and sound under CC0 or open licences, to prototype before your own
(check each item's licence):

| Source | What |
|--------|------|
| [itch.io, CC0 pixel art](https://itch.io/game-assets/assets-cc0/tag-pixel-art) | Sprites and tilesets |
| [itch.io, 16-bit assets](https://itch.io/game-assets/free/tag-16-bit) | Sprites and tilesets |
| [OpenGameArt, CC0](https://opengameart.org/content/cc0-resources) | Art and sound |
| [Kenney](https://kenney.nl/assets) | Art and sound, CC0 |
| [Freesound](https://freesound.org) | Samples, licence per sound |

## Where they fit with the rest of the docs

- The **why** behind the numbers these tools produce — bpp, VRAM cost, palette
  conventions — is @ref craft_planning. Read it before you commit to a mode.
- The **level workflow** that `opensnes-tileset` + `opensnes-level` enable end
  to end is @ref craft_tiles_to_levels and the @ref tutorial_map tutorial.
- The **audio** side pairs with @ref hardware_apu and @ref tutorial_audio.

> **The one rule.** Keep the *source* (the PNG, the Tiled project, the WAV, the
> `.it`) as your single source of truth and let the tools regenerate the binary
> on every build. The moment you hand-edit a `.pic` or a `.brr`, it drifts from
> its source and you have lost the pipeline's whole benefit.

A project written with the earlier converters (gfx4snes, tmx2snes, img2snes,
font2snes, wav2brr, palplan, aseprite2snes) finds each one, and the command that
replaces it option by option, in @subpage tools_legacy.
