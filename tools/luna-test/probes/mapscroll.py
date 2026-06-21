"""Probe: mapscroll D-pad camera-follow (ported from mapscroll.test.mjs).

Proves the map-engine input→state link: D-pad RIGHT increments `xloc`
(the world cursor mapUpdateCamera follows), LEFT decrements it. If xloc
doesn't move under D-pad, the input→C link or the map engine is broken.
"""
from __future__ import annotations

import sys
from lib import find_luna, load_symbols, peek_word, rom_path, LEFT, RIGHT


def run() -> tuple[bool, str]:
    luna = find_luna()
    rom = rom_path("maps/mapscroll/mapscroll.sfc")
    bank, off = load_symbols(rom)["xloc"]

    # Independent deterministic runs from boot; input frames kept low so a
    # generous -n always passes them. Reads happen at -n (well after release).
    x_start = peek_word(luna, rom, 4_000_000, bank, off)                       # no input
    x_right = peek_word(luna, rom, 6_000_000, bank, off, "40:0x100,120:0")     # hold RIGHT 40-119
    x_left  = peek_word(luna, rom, 8_000_000, bank, off,
                        "40:0x100,120:0,140:0x200,210:0")                       # then hold LEFT 140-209

    if not x_right > x_start:
        return False, f"RIGHT did not advance xloc ({x_start} → {x_right})"
    if not x_left < x_right:
        return False, f"LEFT did not retract xloc ({x_right} → {x_left})"
    return True, f"xloc {x_start} →RIGHT {x_right} →LEFT {x_left}"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "mapscroll: " + msg)
    sys.exit(0 if ok else 1)
