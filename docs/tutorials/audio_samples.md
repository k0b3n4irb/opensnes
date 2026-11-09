# Samples, Voices and the Raw APU {#tutorial_audio_samples}

Choose this page for sound driven from C: the `audio` module plays BRR
samples on the eight voices and drives the DSP's envelopes and echo, and the
`apu` module runs an SPC700 program of your own. For tracker music, read
@ref tutorial_audio instead.

## The audio module

`LIB_MODULES += audio` gives
you the full engine with no SPC700 code of your own: the lib ships a
resident driver (built from source at lib build time) and `audio.h`'s
functions drive it — `audioInit()`, `audioLoadSample()` (BRR
streamed into APU RAM at runtime), `audioPlaySampleEx()` (volume/pan/
pitch, 8-voice round-robin polyphony), per-voice ADSR/GAIN, and a
configurable echo with FIR filter. Every call is bounded — the API
returns `AUDIO_ERR_TIMEOUT` rather than hanging. Worked example:
`audio/soundboard` (@ref examples_audio_soundboard). Main-thread only; one engine per ROM (don't link
`audio` and `snesmod` together).

### One-shot samples from WAV {#audio_wav2brr}

A plain one-shot effect — a jump,
a hit, a UI blip, a bit of speech — is a **direct BRR
sample**: convert an audio file to the SNES's BRR format once, bake it into the
ROM, and trigger it from C. This is what @ref examples_audio_sfx_from_wav does.

**1. Put a settings file beside the WAV.** @ref tools_opensnes_sample turns a
PCM `.wav` into a `.brr` using the same encoder @ref tools_opensnes_music uses
on a module's samples, and the build runs it for every settings file that names it:

```toml
# res/jump.wav.toml — a one-shot effect
tool = "opensnes-sample"

[encode]
```

```toml
# res/cello.wav.toml — a looping sample (loop between sample indices 2048 and 8192)
tool = "opensnes-sample"

[encode]
loop = [2048, 8192]
```

Input is PCM WAV (8- or 16-bit, mono or stereo — stereo is downmixed). Keep
the source at or below **32 kHz**; that is the DSP's ceiling, and a higher rate
plays back lower and slower (the tool warns, and says by how much).
`opensnes-sample inspect res/jump.wav` prints the block count and the audio
RAM a sample costs without writing anything.

**2. The build bakes the `.brr` into the ROM.** `opensnes-sample encode` runs
before the first C object; it writes `res/jump.brr`, a `res/jump_data.as` the
build links (its own `ASSET_SECTION`, so any bank but $00), and a
`res/jump.h` that declares `jump_brr[]`, `jump_brr_end[]`, the size
(`jump_brr_size`) and the loop point (`jump_brr_loop`, a byte offset; 0 for
a one-shot).

**3. Load it once, play it on demand:**

```c
#include "res/jump.h"

audioLoadSample(0, jump_brr, (u16)(jump_brr_end - jump_brr), jump_brr_loop);
// ...later, when the player jumps:
audioPlaySample(0);
```

See @ref audio_samples in the API reference for the full sample API
(`audioLoadSample`, `audioPlaySample`, `audioUnloadSample`).

### Loading while the game runs

`audioLoadSample()` returns when the whole sample is in the sound CPU's
memory: about 650 master cycles a byte, so a 5.6 KB voice stops the game
for ten frames. Fine under a title screen; not during play. To swap a
sample during play, send it a piece a frame:

```c
audioLoadSampleBegin(SMP_VOICE, replay_brr, (u16)(replay_brr_end - replay_brr), 0);
loading = 1;
// ...in the game loop, once a frame:
if (loading && audioLoadSampleStep(64) == 0) {
    loading = 0;            /* whole: SMP_VOICE can be played */
}
```

Voices that are playing go on, and `audioPlaySample()`, volume, pitch and
stop commands pass between two steps. The slot cannot be played before a
step returns 0. One sample at a time; `audioUnloadSample()` on it gives
the load up.

What a step costs, measured on luna (interrupts included): about 18,700
master cycles whatever its size (four commands and the end of the
transfer), plus about 650 a byte. Bytes travel three at a time, so a step
is a multiple of 3:

| `maxBytes` | Sent | Master cycles | Share of a frame |
|---:|---:|---:|---:|
| 64  | 63  | 59,750  | 17 % |
| 128 | 126 | 100,800 | 28 % |
| 256 | 255 | 184,750 | 52 % |

A 5.3 KB voice is whole in 85 frames at 64 bytes a frame.

### A swap area: samples at addresses you choose {#audio_swap_area}

`audioLoadSample()` puts each sample where the last one ended, and
`audioUnloadSample()` gives memory back only for the last one. A game whose
samples do not all fit in the sound CPU's 62 KB keeps a resident set and
replaces the others during play, in places it chooses:

