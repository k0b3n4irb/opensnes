#!/usr/bin/env python3
"""Test-only: rewrite the samples of an uncompressed .it module.

smconv decodes IT 2.14 / 2.15 compressed samples, and the repo's music is
all uncompressed. This script makes the compressed twins the golden test
needs, from a module whose soundbank is already known:

  itcompress.py in.it out.it --it214       every sample compressed, IT 2.14
  itcompress.py in.it out.it --it215       same, IT 2.15 (Cvt bit 2 set)
  itcompress.py in.it out.it --stretch 8   uncompressed, every sample
                                           resampled 8x so the long ones
                                           span several blocks
  ... --bits16                             and widened to 16-bit first

The options combine; the order is widen, stretch, compress. New sample
data is appended to the file and the sample pointer moved; the old bytes
stay where they were, unreferenced. Mono samples only.

The encoder is deliberately simple (it narrows the bit width when the next
few deltas fit, widens when one does not) — it is not a reference
compressor, only a valid stream that reaches the three width modes of the
format. `stats` in the return value of compress() counts them so the test
can refuse a variant that would not exercise the decoder.

Format, as the decoder in ../src/itloader.c reads it: blocks of at most
0x8000 samples (8-bit) or 0x4000 (16-bit), each a u16 byte length and a
bit stream, LSB first; the width starts at 9 / 17 bits and the integrators
at zero in every block.
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

LOOKAHEAD = 8   # deltas that must fit before the encoder narrows the width


class BitWriter:
    def __init__(self) -> None:
        self.out = bytearray()
        self.acc = 0
        self.n = 0

    def write(self, value: int, width: int) -> None:
        self.acc |= (value & ((1 << width) - 1)) << self.n
        self.n += width
        while self.n >= 8:
            self.out.append(self.acc & 0xFF)
            self.acc >>= 8
            self.n -= 8

    def bytes(self) -> bytes:
        return bytes(self.out) + (bytes([self.acc & 0xFF]) if self.n else b"")


def fits(delta: int, width: int, bits: int) -> bool:
    """Can `delta` (signed, `bits` wide) be written as a sample at `width`?"""
    if width == bits + 1:
        return True
    half = 1 << (width - 1)
    if width <= 6:
        return -half < delta < half          # -half is the escape code
    guard = 4 if bits == 8 else 8            # the band of width codes
    return -half + guard <= delta <= half - guard - 1


def min_width(delta: int, bits: int) -> int:
    for width in range(1, bits + 2):
        if fits(delta, width, bits):
            return width
    raise AssertionError(delta)


def encode_block(deltas: list[int], bits: int, stats: dict[str, int]) -> bytes:
    default = bits + 1
    bw = BitWriter()
    width = default
    need = [min_width(d, bits) for d in deltas]
    for i, delta in enumerate(deltas):
        target = width
        if need[i] > width:
            target = need[i]
        else:
            ahead = max(need[i:i + LOOKAHEAD])
            if ahead < width:
                target = ahead
        if target != width:
            code = target - 1 if target < width else target - 2
            top = 1 << (width - 1)
            if width <= 6:
                bw.write(top, width)
                bw.write(code, 3 if bits == 8 else 4)
                stats["change_a"] += 1
            elif width < default:
                bw.write(top - (4 if bits == 8 else 8) + code, width)
                stats["change_b"] += 1
            else:
                bw.write(top | (target - 1), width)
                stats["change_c"] += 1
            width = target
        # at the default width the top bit is the width-change flag: a
        # sample is written on `bits` bits under a clear top bit
        bw.write(delta & ((1 << bits) - 1) if width == default else delta, width)
        stats["sample_a" if width <= 6 else "sample_b" if width < default else "sample_c"] += 1
    data = bw.bytes()
    assert len(data) <= 0xFFFF, "block does not fit its u16 length"
    return struct.pack("<H", len(data)) + data


def wrap(value: int, bits: int) -> int:
    value &= (1 << bits) - 1
    return value - (1 << bits) if value >> (bits - 1) else value


def compress(pcm: list[int], bits: int, it215: bool, stats: dict[str, int]) -> bytes:
    """pcm: signed samples. Returns the block sequence."""
    out = bytearray()
    block_max = 0x8000 if bits == 8 else 0x4000
    for start in range(0, len(pcm), block_max):
        block = pcm[start:start + block_max]
        deltas, prev, prev_d = [], 0, 0
        for x in block:
            d = wrap(x - prev, bits)
            prev = x
            if it215:
                d, prev_d = wrap(d - prev_d, bits), d
            deltas.append(d)
        out += encode_block(deltas, bits, stats)
    return bytes(out)


def stretch(pcm: list[int], factor: int) -> list[int]:
    """Resample `factor` times longer. Even source samples slide linearly to
    the next one (small deltas, narrow widths); odd ones are held, so the
    source's big steps survive whole and still need the default width."""
    out = []
    for i, x in enumerate(pcm):
        nxt = pcm[i + 1] if i % 2 == 0 and i + 1 < len(pcm) else x
        out.extend(x + (nxt - x) * k // factor for k in range(factor))
    return out


def new_stats() -> dict[str, int]:
    return {k: 0 for k in ("sample_a", "sample_b", "sample_c",
                           "change_a", "change_b", "change_c")}


def rewrite(src: bytes, *, compress_as: str | None = None, bits16: bool = False,
            factor: int = 1) -> tuple[bytes, dict[str, int]]:
    """Returns the new module and the encoder's mode counts."""
    if src[:4] != b"IMPM":
        raise ValueError("not an IT module")
    mod = bytearray(src)
    ordnum, insnum, smpnum = struct.unpack_from("<HHH", mod, 0x20)
    table = 0xC0 + ordnum + 4 * insnum
    stats = new_stats()
    for n in range(smpnum):
        hdr, = struct.unpack_from("<I", mod, table + 4 * n)
        flags, cvt = mod[hdr + 0x12], mod[hdr + 0x2E]
        length, loop_a, loop_b, c5, sus_a, sus_b, ptr = struct.unpack_from("<7I", mod, hdr + 0x30)
        if not flags & 1 or length == 0:
            continue
        if flags & (4 | 8) or cvt & 4:
            raise ValueError(f"sample {n + 1}: stereo, compressed or delta — not handled")
        bits = 16 if flags & 2 else 8
        raw = struct.unpack_from(f"<{length}{'H' if bits == 16 else 'B'}", mod, ptr)
        bias = 0 if cvt & 1 else 1 << (bits - 1)
        pcm = [wrap(v - bias, bits) for v in raw]

        if bits16 and bits == 8:
            pcm, bits = [x * 256 for x in pcm], 16
            flags |= 2
        if factor > 1:
            pcm = stretch(pcm, factor)
            length, loop_a, loop_b, c5, sus_a, sus_b = (
                v * factor for v in (length, loop_a, loop_b, c5, sus_a, sus_b))
        if compress_as:
            data = compress(pcm, bits, compress_as == "it215", stats)
            flags |= 8
            cvt = (cvt & ~4) | 1 | (4 if compress_as == "it215" else 0)
        else:
            mask = (1 << bits) - 1
            data = struct.pack(f"<{len(pcm)}{'H' if bits == 16 else 'B'}",
                               *((x & mask) for x in pcm))
            cvt |= 1    # written signed

        mod[hdr + 0x12], mod[hdr + 0x2E] = flags, cvt
        struct.pack_into("<7I", mod, hdr + 0x30,
                         length, loop_a, loop_b, c5, sus_a, sus_b, len(mod))
        mod += data
    return bytes(mod), stats


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("src", type=Path)
    ap.add_argument("dst", type=Path)
    how = ap.add_mutually_exclusive_group()
    how.add_argument("--it214", dest="compress_as", action="store_const", const="it214")
    how.add_argument("--it215", dest="compress_as", action="store_const", const="it215")
    ap.add_argument("--bits16", action="store_true")
    ap.add_argument("--stretch", type=int, default=1, metavar="N")
    args = ap.parse_args()
    out, stats = rewrite(args.src.read_bytes(), compress_as=args.compress_as,
                         bits16=args.bits16, factor=args.stretch)
    args.dst.write_bytes(out)
    if args.compress_as:
        print(" ".join(f"{k}={v}" for k, v in stats.items()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
