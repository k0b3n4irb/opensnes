# Audio

**"make it sound like a game."** The SNES has a whole second
processor for sound — the SPC700, with its own 64 KB of RAM, its own S-DSP,
and its own instruction set, talking to the main CPU through just 4 I/O bytes.
This family has two on-ramps — tracker music, and driving the DSP straight
from C — then the DSP's party tricks.

## The ladder

**Tracker music (SNESMOD)** — compose in a tracker, `opensnes-music` converts the
`.it`, the lib plays it:

| Example | Developer question |
|---------|--------------------|
| [snesmod_music](snesmod_music/) | How do I play tracker music with transport controls? |
| [snesmod_sfx](snesmod_sfx/) | How do I mix SFX over music? |
| [sfx_from_wav](sfx_from_wav/) | How do I play a sound effect from a WAV file, converted by the build? |

**Raw APU from C (audio v2)** — the lib uploads its own resident SPC700 driver;
everything else is plain C:

| Example | Developer question |
|---------|--------------------|
| [soundboard](soundboard/) | How do I drive the whole audio-v2 engine from C? |
| [apu_switch](apu_switch/) | How do I hot-swap APU programs at runtime? |
| [play_noise](play_noise/) | How do I make drums from the S-DSP noise generator? |
| [pitch_mod](pitch_mod/) | How do I do hardware vibrato (pitch modulation)? |
| [speech_synth](speech_synth/) | How do I play BRR speech? |
| [echo](echo/) | How do I add echo / reverb from the S-DSP? |

## The idea in one screen

Sound is a co-processor problem: you never touch the DSP registers from the
65816 directly — you hand a driver a command over the 4 I/O ports and it runs
on the SPC700. SNESMOD's driver plays a compiled tracker module; the audio-v2
driver exposes voices, ADSR, BRR samples and the echo unit as C calls
(`audioLoadSample`, `audioPlaySampleEx`, `audioSetEcho`…). Both share the same
64 KB of ARAM, so samples, the echo ring and the driver all compete for it.

*(Audio examples have no screenshot — the lesson is audible. Run them with
sound.)*