```c
#define SWAP_A 0x8000                    /* places the game decided on */
#define SWAP_B 0xA000

audioLoadSampleAt(SMP_ENGINE, SWAP_A, engine_brr, (u16)(engine_brr_end - engine_brr), 0);
/* ...later, the level changes: another sound in the same place */
audioStopVoice(VOICE_ENGINE);
/* two VBlanks: see below */
audioLoadSampleBeginAt(SMP_ENGINE, SWAP_A, boss_brr, (u16)(boss_brr_end - boss_brr), 0);
/* then audioLoadSampleStep() once a frame, as above */
```

The place is any byte from $0B00 to the end of the sample area: a
directory entry is two 16-bit addresses and the S-DSP reads the sample
from the byte its entry names, 9 bytes a block, so there is no alignment
(`audioLoadSample()` places samples on any byte too). It may not overlap
another loaded sample or the load in steps under way, except the slot's
own old place: a slot is replaced where it was. A refused place answers
`AUDIO_ERR_NO_MEMORY` (below $0B00, at or past the end of the area, past
$FFFF, on another sample, a size of 0, or under 3 for the version in steps)
and `AUDIO_ERR_INVALID_ID` (slot 64 or more, the slot being loaded in steps,
a second load in steps); nothing is sent then.

The two allocators share the memory without meeting. A sample placed above
the point where `audioLoadSample()` would load next moves that point up to
its end, so an ordinary load always goes above the placed ones; a sample
placed lower leaves the point where it was, and unloading it gives nothing
back: its place is reused by placing another sample there. Placing a sample
checks the 64 slots first, about 20 scanlines.

