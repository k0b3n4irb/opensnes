# apu_switch — two S-DSP programs, swapped at runtime

Two SPC700 programs written straight against the S-DSP, and the way to
replace the running one at any time. Press **A** for a drum kit made of
nothing but the DSP's noise generator, **B** for a cello sung by hardware
vibrato — the kit cuts, the cello blooms, and back, forever. The backdrop
tracks the active program (red = drums, amber = cello).

The two programs are ports of krom (Peter Lemon)'s **PlayNoise** and
**PitchMod** demos; the swap is his **PlayTwoSong** protocol, productized.
krom's original swap demo uses samples ripped from commercial games
(Axelay, Final Fantasy VI, the Halken logo), so this one swaps between its
own two programs instead — same protocol, no rights issues.

ROM mode: LoROM (project default).

## Controls

| Button | Program |
|--------|---------|
| A | the drum kit (program A, at boot) |
| B | the vibrato cello (program B) |

## SNES Concepts

- Raw APU programming via the `apu` module (IPL upload from C)
- Cooperative APU reset: CPU→APU messaging over `$2140` *after* boot
- The IPL ROM is re-entrant — re-enable it (`CONTROL` bit 7), jump to
  `$FFC0`, and the `$AA/$BB` + `$CC` upload protocol runs again
- The S-DSP noise source (`NON`, the `FLG` noise clock) as an instrument;
  the ADSR envelope *is* the drum type
- Pitch modulation (`PMON`): voice N−1's output scales voice N's pitch; a
  silent voice as a hardware LFO
- FIR echo configuration (`ESA`/`EDL`/`EFB`/`FIR0`) and clearing the echo
  buffer RAM before enabling writes

## The Swap

The `apu` module's upload path runs once, at boot. `apuReset()` adds the
missing capability: it writes `APU_RESET_MAGIC` to I/O port 0; each SPC700
program polls for it inside its wait loops (the `APU_CHECK_RESET` idiom
from `templates/memmap_spc700.inc`), silences the DSP, acks on its output
ports, and jumps back to the IPL boot ROM at `$FFC0` — which is
re-entrant, so the full boot handshake works again and the next
`apuUpload()` / `apuExecute()` proceeds exactly like the first.

**Hot-swap hygiene.** The DSP arrives *dirty* in the second program: this
example's cello inherited the hi-hat's ADSR on its LFO voice and lost its
vibrato until the program zeroed `V0ADSR` explicitly. Every register a
voice depends on must be written — never rely on reset defaults after a
swap. The tests check it: after B, the noise voice (`NON`) and the noise
clock (`FLG`) of the drum kit are back to 0.

## Program A — a Drum Kit with Zero Samples

The S-DSP has a built-in white-noise generator: set a voice's `NON` flag
and the voice plays noise instead of BRR data. Every drum in the looping
bar — kick 480 ms, closed hi-hat 240 ms, open hi-hat 240 ms, snare 960 ms —
is nothing but three register writes: the noise clock (`FLG` bits 0-4: low
= rumble, high = hiss), an ADSR envelope (fast one-shot = closed hat, long
release = open hat), and a bare `KON`. A single-tap FIR echo (feedback 80)
gives the kit its room. The program ships **zero bytes of sample data**.

Measured against krom's original (luna `--audio-out`, 40 s captures): onset
gap patterns identical to 0.01 s, per-hit spectral centroids within 2 %, RMS
within 1 %.

## Program B — Hardware Vibrato by Pitch Modulation

`PMON` multiplies a voice's pitch by the *output* of the previous voice,
every sample, in hardware. Voice 1 holds a looping cello note at C5; voice 0
plays a 9-byte square-wave BRR block at pitch `$0003` — far below
audibility, it is a slow LFO whose tiny amplitude (`GAIN = 2`) sets the
vibrato depth. The cello enters dry; one second in, the LFO keys on and the
note starts to sing. Each note rings out like a bow stroke (instant attack,
~7 s fade) and is re-bowed every 7 s. A maximum-length FIR echo (`EDL 15`,
feedback 100) does the concert hall.

Measured against the original: pitch 174.4 Hz vs 174.3 Hz, vibrato depth
±0.82 % on both, LFO rate ~5 Hz on both.

**Why the envelope differs from the original.** krom's demo sets a flat
`GAIN 127` — which would sustain the note forever — yet it audibly decays.
His SPC700 program has no final idle loop: after the last `KON`, execution
falls off the end of the code, through the sample directory, and runs the
cello's BRR bytes *as opcodes* (luna's `spc-dump`: his PC ends up at `$0548`,
inside the sample data). The decay is that runaway code scribbling on the
DSP. This port produces the bow stroke on purpose: the cello's filename in
krom's repo encodes its envelope (`ADSR $FF/$E8`), so the program sets that
ADSR, parks in a clean idle loop, and re-keys the voice every 7 s.

## Sample Provenance

`res/cello.brr` is synthesized here (`devtools/audio_assets/build.py`): a
bow's scratch settling into an exactly periodic string tone, 100 samples a
period at 17 463 Hz — the rate krom's pitch `$08BB` (`$8BB0 >> 4`) plays a
sample at — so the note is the F3 (174.6 Hz) the program was written for. It
loops on its last 8 periods, from byte 2457. It replaced krom's own cello,
whose repository carries no licence.

## Validation

luna manifests: `apu_switch.toml` (boot state, non-silent drums, the swap
asserted on `current_song`), `audio_apu_switch_noise_kick.toml` /
`_noise_snare.toml` (the drum kit's DSP registers, kick then snare) and
`audio_apu_switch_pitch_mod_dry.toml` / `_vibrato.toml` (the cello before and
after the LFO keys on, and nothing left of the drums).

## How to Build

```bash
make
```

## Modules Used

console, apu, input
