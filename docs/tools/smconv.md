# smconv — tracker modules to SNESMOD soundbanks {#tools_smconv}

`smconv` turns music written in a tracker — Impulse Tracker `.it` modules — into
a **SNESMOD soundbank**: the packed music, samples, and effect data the SPC700
plays. It is how you get real, multi-channel music (and sound-effect banks) into
a game, driven from C by a few `spcLoad` / `spcPlay` calls.

## What goes in, what comes out

**In:** one or more `.it` files (each becomes a numbered module — songs and/or
SFX banks).

**Out** (soundbank mode, `-s`):

| File | What it is | Used by |
|------|-----------|---------|
| `<base>.asm` | the soundbank data (assembled into the ROM) | the build |
| `<base>.h` | C constants for each module and effect | your game code |
| `<base>.brr` | the BRR sample data | the soundbank |

Without `-s`, smconv instead writes a single standalone `.spc` you can play in an
SPC player — handy for auditioning a module outside the SDK.

## How you actually use it

You almost never call smconv directly. Declare it in the example Makefile and the
build runs it with the right OpenSNES conventions:

```makefile
USE_SNESMOD  := 1
SOUNDBANK_SRC := music/mysong.it     # one or more .it files
```

Under the hood that becomes:

```sh
smconv -s -o soundbank -b 1 -n -p soundbank music/mysong.it
```

Then, in C, load and play by module number (the `.h` gives you the names):

```c
spcLoad(MOD_MYSONG);   // load a module
spcPlay(0);            // play it
spcPlaySound(SFX_JUMP);// trigger a sound effect from an SFX bank
```

See @ref tutorial_audio and @ref snes_sound_guide for the runtime side.

## The flags behind the Makefile

| Flag | Meaning |
|------|---------|
| `-s` | soundbank mode (the game path; default is standalone `.spc`) |
| `-o FILE` | output base name (**required** in soundbank mode) |
| `-b N` | ROM bank for the soundbank data — **OpenSNES uses 1** (SNESMOD default is 5) |
| `-n` | skip the `hdr.asm` include — **required** in OpenSNES |
| `-p NAME` | symbol prefix (OpenSNES sets it to the output base) |
| `-f` | check `.it` sizes against the first file (for SFX banks) |
| `-i` | HiROM mapping · `-V` verbose (reports SPC RAM usage) |

## Gotchas worth knowing up front

- **Declare, don't invoke.** For a game build, set `USE_SNESMOD` and
  `SOUNDBANK_SRC`; the bank (`-b 1`), no-header (`-n`) and prefix flags are
  OpenSNES-specific and the Makefile gets them right. Calling smconv by hand with
  SNESMOD's defaults will not link.
- **Many modules, one bank.** Pass several `.it` files to get several modules —
  music via `spcLoad(N)` + `spcPlay(0)`, effects via `spcPlaySound(N)`.
- **SPC700 limits are real.** Up to 8 channels; samples auto-convert to BRR at
  32 kHz max; `-V` reports how much of the SPC's 64 KB you are using — watch it,
  because music and samples share that space.
- **HiROM builds** relocate the soundbank origin (the build handles the `.ORG`
  fix-up for you).

## See it in practice

- @ref examples_audio_snesmod_music — a full song playing.
- @ref examples_audio_snesmod_sfx — a sound-effect bank.
- @ref examples_games_likemario and @ref examples_games_tetris — music and SFX in
  real games.

For a single one-shot sample rather than a whole module, use @ref tools_wav2brr —
it shares smconv's BRR encoder, so the samples sound identical.
