#!/usr/bin/env python3
"""Runtime assertions for the DSP-1 library fixture (devtools/libtests_dsp1).

Firmware-gated: the DSP-1 needs the user-supplied `dsp1b.rom` dump in luna's
firmware folder (copyrighted, never in CI). Absent -> SKIP, exit 0, exactly
like the firmware-gated manifests. Present -> the vectors must pass.
"""
from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROM = HERE / "libtest_dsp1.sfc"
sys.path.insert(0, str(HERE.parents[1] / "tools" / "luna-test" / "probes"))
sys.path.insert(0, str(HERE.parents[1] / "tools" / "luna-test"))
from lib import find_luna, assert_mem  # noqa: E402
from luna_runner import firmware_dir  # noqa: E402

STEPS = 2_000_000

CASES = [
    ("r_done",      2, 0xBEEF), ("dsp1_ok",     2, 1), ("dsp1_ok_old", 2, 1),
    ("r_mul",       2, 0x2000), ("r_mul_neg",   2, 0xE000), ("r_mul_sign", 2, 1),
    # Distance reads one low on exact lengths (measured, see dsp1.h)
    ("r_dist",      2, 12),     ("r_dist_mid",  2, 499),    ("r_dist_big", 2, 9999),
    # Range = ((x2+y2+z2) - r2) >> 15, arithmetic
    ("r_range_out", 2, 732),    ("r_range_in",  2, 0xFFE1), ("r_range_on", 2, 0), ("r_range_sm", 2, 0),
    ("r_rot_x",     2, 0),      ("r_rot_y",     2, 0xFF9D),   # -99: sin 90 deg = 0x7FFF
    ("r_tgt_x",     2, 1),      ("r_tgt_y",     2, 1),
]


def main() -> int:
    fw = firmware_dir() / "dsp1b.rom"
    if not fw.is_file() or fw.stat().st_size != 8192:
        print(f"  SKIP  DSP-1 fixture: no 8192-byte dsp1b.rom in {firmware_dir()}")
        return 0
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make -C devtools/libtests_dsp1` first)")
    luna = find_luna()
    fails = 0
    for name, width, want in CASES:
        hexval = "".join(f"{(want >> (8 * i)) & 0xFF:02X}" for i in range(width))
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, hexval)])
        print(f"  {'PASS' if ok else 'FAIL'}  {name} == 0x{want:0{width * 2}X}" + ("" if ok else f"  [{detail}]"))
        fails += 0 if ok else 1
    print(f"\nLib runtime assertions (dsp1 fixture): {len(CASES) - fails}/{len(CASES)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
