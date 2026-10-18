# The examples' sounds: composed and synthesized here, or CC0

Owner, 2026-10-18: "pour les sons, je te laisse champ libre … changer les
sons utilisés par PVSnesLib nous permet de nous émanciper … ajouter un peu de
qualité d'audio et se démarquer suffit."

## Where every sound comes from

| Source | What | Licence, checked how |
|---|---|---|
| `devtools/audio_assets/` | every tracker module and every synthesized instrument (cycles, Karplus-Strong pluck, FM bell, drums, bowed string) | ours, CC0 (owner decision on question 10) |
| Kenney (kenney.nl) | realistic one-shot effects, converted from OGG | CC0: kenney.nl/support ("all game assets on the asset pages are public domain licensed (CC0)") and the `License.txt` in each zip ("License (Creative Commons Zero, CC0) … personal and commercial projects") |

Looked at and not taken:

- **Haydenwoffle .it Soundtrack** (opengameart.org, CC0, VinsCool / Spring):
  real `.it` modules, but 350 KB to 2.3 MB of samples each for 58 KB of SPC
  RAM, and three of nine have more than 8 channels. Usable only after a full
  re-arrangement.
- **The Mod Archive**: licence chosen per module by the uploader; the site
  answered 503 on 2026-10-18. Not needed once we compose.
- Free cello libraries (Philharmonia, Karoryfer): not CC0, or no licence
  stated.

## What the generator has to know about SNESMOD (measured on luna)

- **Instrument mode only.** A module in sample mode (no instruments) loads
  and converts without a word, and plays nothing: SNESMOD reads the
  instrument column as an instrument.
- **A volume envelope on every instrument.** An instrument without one is cut
  a few frames after its note (every note died after about 0.15 s, the bell's
  0.7 s sample included), where libopenmpt holds it. A flat envelope at 64
  with a sustain point holds until the note off.
- Echo lives in the song message: a `[[SNESMOD]]` line, then `edl`, `efb`,
  `evol`, `efir`, `eon` (tools/lib/smconv/src/it2spc.c, `parse_sm_option`).
- SNESMOD plays its sound effects on voices 7 and 8 (it alternates): a song
  meant to have effects over it keeps to six channels.
- Loops: the converter resamples a loop to a multiple of 16 samples; a
  single cycle of 64 or 128 samples needs no resampling.

## How to check a sound without ears

- `ffmpeg -i song.it out.wav` renders a module with libopenmpt: a reference
  that is not SNESMOD.
- A build of the module in a two-line ROM and `luna run --audio-out` gives
  what the SNES plays. The two renders measured the same pitch on every
  instrument (523 Hz for C-5, numpy FFT over 0.4 s).
- A Karplus-Strong loop of N samples whose filter averages a sample with the
  next one is N - 0.5 long, not N + 0.5: the pluck was 19 cents sharp, then 9
  flat, until its playback rate was set from its real loop length.
