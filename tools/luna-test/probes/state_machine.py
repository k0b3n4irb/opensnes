"""Probe: button-driven state machines (ported from dynamic_map / scene_stack).

dynamic_map: A toggles is_map32x32 (1→0→1). scene_stack: START pushes a scene
(scene_top increments) then pops it back. Each press is edge-detected, so we
pulse the button (hold a few frames, release) between reads.
"""
from __future__ import annotations

import sys
from lib import find_luna, sym_of, peek_byte, rom_path, A, START


def _dynamic_map(luna) -> tuple[bool, str]:
    rom = rom_path("maps/dynamic_map/dynamic_map.sfc")
    bank, off = sym_of(rom, "is_map32x32")
    s0 = peek_byte(luna, rom, 5_000_000, bank, off)                       # boot
    s1 = peek_byte(luna, rom, 6_000_000, bank, off, "60:0x80,64:0")       # 1 pulse A
    s2 = peek_byte(luna, rom, 7_000_000, bank, off, "60:0x80,64:0,120:0x80,124:0")  # 2 pulses
    ok = (s1 != s0) and (s2 == s0)
    return ok, f"dynamic_map is_map32x32 {s0}→{s1}→{s2}"


def _scene_stack(luna) -> tuple[bool, str]:
    rom = rom_path("basics/scene_stack/scene_stack.sfc")
    bank, off = sym_of(rom, "scene_top")
    t0 = peek_byte(luna, rom, 4_000_000, bank, off)                       # boot
    t1 = peek_byte(luna, rom, 5_500_000, bank, off, "60:0x1000,64:0")     # START → push
    t2 = peek_byte(luna, rom, 7_000_000, bank, off,
                   "60:0x1000,64:0,120:0x1000,124:0")                      # START → pop
    ok = (t1 != t0) and (t2 == t0)
    return ok, f"scene_stack scene_top {t0}→push {t1}→pop {t2}"


def run() -> tuple[bool, str]:
    luna = find_luna()
    checks = [_dynamic_map(luna), _scene_stack(luna)]
    bad = [m for ok, m in checks if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in checks)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "state_machine: " + msg)
    sys.exit(0 if ok else 1)
