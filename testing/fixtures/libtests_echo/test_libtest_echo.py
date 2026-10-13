#!/usr/bin/env python3
"""Runtime assertions for the echo-ring fixture (testing/fixtures/libtests_echo).

The ring takes the top delay x 2 KB of the sound CPU's memory: refused when
the samples loaded reach into it (nothing changed), accepted when they do
not, and a shorter delay gives the difference back. The driver answers
after a long ring was replaced by a short one, and the sample still loaded
is whole (main.c says why that is the dangerous case).
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROM = HERE / "libtest_echo.sfc"
sys.path.insert(0, str(HERE.parents[2] / "testing"))
from lib import find_luna, assert_mem  # noqa: E402

STEPS = 12_000_000

CASES = [
    ("r_done", 0xBEEF), ("r_init", 0), ("r_load", 0),
    ("r_free_full", 0x34C0),
    ("r_refuse7", 1), ("r_free_refused", 0x34C0),
    ("r_ok6", 0), ("r_free6", 0x0400),
    ("r_ok1", 0), ("r_free1", 0xAC00), ("r_alive", 0),
    ("r_ok7", 0), ("r_free7", 0x7C00),
]


def main() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make -C testing/fixtures/libtests_echo` first)")
    luna = find_luna()
    fails = 0
    for name, want in CASES:
        hexval = f"{want & 0xFF:02X}{want >> 8:02X}"
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, hexval)])
        print(f"  {'PASS' if ok else 'FAIL'}  {name} == 0x{want:04X}" + ("" if ok else f"  [{detail}]"))
        fails += 0 if ok else 1
    # The sample that stays loaded ($0B00-$4AFF, no byte of it 0) is whole.
    # Without the driver's wait, the 12 KB ring that the 2 KB one replaces
    # keeps its length from the new start, passes $FFFF and writes zeros over
    # $0000 and up for the rest of its turn (measured on luna: 736 bytes of
    # this sample, $2420-$26FF; where it lands depends on the moment).
    with tempfile.TemporaryDirectory() as td:
        dump = Path(td) / "aram.bin"
        subprocess.run([str(luna), "state", "-n", str(STEPS), "--dump-aram", str(dump), "--out", "-", str(ROM)],
                       capture_output=True, check=True)
        aram = dump.read_bytes()
    bad = [a for a in range(0x0B00, 0x4B00) if aram[a] != 0x5A]
    ok = not bad
    print(f"  {'PASS' if ok else 'FAIL'}  sample 0 untouched by the ring it does not hold"
          + ("" if ok else f"  [{len(bad)} bytes written over, ${bad[0]:04X}-${bad[-1]:04X}]"))
    fails += 0 if ok else 1
    total = len(CASES) + 1
    print(f"\nLib runtime assertions (echo fixture): {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
