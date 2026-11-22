"""hdma/hdma_wave: the HDMA table of the hand-built ripple (res/wavetable.bin).

Imported by build.py, and runnable alone: `python3 hdma_wave.py` writes the
file, `--check` says whether the committed one is what it writes.

The layout is what the example reads (main.c: ENTRY_BYTES, WAVE_WRAP):
ENTRIES entries of [0x01][lo][hi] (one scanline, then a 16-bit BG1HOFS
value, two's complement) and a 0x00 terminator. Line L of the frame reads
entry phase+L, the phase going 0..WAVE_WRAP-1, so the table is a function
of (entry mod WAVE_WRAP): ENTRIES = WAVE_WRAP + 224, one full screen past
the wrap, and every component below has a whole number of cycles in
WAVE_WRAP entries, which makes the wrap seamless.

The formula is ours: a sum of three sines, rounded to whole pixels.

  ripple   26 cycles / 672 entries  (period 25.85 lines)  amplitude 7.5 px
  chop     52 cycles / 672 entries  (period 12.92 lines)  amplitude 1.5 px
  sway      4 cycles / 672 entries  (period 168 lines)    amplitude 2.5 px

The ripple is the water, the chop roughens its crests, the sway makes the
whole picture breathe slowly; the peak is at most 11.5 px (the `hdma`
module's wave, at the example's default amplitude, peaks at 10).
"""
from __future__ import annotations

import argparse
import math
import struct
import sys
from pathlib import Path

WAVE_WRAP = 672
ENTRIES = WAVE_WRAP + 224            # 896
# (cycles per WAVE_WRAP entries, amplitude in pixels, phase in radians)
COMPONENTS = ((26, 7.5, 0.0), (52, 1.5, 1.1), (4, 2.5, 0.6))


def sample(i: int) -> int:
    i %= WAVE_WRAP
    return round(sum(a * math.sin(2 * math.pi * c * i / WAVE_WRAP + p) for c, a, p in COMPONENTS))


def build_hdma_wave() -> dict[str, bytes]:
    table = b"".join(struct.pack("<Bh", 1, sample(i)) for i in range(ENTRIES))
    return {"wavetable.bin": table + b"\x00"}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true", help="compare with the committed file instead of writing it")
    args = ap.parse_args()
    path = Path(__file__).resolve().parents[2] / "examples" / "hdma" / "hdma_wave" / "res" / "wavetable.bin"
    data = build_hdma_wave()["wavetable.bin"]
    if args.check:
        if not path.is_file() or path.read_bytes() != data:
            print(f"  DIFFERS {path}")
            return 1
        return 0
    path.write_bytes(data)
    return 0


if __name__ == "__main__":
    sys.exit(main())
