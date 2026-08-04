# OpenSNES starter

A complete, running Super Nintendo game in ~50 lines of C — a sprite you steer
with the D-pad. Clone it, build it, and start replacing the parts marked in
`main.c` with your own game.

![what you get: a movable sprite](res/player.png)

## Get it

**As a GitHub template** — click **“Use this template”** at the top of the repo
page to make your own copy, then clone it.

**Or copy it out of the SDK** — the `starter/` directory of the
[OpenSNES](https://github.com/k0b3n4irb/opensnes) repo *is* this project. Copy
it anywhere and start editing.

> **Just want a project folder, no git repo or CI?** The SDK ships a scaffolder:
> `opensnes init my-game --template game` writes a minimal single-file project
> locally. This starter is the heavier option — a real git repo with CI and a
> PNG→tiles asset pipeline, ready to push to GitHub.

## Build & run

You need the OpenSNES SDK. Get it two ways:

- **A release** — download and unzip the build for your OS from the
  [releases page](https://github.com/k0b3n4irb/opensnes/releases).
- **From source** — `git clone` the repo and run `make` once.

Then point this project at it and build:

```sh
make OPENSNES=/path/to/opensnes      # or edit the OPENSNES line in the Makefile
```

That produces `game.sfc`. Run it in any SNES emulator, or with the SDK's
bundled emulator:

```sh
/path/to/opensnes/tools/luna-test/bin/luna run game.sfc
```

> If you copied this directory *while it was still inside the SDK repo*, the
> Makefile's default `OPENSNES` already points at the SDK and plain `make`
> works — but set `OPENSNES` explicitly once you move the project out.

## What's here

```
starter/
  main.c        the game: setup, screen-on, game loop (edit the marked parts)
  data.asm      links res/player.pic + .pal into the ROM (add your assets here)
  res/player.png  the sprite art — gfx4snes converts it at build time
  Makefile      TARGET, ROM_NAME, LIB_MODULES, OPENSNES path
  .github/workflows/build.yml   CI that builds your ROM on every push
```

## Grow it

Everything in `main.c` is one of three parts — **setup**, **screen-on**, and
the **game loop** — labelled in the file. To go further:

- **Add graphics** — drop a `.png` in `res/`, copy the gfx4snes rule in the
  `Makefile`, and `.incbin` its `.pic`/`.pal` in `data.asm`.
- **Add sound** — drop a `.wav` in `res/` and `.incbin` its `.brr`; it converts
  automatically (see the SDK's audio tutorial).
- **Add layers, text, scrolling, more** — the SDK ships **82 examples** and a
  set of [tutorials](https://github.com/k0b3n4irb/opensnes/tree/main/docs) plus
  game-craft guides (budgeting, choosing a background mode, composing layers).
  Add the module you need to `LIB_MODULES` and follow the matching example.

## License

This starter is MIT-licensed (same as OpenSNES). `res/player.png` is
OpenSNES-original art. Replace it with your own; keep or drop this notice as you
like — it's your project now.