**Silence before overwriting.** A voice still playing the sample of a place
reads the new bytes as they arrive. `audioStopVoice()` is a key-off, which
is a release, not a cut: the S-DSP takes 8 off the voice's envelope every
sample, from at most $7FF, so the voice still sounds for 256 samples, 8 ms,
and goes on reading its sample meanwhile (anomie's S-DSP doc). Measured on
luna with @ref examples_audio_sample_swap, against a build whose
replacement holds the old bytes, so that any difference in the output is a
changed byte the voice played:

| Before the first step | Key-off to first step | Output |
|---|---:|---|
| voice left playing | — | the new bytes are heard (expected) |
| `audioStopVoice()`, step at once | 36 scanlines | differs for 8 ms |
| `audioStopVoice()` at scanline 200, one VBlank | 62 scanlines | differs for 2 ms |
| `audioStopVoice()` after a VBlank, one VBlank | 294 scanlines | identical |
| `audioStopVoice()`, two VBlanks | 556 scanlines | identical |

The voice's envelope reads 0 133 scanlines (8.4 ms) after `audioStopVoice()`.
So the rule: stop every voice that may be playing the place, then let 8 ms
pass before the first step. Two `WaitForVBlank()` always do, wherever in the
frame the stop was sent; one does only when the stop went at least 126
scanlines before the VBlank. The same holds for `audioLoadSampleAt()`.

From `audioLoadSampleBeginAt()` to the last step the slot is "not loaded":
a note asked of it is refused (`audioPlaySampleOn()` returns
`AUDIO_VOICE_NONE`) and plays again once the new sample is whole.

**Notes on other voices.** A load in steps does not move a note to another
frame: in the example twelve notes on another voice went in twelve frames
running, with and without the load. Within the frame, a note sent before
the step goes at its usual time; sent after a 192-byte step it goes 105
scanlines later, the step's own time. Send the frame's notes first.

### Choosing the voice

`audioPlaySample()` and `audioPlaySampleEx()` pick a voice round-robin
and return it (or `AUDIO_VOICE_NONE`). `audioPlaySampleOn(voice, sample,
volume, pan, pitch)` plays on the voice you name — reserve one for a
long sound that must not be cut by the next effects, and shape its
envelope before the key-on:

```c
#define VOICE_ENGINE 7
audioSetADSR(VOICE_ENGINE, 4, 7, 7, 0);                /* slow attack */
audioPlaySampleOn(VOICE_ENGINE, SFX_ENGINE, 90, AUDIO_PAN_CENTER, 0x1000);
/* ... */
audioStopVoice(VOICE_ENGINE);                          /* key-off */
```

`audioStopAll()` keys off all eight voices. Passing `AUDIO_VOICE_AUTO`
as the voice gives the round-robin choice.

To stop several voices at once, give `audioStopVoices()` a mask, one bit
per voice number: it is one command to the sound CPU, where a loop of
`audioStopVoice()` sends one per voice and waits for each to be taken
before the next. The driver holds each key-off about 2 scanlines (the
S-DSP reads KOFF only every other sample, 64 of the sound CPU's cycles, and
a KOFF cleared before that read can leave the voice playing: fullsnes,
KON/KOFF notes; anomie's S-DSP doc; the driver holds it 132), so on a frame
that stops four voices:

```c
audioStopVoices((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3));   /* voices 0-3 */
```

Measured on luna up to the end of a state read that follows: the read alone
7 scanlines, four voices by one `audioStopVoices()` then the read 11, by
four `audioStopVoice()` then the read 25. A mask of 0 sends nothing.

### Volume and pitch

```c
audioSetVolume(100);                     /* master, 0-127 */
u8 v = audioGetVolume();                 /* the value last set */
audioSetVoiceVolume(3, 127, 40);         /* voice 3, left/right */
audioSetVoicePitch(3, 0x1800);           /* 0x1000 = the sample's own rate */
```

Pitch is clamped to 0x3FFF, the S-DSP's 14-bit range.

A sequencer that moves a voice's volume and its pitch at every tick (a
tremolo and a vibrato) sends them together:

```c
audioSetVoicePan(3, AUDIO_PAN_LEFT);     /* once: the side of the voice */
audioSetVoiceVolumePitch(3, 96, 0x1840); /* every tick: one command */
```

Each command is a handshake with the driver, so one call instead of two
is what counts: four voices updated every frame cost about 10 600 master
cycles a voice with the two calls and 5 500 with this one, measured on
luna. The volume is split between left and right by the voice's pan — the
one `audioPlaySampleOn()` gave it, or the one `audioSetVoicePan()` set
since — exactly as `audioPlaySampleOn()` splits it, so a voice that plays
hard left stays hard left.

### Driving effects voice by voice {#audio_voice_recipes}

A game that owns its sound player (it decides which voice a sound takes and
what cuts what) needs three things from the module, all of which it already
has. `@ref examples_audio_sfx_voices` runs each of them and times it; the
figures below are read from that ROM by `testing/manifests/audio_sfx_voices.toml`.

**A looped waveform whose note moves every frame.** Convert one cycle (64
samples, a `loop = [0, 64]` settings file), play it on a voice with a full
sustain, and send the pitch each frame with `audioSetVoiceVolumePitch()`: the
pitch you send is the note you hear. One update costs 3 scanlines at most
(about 4 000 master cycles).

**Stop, then play, on the same voice in one frame.** `audioStopVoice(v)`
followed at once by `audioPlaySampleOn(v, …)` is accepted: the voice takes the
new sample, pitch and volume, and sounds from the next sample period (the
example checks the DSP registers one frame later). The pair costs 12
scanlines at most (about 16 400 master cycles), most of it the play command.
This is the rule "the last sound to arrive cuts the one in progress".

**A sample sent in pieces.** `audioLoadSampleBegin()` then
`audioLoadSampleStep(192)` once a frame loaded three 4 800-byte samples in 75
frames, byte for byte (the manifest compares the head and the tail of each
in ARAM). The worst step cost 115 scanlines (about 157 000 master cycles),
so size the step to the frame's spare time; samples live in one bank each,
so a large bank of effects is several samples, not one.

**Seven voices updated every frame** (volume and pitch together) cost 29
scanlines (about 39 500 master cycles) every frame, 5 600 per voice, in line with
the four-voice figure above. A scanline is 1 364 master cycles; the example
counts them with `profileScanlineStart()` / `profileScanlineEnd()`, so the
numbers come out of RAM with no tool.

### Shaping the envelope: ADSR or GAIN

`audioSetADSR(voice, attack, decay, sustain, release)` gives a voice the
classic envelope (the `AUDIO_ATTACK_*`, `AUDIO_DECAY_*`,
`AUDIO_SUSTAIN_*`, `AUDIO_RELEASE_*` constants). `audioSetGain(voice,
mode)` switches the voice to GAIN instead and writes `mode` to its GAIN
register as is: `$00-$7F` hold the envelope at a fixed level (value × 16);
with bit 7 set, bits 5-6 choose linear decrease, exponential decrease,
linear increase or bent increase, and bits 0-4 the rate (snesdev-wiki,
S-DSP registers, VxGAIN; anomie's S-DSP doc). They shape the attack, so
set them before playing on that voice — which is why
`audioPlaySampleOn()` lets you choose it.

```c
audioSetGain(2, 0x7F);                   /* voice 2 at a steady full level */
audioSetGain(2, 0xC0 | 20);              /* voice 2 fades in linearly */
```

### Echo

Echo takes three steps, in this order: the delay and levels, the filter,
then the voices that feed it.

```c
static const s8 fir_flat[8] = { 0x7F, 0, 0, 0, 0, 0, 0, 0 };

audioSetEcho(4, 40, 50, 50);             /* 64 ms, feedback 40, volume L/R */
audioSetEchoFilter(fir_flat);
audioEnableEcho(0x03);                   /* voices 0 and 1 */
/* ... */
audioDisableEcho();                      /* mutes the echo and stops the ring writes */
```

`{ 0x7F, 0, 0, 0, 0, 0, 0, 0 }` is the identity filter: the echo comes
back unfiltered (snesdev-wiki, S-DSP registers, FIRx, which also says to
set the filter before echo is enabled). `audioSetEcho()` takes the top of the
sound CPU's memory for the echo ring, 2 KB per unit of delay, and answers
`AUDIO_ERR_NO_MEMORY` when samples already loaded are in the way: a game
short of sample memory sets its echo before it loads, and keeps the delay
short. The call blocks: the driver waits for the ring it replaces to run
out (up to 240 ms the first time, 16 ms per unit of the previous delay
after) and clears the new one. Call it when a scene starts, never during
play. `audioDisableEcho()` then `audioEnableEcho()` is the cheap way to
turn the same echo off and on; the ring keeps its memory meanwhile.

A game that wants an echo in one scene only gives the ring back when the
scene ends:

```c
audioReleaseEcho();                      /* echo off, the ring's memory back to the samples */
/* samples may be loaded where the ring was as soon as it returns */
```

The next `audioSetEcho()` takes a ring again, with the same wait, and
answers `AUDIO_ERR_NO_MEMORY` if samples loaded in between are where it
goes: unload them first.

### Asking the engine

| Call | Returns |
|---|---|
| `audioIsReady()` | 1 once `audioInit()` has started the driver; 0 if the driver did not answer |
| `audioGetFreeMemory()` | bytes left for samples in the SPC700's RAM |
| `audioGetVoiceState(voice, &state)` | whether the voice's envelope is still sounding, and the sample, volume, pan and pitch last sent to it |

There is no per-frame audio call: the
engine sends each command as you call it, nothing is deferred.

## The raw APU path

The `apu` module uploads a wla-spc700-assembled program straight through the IPL boot-ROM
protocol — `apuWaitBoot()`, `apuUpload()`, `apuExecute()` — giving full
DSP control (voices, ADSR, echo, pitch) with no tracker involved. Worked
example: `audio/speech_synth` (phoneme-bank speech — upload, DSP
config, per-phoneme sequencing). Like `audio`, it is never linked
with `snesmod` in one ROM.

APU-side memory layout matters: the flat binary is laid out by
`wlalink -b`, and an `.ORG` section that overlaps your growing code
OVERWRITES it silently.
Budget the code page before placing the sample directory.

### Hot-swapping APU programs

`apuWaitBoot()` only works once — the IPL handshake is consumed at
boot. To replace the running program later, use the cooperative reset
protocol (worked example: `audio/apu_switch`):

1. Build the APU program with `APU_CHECK_RESET` (from
   `templates/memmap_spc700.inc`) inside its wait loops. The idiom
   polls CPUIO0 for `APU_RESET_MAGIC`; on match it silences the DSP,
   acks, re-enables the IPL ROM and jumps to `$FFC0` — the boot ROM is
   re-entrant.
2. On the 65816, call `apuReset()`, then `apuUpload()`/`apuExecute()`
   for the next program — but NOT `apuWaitBoot()` (the reset already
   consumed the ready signal).

Two contract points, both shown by `apu_switch`:
`apuReset()` blocks forever on a program that never polls for the
magic, and the next program receives the DSP *dirty* — a residual
`ADSR1` bit 7 from the previous program silently overrides `GAIN`, so
every register a voice depends on must be written explicitly.

## Debugging audio: luna's spc-dump

`luna spc-dump` runs a ROM and exports the complete APU state — 64 KB of
ARAM plus all 128 DSP registers — as a standard playable `.spc`. Diffing
two dumps (yours vs a reference, or two instants of your own ROM) answers
in seconds what ears cannot localize: upload integrity, directory/loop
addresses, per-voice ADSR/pitch/envelope state, phoneme/note schedules.
`--audio-out` (WAV capture) complements it for spectral verification.

## Example ROMs

- `examples/audio/soundboard/` - samples, voices, ADSR and echo from C
- `examples/audio/sfx_from_wav/` - a one-shot effect converted from a WAV
- `examples/audio/sfx_voices/` - voice by voice: a looped waveform at a pitch set every frame, a stop and play on one voice in one frame, a sample sent in pieces, with the cost of each
- `examples/audio/sample_swap/` - samples at addresses the game chooses: a resident set, a swap area replaced during play, the refusals, the silence a replacement needs
- `examples/audio/speech_synth/` - an SPC700 program of its own through `apu`
- `examples/audio/apu_switch/` - replacing the running SPC700 program

## Next Steps

- @ref tutorial_audio "Music with SNESMOD"
- @ref audio.h "Audio API Reference" · @ref apu.h "APU API Reference"
- @ref hardware_apu "The APU"
