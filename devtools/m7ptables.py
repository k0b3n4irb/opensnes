#!/usr/bin/env python3
"""m7ptables.py — extract & verify krom's Mode 7 perspective-rotation tables.

The mode7_perspective_rotate example ships the exact HDMA tables of krom
(Peter Lemon)'s Mode7/Perspective demo (res/m7{cos,sin,nsin}.bin). This
tool regenerates the binary blobs from his .asm sources (extract mode)
and documents/verifies the underlying math (verify mode):

    entry(angle, line) = trig(2*pi*angle/48) * 20480 / line

with trig = cos / sin / -sin, line = 1..224, value in 8.8 fixed point
(20480 = 80 * 256: an 80x zoom-out at the top scanline shrinking
hyperbolically to 0.36x at the bottom — the perspective divide).
Each of the 48 angle tables is a full HDMA stream: 224 x [count=1][val16]
entries + one terminator byte = 673 bytes; three table sets (cos, sin,
-sin) feed M7A/M7D, M7B and M7C respectively.

Usage:
  m7ptables.py extract <krom_Perspective_dir> <out_dir>   # .asm -> .bin
  m7ptables.py verify  <bin_dir>                          # math check

The verify pass proves every 16-bit value equals the formula under
trunc-or-round (krom's generator's rounding is not bit-pinned, hence
the shipped-verbatim policy — same as the wavetable.bin precedent).
"""
import math
import re
import sys
from pathlib import Path

FILES = (('M7COSTable.asm', 'm7cos.bin', math.cos),
         ('M7SINTable.asm', 'm7sin.bin', math.sin),
         ('M7NSINTable.asm', 'm7nsin.bin', lambda t: -math.sin(t)))
ANGLES, LINES, STRIDE = 48, 224, 673


def extract(src_dir: Path, out_dir: Path) -> None:
    for asm, bin_name, _ in FILES:
        blob = bytearray()
        tables = 0
        for line in (src_dir / asm).read_text().splitlines():
            if re.match(r'M7\w+?\d+:', line):
                tables += 1
                continue
            m = re.match(r'db \$01; dw (-?\d+)', line)
            if m:
                v = int(m.group(1)) & 0xFFFF
                blob += bytes([1, v & 0xFF, v >> 8])
            elif re.match(r'\s*db \$?0+\s*(//|$)', line):
                blob += b'\x00'
        assert tables == ANGLES and len(blob) == ANGLES * STRIDE, \
            f'{asm}: {tables} tables, {len(blob)} bytes'
        (out_dir / bin_name).write_bytes(bytes(blob))
        print(f'{bin_name}: {len(blob)} bytes ({tables} tables)')


def verify(bin_dir: Path) -> int:
    bad = 0
    for _, bin_name, trig in FILES:
        blob = (bin_dir / bin_name).read_bytes()
        for a in range(ANGLES):
            th = 2 * math.pi * a / ANGLES
            base = a * STRIDE
            assert blob[base + STRIDE - 1] == 0, f'{bin_name}[{a}]: no terminator'
            for i in range(LINES):
                off = base + i * 3
                assert blob[off] == 1, f'{bin_name}[{a}] line {i}: count != 1'
                v = blob[off + 1] | (blob[off + 2] << 8)
                if v >= 0x8000:
                    v -= 0x10000
                f = trig(th) * 20480 / (i + 1)
                if v not in (int(f), round(f)):
                    bad += 1
                    if bad < 6:
                        print(f'MISMATCH {bin_name} angle {a} line {i+1}: {v} vs {f:.2f}')
    total = 3 * ANGLES * LINES
    print(f'verified {total - bad}/{total} entries against trig(2*pi*a/48)*20480/y')
    return 1 if bad else 0


if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'extract':
        extract(Path(sys.argv[2]), Path(sys.argv[3]))
    elif len(sys.argv) >= 3 and sys.argv[1] == 'verify':
        sys.exit(verify(Path(sys.argv[2])))
    else:
        sys.exit(__doc__)
