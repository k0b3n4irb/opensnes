"""Probe: SNES Mouse input decode (luna v1.1.0 `--port1 mouse --mouse`).

Closes a coverage gap the old harness (and luna < 1.1) could not reach: the
input/mouse example previously got boot+visual only because no headless tool
could inject mouse motion. luna v1.1.0 models the serial Mouse, so we drive it
and assert the decoded absolute cursor position in WRAM.

The example accumulates per-frame deltas into pos_x/pos_y (init 128/112) and
clamps to [0,255] / [0,223]. luna latches the injected delta and re-applies it
every frame, so a sustained push SATURATES to the clamp — a value independent of
the mouse sensitivity setting, hence a robust deterministic assertion.
"""
from __future__ import annotations

import sys
from lib import find_luna, sym_of, assert_mem, word_bytes, rom_path

ROM = "input/mouse/mouse.sfc"
STEPS = 2_500_000
MOUSE_ON = ["--port1", "mouse"]


def run() -> tuple[bool, str]:
    luna = find_luna()
    rom = rom_path(ROM)
    cb, co = sym_of(rom, "mouse_con")   # connection flag (u8)
    xb, xo = sym_of(rom, "pos_x")       # cursor X (s16), init 128, clamp 0..255
    _, yo = sym_of(rom, "pos_y")        # cursor Y (s16), init 112, clamp 0..223

    # 1. The Mouse must be detected on port 1.
    ok, det = assert_mem(luna, rom, STEPS, [(cb, co, "01")], extra=MOUSE_ON + ["--mouse", "30:0,0,0"])
    if not ok:
        return False, f"mouse not detected (mouse_con != 1): {det}"

    # 2. Sustained right+down saturates the cursor to the bottom-right clamp.
    ok, det = assert_mem(luna, rom, STEPS,
                         [(xb, xo, word_bytes(255)), (xb, yo, word_bytes(223))],
                         extra=MOUSE_ON + ["--mouse", "30:20,20,0"])
    if not ok:
        return False, f"right/down motion did not saturate to (255,223): {det}"

    # 3. Sustained left+up saturates to the top-left clamp (proves sign handling).
    ok, det = assert_mem(luna, rom, STEPS,
                         [(xb, xo, word_bytes(0)), (xb, yo, word_bytes(0))],
                         extra=MOUSE_ON + ["--mouse", "30:-20,-20,0"])
    if not ok:
        return False, f"left/up motion did not saturate to (0,0): {det}"

    return True, "mouse detected; cursor saturates (255,223) right/down and (0,0) left/up"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "mouse: " + msg)
    sys.exit(0 if ok else 1)
