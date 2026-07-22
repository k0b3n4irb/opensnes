#!/usr/bin/env python3
"""Regenerate res/au.brr as an RMS-boosted variant of speech_synth's AU.

The speech_synth phoneme bank is PEAK-normalized (24000), and a vowel's
glottal-pulse waveform has a high crest factor — fine inside a spoken
phrase, but ~3x quieter in RMS than the soundboard's recorded cello.
This applies a mild tanh soft-knee (x1.4 drive — higher drives turn the
vowel metallic: hard-saturated glottal pulses ring like a phone) before the same BRR encode,
lifting the loudness to match the other pads without clipping.

Run from this directory:  python3 gen_au_boost.py
(reuses the formant synth + encoder from speech_synth's gen_phonemes.py)
"""
import os
import sys
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
SPEECH = HERE.parent / "speech_synth"
sys.path.insert(0, str(SPEECH))

# gen_phonemes.py regenerates its whole bank AT IMPORT TIME, writing
# `res/...` relative to the CWD — import it from a throwaway dir so
# those side-effect files land nowhere that matters.
with tempfile.TemporaryDirectory() as td:
    os.makedirs(Path(td) / "res")
    old = os.getcwd()
    os.chdir(td)
    try:
        import gen_phonemes as gp  # noqa: E402
    finally:
        os.chdir(old)

# A FRANK one-shot "AU": ~500 ms of vowel with the envelope baked into
# the PCM (5 ms attack, exponential decay, drive 1.9 for a light
# robotic-synth edge — owner-tuned), no loop — the
# looping variant + a slow voice fade read as a reverb tail.
period = gp.voiced_loop([(570, 80, 1.0), (840, 90, 0.7), (2410, 150, 0.15)])
n = int(0.50 * gp.SR / 16) * 16
au = np.tile(period, n // len(period) + 1)[:n]
t = np.arange(n) / gp.SR
env = np.minimum(t / 0.005, 1.0) * np.exp(-t / 0.14)
boosted = np.tanh(1.9 * au * env / 24000.0) * 27000.0
data = gp.encode(boosted, looped=False)
(HERE / "res" / "au.brr").write_bytes(data)
print(f"au.brr (one-shot): {len(data)} B ({len(data)//9} blocks)")
