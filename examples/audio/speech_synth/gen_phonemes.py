#!/usr/bin/env python3
"""Generate the phoneme bank for speech_synth — own art, krom's method.

Five phonemes spell "OPEN SNES": AU (o), PP (p), EA (e), NN (n), SS (s).
Vowels/nasal: formant-filtered glottal loops (loop from block 1, krom's
layout). Plosive: a shaped broadband burst, one-shot. Fricative: shaped
high-frequency noise, one-shot. Encoded with the filter-searching BRR
encoder (block 0 and the loop block forced to filter 0).
"""
import math
import numpy as np

SR = 32000.0
F0 = 120.0        # spoken male pitch

def formant(x, fc, bw, gain):
    w = 2 * np.pi * fc / SR
    r = math.exp(-math.pi * bw / SR)
    a1, a2 = -2 * r * math.cos(w), r * r
    y = np.zeros_like(x); y1 = y2 = 0.0
    for i in range(len(x)):
        y0 = x[i] - a1 * y1 - a2 * y2
        y[i] = y0
        y2, y1 = y1, y0
    return gain * y * (1 - r)

def glottal(n):
    t = np.arange(n) / SR
    ph = 2 * np.pi * F0 * t
    return sum(np.sin(k * ph) / k for k in range(1, 30))

def voiced_loop(formants, periods=8):
    n = int(round(periods * SR / F0 / 16.0)) * 16
    src = glottal(n * 3)[n:2*n]     # skip filter warmup
    out = np.zeros(n)
    for fc, bw, g in formants:
        out += formant(glottal(n * 3), fc, bw, g)[n:2*n]
    return out / np.abs(out).max() * 24000

def noise_burst(ms, color_fc, color_bw, env_pow):
    n = int(SR * ms / 1000 / 16) * 16
    rng = np.random.default_rng(7)
    x = rng.normal(0, 1, n)
    y = formant(x, color_fc, color_bw, 1.0)
    env = np.linspace(1, 0, n) ** env_pow
    y = y * env
    return y / np.abs(y).max() * 22000

PHONEMES = {
    # name: (samples, looped)
    'AU': (voiced_loop([(570, 80, 1.0), (840, 90, 0.7), (2410, 150, 0.15)]), True),   # 'o'
    'PP': (np.concatenate([np.zeros(96), noise_burst(24, 900, 600, 2.5)]), False),    # plosive
    'EA': (voiced_loop([(530, 80, 1.0), (1840, 110, 0.5), (2480, 150, 0.2)]), True),  # 'e'
    'NN': (voiced_loop([(250, 60, 1.0), (1200, 150, 0.12), (2200, 200, 0.08)]), True),# nasal
    'SS': (noise_burst(140, 5200, 2200, 1.2), False),                                  # fricative
}

FILTERS = {0: (0.0, 0.0), 1: (0.9375, 0.0),
           2: (1.90625, -0.9375), 3: (1.796875, -0.8125)}

def encode(samples, looped):
    samples = [int(v) for v in samples]
    n_blocks = len(samples) // 16
    out, p1, p2 = [], 0, 0
    for b in range(n_blocks):
        seg = samples[b*16:(b+1)*16]
        allowed = (0,) if b in (0, 1) else (0, 1, 2, 3)  # loop lands on block 1
        best = None
        for fil in allowed:
            a, bq = FILTERS[fil]
            for r in range(13):
                q1, q2, nibs, err = p1, p2, [], 0.0
                for s in seg:
                    pred = a * q1 + bq * q2
                    nv = max(-8, min(7, int(round((s - pred) / (1 << r)))))
                    rec = max(-32768, min(32767, int((nv << r) + pred)))
                    nibs.append(nv & 0xF)
                    err += (s - rec) ** 2
                    q2, q1 = q1, rec
                if best is None or err < best[0]:
                    best = (err, fil, r, nibs, q1, q2)
        err, fil, r, nibs, p1, p2 = best
        last = b == n_blocks - 1
        header = (r << 4) | (fil << 2) | ((1 if looped else 0) << 1) | (1 if last else 0)
        blk = bytearray([header])
        for i in range(0, 16, 2):
            blk.append((nibs[i] << 4) | nibs[i+1])
        out.append(bytes(blk))
    return b''.join(out)

for name, (samples, looped) in PHONEMES.items():
    data = encode(samples, looped)
    open(f'res/{name.lower()}.brr', 'wb').write(data)
    print(f'{name}: {len(data)} B ({len(data)//9} blocks, {"loop@1" if looped else "one-shot"})')
