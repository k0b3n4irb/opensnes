"""The instruments of the examples, synthesized: nothing recorded, nothing
borrowed. Every function returns floats in -1..1 at the rate it is given;
`to16` quantizes them for a module or a WAV."""
import math
import random
import struct
import wave


def to16(xs, gain=1.0):
    return [max(-32768, min(32767, int(round(x * gain * 32767)))) for x in xs]


def write_wav(path, xs, rate, gain=1.0):
    pcm = [max(-32768, min(32767, int(round(x * gain * 32767)))) for x in xs]
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(struct.pack("<%dh" % len(pcm), *pcm))


def normalize(xs, peak=0.95):
    m = max(abs(x) for x in xs) or 1.0
    return [x * peak / m for x in xs]


def cycle(shape, n, harmonics=24, duty=0.5):
    """One band-limited cycle of n samples: 'saw', 'square', 'pulse',
    'triangle' or 'sine' — additive, so it loops without a click."""
    out = []
    for i in range(n):
        t = i / n
        if shape == "sine":
            v = math.sin(2 * math.pi * t)
        else:
            v = 0.0
            for k in range(1, harmonics + 1):
                if shape == "saw":
                    v += math.sin(2 * math.pi * k * t) / k
                elif shape == "square" and k % 2:
                    v += math.sin(2 * math.pi * k * t) / k
                elif shape == "triangle" and k % 2:
                    v += ((-1) ** ((k - 1) // 2)) * math.sin(2 * math.pi * k * t) / (k * k)
                elif shape == "pulse":
                    v += math.sin(math.pi * k * duty) * math.cos(2 * math.pi * k * (t - duty / 2)) / k
        out.append(v)
    return normalize(out, 0.9)


def mix_cycles(*parts):
    """Weighted sum of equal-length cycles: [(cycle, weight), ...]."""
    n = len(parts[0][0])
    return normalize([sum(c[i] * w for c, w in parts) for i in range(n)], 0.9)


def pluck_period(freq, rate):
    # the filter averages a sample with the NEXT one: the loop is N - 0.5 long
    return max(2, int(round(rate / freq + 0.5)))


def pluck_pitch(freq, rate):
    """The pitch pluck() really sounds: whole samples of delay, minus half."""
    return rate / (pluck_period(freq, rate) - 0.5)


def pluck(freq, rate, seconds, damping=0.996, seed=1):
    """Karplus-Strong string: a burst of noise through a tuned delay."""
    rnd = random.Random(seed)
    period = pluck_period(freq, rate)
    buf = [rnd.uniform(-1, 1) for _ in range(period)]
    out = []
    for i in range(int(rate * seconds)):
        a = buf[i % period]
        b = buf[(i + 1) % period]
        buf[i % period] = damping * 0.5 * (a + b)
        out.append(a)
    fade = int(rate * 0.02)
    for i in range(fade):
        out[-1 - i] *= i / fade
    return normalize(out)


def fm_bell(freq, rate, seconds, ratio=3.5, index=2.5, decay=3.0):
    out = []
    for i in range(int(rate * seconds)):
        t = i / rate
        env = math.exp(-decay * t)
        mod = index * env * math.sin(2 * math.pi * freq * ratio * t)
        out.append(env * math.sin(2 * math.pi * freq * t + mod))
    return normalize(out)


def kick(rate, seconds=0.18):
    out, phase = [], 0.0
    for i in range(int(rate * seconds)):
        t = i / rate
        f = 45 + 110 * math.exp(-t * 35)
        phase += 2 * math.pi * f / rate
        out.append(math.sin(phase) * math.exp(-t * 18) + 0.3 * math.exp(-t * 300) * math.sin(phase * 3))
    return normalize(out)


def snare(rate, seconds=0.16, seed=2):
    rnd = random.Random(seed)
    out, lp = [], 0.0
    for i in range(int(rate * seconds)):
        t = i / rate
        n = rnd.uniform(-1, 1)
        lp += 0.6 * (n - lp)
        tone = math.sin(2 * math.pi * 190 * t) * math.exp(-t * 30)
        out.append(0.8 * lp * math.exp(-t * 22) + 0.5 * tone)
    return normalize(out)


def hat(rate, seconds=0.05, seed=3):
    rnd = random.Random(seed)
    out, prev = [], 0.0
    for i in range(int(rate * seconds)):
        t = i / rate
        n = rnd.uniform(-1, 1)
        hp = n - prev        # first difference: the hiss without the rumble
        prev = n
        out.append(hp * math.exp(-t * 70))
    return normalize(out, 0.8)


def bowed(freq, rate, seconds, vibrato_hz=0.0, seed=4):
    """A bowed-string tone: a saw through a body resonance, with a little
    bow noise; meant to be looped on whole cycles."""
    rnd = random.Random(seed)
    out, phase, lp1, lp2 = [], 0.0, 0.0, 0.0
    for i in range(int(rate * seconds)):
        t = i / rate
        f = freq * (1 + 0.004 * math.sin(2 * math.pi * vibrato_hz * t))
        phase = (phase + f / rate) % 1.0
        saw = 2 * phase - 1 + 0.03 * rnd.uniform(-1, 1)
        lp1 += 0.35 * (saw - lp1)
        lp2 += 0.35 * (lp1 - lp2)
        out.append(0.6 * lp1 + 0.4 * lp2)
    return normalize(out)


def string_period(period, rate, harmonics=None):
    """One period of a bowed string: harmonics with the falling slope of a
    bow's sawtooth and two body resonances (about 500 Hz and 1.6 kHz)."""
    f0 = rate / period
    top = harmonics or int((rate / 2 * 0.9) // f0)
    out = [0.0] * period
    for k in range(1, top + 1):
        f = k * f0
        amp = (1.0 / k) * (1 + 1.5 * math.exp(-((f - 500) / 300) ** 2)
                           + 0.8 * math.exp(-((f - 1600) / 600) ** 2))
        ph = 0.37 * k * k            # fixed phases: no peaky, buzzy sum
        for i in range(period):
            out[i] += amp * math.sin(2 * math.pi * k * i / period + ph)
    return normalize(out, 0.9)


def bowed_note(period, rate, attack_blocks, loop_periods, seed=5):
    """A note that starts with the bow's scratch and settles into an exactly
    periodic tone: the first `attack_blocks` x 16 samples cross-fade from
    noise-modulated to clean, then `loop_periods` clean periods, which loop.
    Returns (samples, loop_start, loop_end)."""
    rnd = random.Random(seed)
    one = string_period(period, rate)
    attack = attack_blocks * 16
    loop_len = loop_periods * period
    out = []
    for i in range(attack + loop_len):
        v = one[i % period]
        if i < attack:
            t = i / attack
            env = min(1.0, i / (0.06 * rate))           # 60 ms rise
            scratch = (1 - t) ** 2 * 0.25 * rnd.uniform(-1, 1)
            v = env * (v * (0.7 + 0.3 * t) + scratch)
        out.append(v)
    return normalize(out, 0.95), attack, attack + loop_len
