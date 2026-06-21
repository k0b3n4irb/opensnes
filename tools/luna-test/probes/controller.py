"""Probe: controller button decode (ported from controller.test.mjs).

Injects a single button mask, lets the NMI auto-joypad read decode it, and
asserts `pad_keys[0]` (port-0 controller word) equals the injected mask. Uses
luna v0.3.0's `--assert` to delegate the WRAM equality + verdict (feature L2).
"""
from __future__ import annotations

import sys
from lib import (find_luna, sym_of, assert_mem, word_bytes, rom_path,
                 B, START, UP, RIGHT, A, L)

# Representative subset across high and low joypad bits (one luna run each).
KEYS = [("B", B), ("START", START), ("UP", UP), ("RIGHT", RIGHT), ("A", A), ("L", L)]


def run() -> tuple[bool, str]:
    luna = find_luna()
    rom = rom_path("input/controller/controller.sfc")
    bank, off = sym_of(rom, "pad_keys")  # u16[8], index 0 = port 0
    for name, mask in KEYS:
        # Hold the mask from frame 30 (no release) so it is latched when asserted.
        ok, detail = assert_mem(luna, rom, 2_500_000,
                                [(bank, off, word_bytes(mask))], f"30:0x{mask:04X}")
        if not ok:
            return False, f"{name}: {detail}"
    return True, f"{len(KEYS)} buttons decode correctly (via luna --assert)"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "controller: " + msg)
    sys.exit(0 if ok else 1)
