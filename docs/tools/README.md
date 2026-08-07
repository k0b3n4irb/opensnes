# The OpenSNES toolbox {#tools}

Your game is written in C, but the SNES does not eat PNGs, Tiled maps, or WAV
files — it eats tiles, tilemaps, BGR555 palettes, BRR samples, and SPC700
soundbanks. The OpenSNES tools are the converters that bridge the two: each takes
an artist- or musician-friendly source format and emits the exact binary the
hardware (and this SDK's runtime) expects.

You rarely run most of them by hand — the build system wires the common ones in,
so dropping a `.png` or `.wav` in an example's `res/` is usually enough. But
knowing what each tool does, and which knobs it exposes, is what takes you from
"the example builds" to "I shaped my own assets on purpose." That confidence is
what this section is here to give.

## The pipeline

Most assets flow through a short, one-way pipeline from source art to ROM data:

```
  RGB art ──► img2snes ──► gfx4snes ──► tmx2snes ──► map engine
   (.png)     (quantize)   (tiles +     (Tiled map     (mapLoad)
                            palette +     + that .map
                            tilemap)      → map binaries)

  font.png ──► font2snes ──► text tiles      (dmaCopyVram)
  sound.wav ─► wav2brr   ──► .brr sample     (audioLoadSample)
  music.it ──► smconv    ──► SNESMOD bank    (spcLoad / spcPlay)
```

Each tool does exactly one conversion; the Makefile chains them. It is the
Unix-pipe idea applied to asset building — small, composable, individually
testable steps rather than one opaque converter.

## The tools at a glance

| Tool | Turns… | …into | In your build |
|------|--------|-------|---------------|
| @subpage tools_gfx4snes | an indexed PNG/BMP | tiles + palette + tilemap | auto (drop a PNG in `GFXSRC`) |
| @subpage tools_tmx2snes | a Tiled JSON map | tilemap + collision + objects | one line per example |
| @subpage tools_img2snes | RGB/RGBA artwork | a palette-indexed PNG (feeds gfx4snes) | manual pre-step |
| @subpage tools_font2snes | a 96-glyph font PNG | 2bpp/4bpp text tiles | manual, optional |
| @subpage tools_wav2brr | a PCM `.wav` | a `.brr` sound sample | auto (drop a WAV in `res/`) |
| @subpage tools_smconv | an Impulse Tracker `.it` | a SNESMOD soundbank | auto (`USE_SNESMOD`) |

## Three levels of wiring

The tools sit at three levels of automation — worth knowing so you reach for the
right one:

1. **Zero-config, automatic.** `gfx4snes` (via the `GFXSRC` Makefile variable)
   and `wav2brr` (any `res/*.wav`) and `smconv` (`USE_SNESMOD := 1`) run for you
   during `make`. Drop the source in, reference the output, done.
2. **One explicit line.** `tmx2snes` is called from an example's own Makefile,
   because a Tiled project has choices (which layers, which extra outputs) the
   build cannot guess.
3. **By hand, when you need them.** `img2snes` (RGB → indexed) and `font2snes`
   (custom font) are standalone pre-steps you run once and commit the result.
   Looping `wav2brr` samples are also hand-built (the loop points are yours).

All binaries live in `bin/` and are built by `make tools`. Every tool prints
`--help`; the pages here are the guided version.

## Where they fit with the rest of the docs

- The **why** behind the numbers these tools produce — bpp, VRAM cost, palette
  conventions — is @ref craft_planning. Read it before you commit to a mode.
- The **level workflow** that `gfx4snes` + `tmx2snes` enable end-to-end is
  @ref craft_tiles_to_levels and the @ref tutorial_map tutorial.
- The **audio** side pairs with @ref snes_sound_guide and @ref tutorial_audio.

> **The one rule.** Keep the *source* (the PNG, the Tiled project, the WAV, the
> `.it`) as your single source of truth and let the tools regenerate the binary
> on every build. The moment you hand-edit a `.pic` or a `.brr`, it drifts from
> its source and you have lost the pipeline's whole benefit.
