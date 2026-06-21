"""Probe: D-pad → world-coordinate movement (ported from aim_target /
collision_demo / tiled / mode7_perspective / likemario .test.mjs).

For each example: snapshot a signed coordinate with no input, then hold a
direction and assert the coordinate moved the expected way — proving the
input → C-state link and that example's movement logic.
"""
from __future__ import annotations

import sys
from lib import (find_luna, sym_of, peek_sword, rom_path,
                 LEFT, RIGHT, UP, DOWN)

# (example rom, [(symbol, dir_mask, want_increase)])
CASES = [
    ("basics/aim_target/aim_target.sfc",
     [("target_x", RIGHT, True), ("target_y", UP, False)]),
    ("basics/collision_demo/collision_demo.sfc",
     [("player_x", RIGHT, True), ("player_y", DOWN, True)]),
    ("maps/tiled/tiled.sfc",
     [("mapscx", RIGHT, True), ("mapscx", LEFT, False)]),
    ("graphics/backgrounds/mode7_perspective/mode7_perspective.sfc",
     [("sx", RIGHT, True), ("sy", DOWN, True)]),
    ("games/likemario/likemario.sfc",
     [("mario_x", RIGHT, True)]),
]


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for rel, checks in CASES:
        rom = rom_path(rel)
        name = rom.parent.name
        for symname, mask, want_inc in checks:
            bank, off = sym_of(rom, symname)
            base = peek_sword(luna, rom, 4_000_000, bank, off)
            held = peek_sword(luna, rom, 6_000_000, bank, off, f"40:0x{mask:04X}")
            moved_up = held > base
            ok = (moved_up == want_inc)
            arrow = "↑" if want_inc else "↓"
            results.append((ok, f"{name}.{symname}{arrow} {base}→{held}"))
    bad = [m for ok, m in results if not ok]
    msg = f"{len(results) - len(bad)}/{len(results)} coords moved correctly"
    if bad:
        return False, msg + " | FAILED: " + "; ".join(bad)
    return True, msg


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "movement: " + msg)
    sys.exit(0 if ok else 1)
