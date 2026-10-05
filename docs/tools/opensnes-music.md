# opensnes-music — Impulse Tracker to soundbank {#tools_opensnes_music}

The music tool of the 1.x family (@ref tools_conventions). Your composer
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
several. A module that needs more SPC RAM than a module may take is named
with its size.

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

Today the build calls `smconv` through `USE_SNESMOD` and `SOUNDBANK_SRC`;
`opensnes-music bank` produces the same files, and the build switches to it
with the family's generic rule.

## Inspect

```sh
opensnes-music inspect music/*.it
# theme.it: "pollen resurrection", 18 patterns, 17 instruments, 17 samples, 21 orders
#   -> 44137 bytes of SPC RAM (13820 free of 57957), 17 BRR sources
```

Nothing is written. The figure is the one that decides whether a song
fits: the SPC700 has 64 KB, the driver and its tables take the rest, and a
module may use 57 957 bytes for its patterns, instruments, samples and
echo buffer. Samples shared between modules of one bank are stored once
(`BRR sources` counts them).

## Spc

```sh
opensnes-music spc music/theme.it --out build/   # → build/theme.spc
```

A 66 048-byte SPC file (the SPC700's RAM and registers with the driver and
the module loaded), for an SPC player or for sharing a song outside the
game.

## Exit codes and messages

0 done; 1 the input was refused (a file that is not an IT module, a bank
number outside 1..255); 2 usage; 3 the file system. `--json` puts the
outputs, the modules and their SPC RAM figures on stdout as one object.
The converter's diagnostics come out in the same shape (`opensnes-music:
file: message`), on stderr; under `--json` the warnings are listed in a
`warnings` array.

## From smconv

| smconv | opensnes-music |
|---|---|
| `smconv -s -o NAME -b N -n -p NAME a.it b.it` | `opensnes-music bank a.it b.it --name NAME --bank N` |
| `smconv -s -f ...` | `--same-size` |
| `smconv a.it` (an `.spc`) | `opensnes-music spc a.it` |
| `smconv -V` (RAM usage) | `opensnes-music inspect` |
| `-i` (HiROM) | the build relocates the origin, as before |
