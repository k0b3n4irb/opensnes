"""Probe: random RNG advance + simple_sprite OAM placement.

random: pressing A advances `current_value` (the PRNG output) — proves the RNG
is actually stepping under input. simple_sprite: oamSet(0,112,96,0x10,0,3,0)
must land the exact bytes in the OAM shadow at $7E:0300 (X,Y,tile,attr).
"""
from __future__ import annotations

import sys
from lib import find_luna, sym_of, peek_word, peek, rom_path, A


def _random(luna) -> tuple[bool, str]:
    rom = rom_path("basics/random/random.sfc")
    bank, off = sym_of(rom, "current_value")
    v0 = peek_word(luna, rom, 4_000_000, bank, off)
    v1 = peek_word(luna, rom, 5_500_000, bank, off, "60:0x80,64:0")   # pulse A once
    ok = v0 != v1
    return ok, f"random current_value {v0}→A→{v1}"


def _simple_sprite(luna) -> tuple[bool, str]:
    # oamSet(0, 112, 96, 0x10, 0, 3, 0) → OAM shadow $7E:0300 = X,Y,tile,attr.
    # oamSet stores y-1 ("dec a" in sprite_oamset.asm: compensate the +1 scanline
    # PPU pipeline delay), so the shadow Y is 95, not 96. (luna reads 95 — the
    # snes9x-era test's expected 96 ignored this y-1 convention.)
    rom = rom_path("graphics/sprites/simple_sprite/simple_sprite.sfc")
    bank, off = sym_of(rom, "oamMemory")  # 7E:0300
    x, y, tile, attr = peek(luna, rom, 4_000_000, bank, off, 4)
    ok = (x == 112 and y == 95 and tile == 0x10 and (attr & 0x3E) == 0x30)
    return ok, f"simple_sprite OAM0 x={x} y={y}(=96-1) tile=0x{tile:02X} attr=0x{attr:02X}"


def run() -> tuple[bool, str]:
    luna = find_luna()
    checks = [_random(luna), _simple_sprite(luna)]
    bad = [m for ok, m in checks if not ok]
    if bad:
        return False, "FAILED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in checks)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "sprites_random: " + msg)
    sys.exit(0 if ok else 1)
