# opensnes-music — Impulse Tracker to soundbank {#tools_opensnes_music}

The music tool of the `opensnes-*` family (@ref tools_conventions). Your composer
writes in OpenMPT or Schism Tracker and exports `.it`; `opensnes-music`
turns one or several modules into the **soundbank** the SNESMOD driver
plays, tells you what each song costs in the SPC700's 64 KB, and writes a
standalone `.spc` for a player. The converter is `smconv`'s (SNESMOD, by
Mukunda Johnson), so a soundbank from here is byte for byte what `smconv`
wrote.

## Bank

```sh
opensnes-music bank music/theme.it                        # → music/theme.asm, theme.h, theme.bnk
opensnes-music bank music/*.it --name soundbank --bank 1  # several modules, one bank
opensnes-music bank sfx/*.it --name sfx --same-size       # a sound-effect bank: every module sized like the first
```

**In:** Impulse Tracker modules, uncompressed or IT 2.14/2.15 compressed
samples, up to 8 channels (the SPC700 has eight voices).

**Out:** `NAME.asm` (the data, in ROM bank `--bank`, default 1 — the
OpenSNES layout; bank 0 holds the code), `NAME.h` (one constant per module
and per effect, prefixed `NAME`), `NAME.bnk` (the sample data). `NAME`
defaults to the module's name for one input and to `soundbank` for
several. A module that does not fit the SPC700's RAM is refused, named
with what it needs and what the driver has, and nothing is written.

**The bank as an asset.** A soundbank has several sources, so its settings
live in a file named after it (@ref tools_conventions):

```toml
tool = "opensnes-music"

[bank]
inputs = ["theme.it", "jingle.it"]
bank = 1
```

`opensnes-music bank music/soundbank.toml` builds `soundbank.asm`, `.h`
and `.bnk` beside the file; `opensnes-music bank music/*.it --name
soundbank --save` writes that file from a command-line run.

## In a game

The build runs `opensnes-music bank` for you; you list the modules in the
Makefile:

```makefile
USE_SNESMOD   := 1
SOUNDBANK_SRC := music/theme.it sfx/effects.it    # one module per file
```

It converts them (`--name soundbank`, `--bank` from `SOUNDBANK_BANK`, 1 by
default) and links the soundbank; the HiROM build relocates its origin for
you. A settings file is for running the tool by hand: the build links a
soundbank only through `SOUNDBANK_SRC`. In C, the `.h` gives each module
and effect a name:

```c
#include "soundbank.h"

snesmodInit();
snesmodSetSoundbank(SOUNDBANK_BANK);
snesmodLoadModule(MOD_THEME);             // load a module
u8 jump = snesmodLoadEffect(SFX_JUMP);    // load an effect, keep its slot
snesmodPlay(0);                           // play from pattern 0

/* in the game loop */
snesmodPlayEffect(jump, 127, 128, SNESMOD_PITCH_NORMAL);   // volume, pan, pitch
snesmodProcess();                         // once per frame
```

`examples/audio/snesmod_music`, `snesmod_sfx`, and the games `likemario` and
`tetris` do this. The runtime side is @ref tutorial_audio.

- **Many modules, one bank.** Songs through `snesmodLoadModule(N)` and
  `snesmodPlay(0)`, effects through `snesmodLoadEffect(N)` and
  `snesmodPlayEffect(slot, …)`.
- **The SPC700 limits are real.** Eight channels; samples become BRR at
  32 kHz at most; `inspect` says how much of the 64 KB a module takes, and
  music and samples share it.


## Inspect

```sh
opensnes-music inspect music/*.it
# theme.it: "pollen resurrection", 18 patterns, 17 instruments, 17 samples, 21 orders
#   -> 16446 bytes of SPC RAM (13506 free of 29952), 17 BRR sources
```

Nothing is written. The figure is the one that decides whether a song
fits. The driver loads the module at `$1A00` (its patterns, instruments
and sample table) and the module's BRR samples right after it; the echo
buffer sits under `$FF00` and takes 2 KB per step of the module's echo
delay (`edl` in its `[[SNESMOD]]` message): with echo it starts at page
`$FF - 8 * edl`. So a module has 58 368 bytes without echo and
58 624 - 2 048 * edl with it: 29 952 for the `edl 14` above.
The `MOD_<NAME>_SIZE` constant of the soundbank's `.h` is another figure,
kept as smconv wrote it (it counts the echo and leaves out the module's
tables); the one that decides whether a song fits is this one.
Samples shared between modules of one bank are stored once in the bank
(`BRR sources` counts them); in the SPC700's RAM each module holds its own.

## Spc

```sh
opensnes-music spc music/theme.it --out build/   # → build/theme.spc
```

A 66 048-byte SPC file (the SPC700's RAM and registers with the driver and
the module loaded), for an SPC player or for sharing a song outside the
game.

## Exit codes and messages

0 done; 1 the input was refused (a file that is not an IT module, a module
too big for the SPC700's RAM, a bank number outside 1..255); 2 usage; 3 the
file system. `--json` puts the
outputs, the modules and their SPC RAM figures on stdout as one object.
The converter's diagnostics come out in the same shape (`opensnes-music:
file: message`), on stderr; under `--json` the warnings are listed in a
`warnings` array.
