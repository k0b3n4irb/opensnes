"""Probe: SNES Super Scope decode (luna v1.1.0 `--port2 superscope --superscope`).

The other half of the input gap luna < 1.1 couldn't reach: the input/superscope
example had boot+visual coverage only because no headless tool could pull a light-
gun trigger at a known screen position. luna v1.1.0 models the Super Scope, so we
fire at a known aim and assert the lib's decoded beam position in WRAM.

Aim at (100, 80) and pull the trigger. The lib latches the beam position into
scope_shothraw / scope_shotvraw:
  - vertical  == 80   (exact: the injected scanline)
  - horizontal == 124 (the injected X plus the Super Scope's ~+24 horizontal beam-
    latch offset — a hardware quirk luna models deterministically)
proving detection + trigger + beam-coordinate read end-to-end.
"""
from __future__ import annotations

import sys
from lib import find_luna, sym_of, assert_mem, word_bytes, rom_path

ROM = "input/superscope/superscope.sfc"
STEPS = 2_500_000
SCOPE_ON = ["--port2", "superscope"]
# aim+release+aim so a clean, debounced shot is latched.
FIRE_AT_100_80 = ["--superscope", "60:100,80,1;90:100,80,0;120:100,80,1"]
AIM_X_RAW = 124   # injected 100 + Super Scope horizontal latch offset
AIM_Y_RAW = 80    # injected 80, exact


def run() -> tuple[bool, str]:
    luna = find_luna()
    rom = rom_path(ROM)
    cb, co = sym_of(rom, "scope_con")        # connection flag (u8)
    hb, ho = sym_of(rom, "scope_shothraw")   # raw beam X (u16)
    _, vo = sym_of(rom, "scope_shotvraw")    # raw beam Y (u16)

    ok, det = assert_mem(luna, rom, STEPS, [(cb, co, "01")],
                         extra=SCOPE_ON + ["--superscope", "60:128,112,0"])
    if not ok:
        return False, f"Super Scope not detected (scope_con != 1): {det}"

    ok, det = assert_mem(luna, rom, STEPS,
                         [(hb, ho, word_bytes(AIM_X_RAW)), (hb, vo, word_bytes(AIM_Y_RAW))],
                         extra=SCOPE_ON + FIRE_AT_100_80)
    if not ok:
        return False, f"shot at (100,80) decoded wrong (raw h/v != {AIM_X_RAW}/{AIM_Y_RAW}): {det}"

    return True, f"Super Scope detected; shot at (100,80) → raw ({AIM_X_RAW},{AIM_Y_RAW})"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "superscope: " + msg)
    sys.exit(0 if ok else 1)
