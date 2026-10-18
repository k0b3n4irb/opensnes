"""Sound effects from Kenney's packs (kenney.nl, CC0: see the License_*.txt
beside the OGG files in kenney/), decoded with ffmpeg and cut to what the
SPC700's RAM can hold."""
import os
import struct
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))


def load(name, rate=16000, seconds=None, fade=0.03):
    """kenney/<name>.ogg as floats in -1..1, mono, at `rate`, cut at
    `seconds` with a short fade so the cut does not click."""
    raw = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", os.path.join(HERE, "kenney", name + ".ogg"),
         "-ac", "1", "-ar", str(rate), "-f", "s16le", "-"],
        check=True, capture_output=True).stdout
    xs = [v / 32768 for v in struct.unpack("<%dh" % (len(raw) // 2), raw)]
    while xs and abs(xs[-1]) < 1e-3:
        xs.pop()
    if seconds is not None:
        xs = xs[:int(rate * seconds)]
    n = min(len(xs), int(rate * fade))
    for i in range(n):
        xs[-1 - i] *= i / n
    peak = max(abs(x) for x in xs) or 1.0
    return [x * 0.95 / peak for x in xs]
