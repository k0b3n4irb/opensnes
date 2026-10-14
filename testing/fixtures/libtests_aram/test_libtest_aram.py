#!/usr/bin/env python3
"""Runtime assertions for the sound-memory fixture (testing/fixtures/libtests_aram).

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
ROM = HERE / "libtest_aram.sfc"
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
    # a sample loaded in steps
    ("s_begin", 0), ("s_free_begin", 0x7818), ("s_begin_twice", 2), ("s_play_early", 0xFF),
    ("s_step1", 937), ("s_other", 0), ("s_other_addr", 0x4EE8), ("s_steps", 16),
    ("s_addr", 0x4B00), ("s_play", 2), ("s_idle", 0),
    ("s_odd1", 4), ("s_odd2", 0),
    ("s_cancel", 0), ("s_free_cancel", 1), ("s_begin_after", 0),
    # the ring given back
    ("e_rel", 0), ("e_free_rel", 0xB0C5), ("e_load", 0), ("e_free_full", 0),
    ("e_enable", 0), ("e_set_busy", 1), ("e_set_again", 0), ("e_free_again", 0x2805),
]


def main() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make -C testing/fixtures/libtests_aram` first)")
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
    # The sample sent in sixteen steps is in the sound CPU's memory byte for
    # byte, its directory entry (slot 10, at $0A28) names its first byte and
    # not its last step's, the sample loaded between two steps sits above it,
    # the ten-byte one after, and the voice command sent between two steps
    # reached the DSP (V1VOLL / V1VOLR, read from the driver's last write).
    pat = bytes(((n * 7 + 3) & 255) | 1 for n in range(1000))
    checks = [
        ("the 1000 bytes sent in steps, at $4B00", aram[0x4B00:0x4B00 + 1000] == pat),
        ("slot 10's directory entry: start and loop $4B00", aram[0x0A28:0x0A2C] == bytes([0x00, 0x4B, 0x00, 0x4B])),
        ("the sample loaded between two steps, at $4EE8", aram[0x4EE8:0x4EE8 + 9] == b"\x5A" * 9),
        ("the ten bytes whose last step took the odd one, at $4EF1", aram[0x4EF1:0x4EF1 + 10] == pat[:10]),
        ("slot 12's directory entry: $4EF1", aram[0x0A30:0x0A34] == bytes([0xF1, 0x4E, 0xF1, 0x4E])),
        # Loaded where the released ring was ($C700-$FEFF), then echo routed
        # to every voice for twenty frames: a ring still written, or cleared
        # again by audioEnableEcho(), leaves zeros here. The new ring of
        # delay 1 ($F700 up) is cleared by its own setup, so stop under it.
        ("the samples loaded over the released ring, $4EFB-$F6FF", all(b == 0x5A for b in aram[0x4EFB:0xF700])),
    ]
    for label, ok in checks:
        print(f"  {'PASS' if ok else 'FAIL'}  {label}")
        fails += 0 if ok else 1
    total = len(CASES) + 1 + len(checks)
    print(f"\nLib runtime assertions (aram fixture): {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
