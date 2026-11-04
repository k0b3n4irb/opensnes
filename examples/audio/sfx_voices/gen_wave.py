#!/usr/bin/env python3
"""Write res/wave.wav: one 64-sample cycle of a sine plus its 3rd harmonic,
16-bit mono. The .wav.toml loops it end to end, so the S-DSP replays one
cycle for ever: the pitch you send is the note you hear (a synthesised
waveform, the shape a tracker-less game engine drives by hand)."""
import math, struct, wave
N = 64
pcm = [int(12000 * (math.sin(2 * math.pi * i / N) + 0.35 * math.sin(6 * math.pi * i / N))) for i in range(N)]
with wave.open("res/wave.wav", "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(32000)
    w.writeframes(b"".join(struct.pack("<h", s) for s in pcm))
