"""Probe: DSP-1 coprocessor execution (firmware-gated).

The Super FX / SA-1 coprocessor-liveness checks moved to native `luna test`
manifests (`tools/luna-test/manifests/coproc_*.toml`, `[asserts.trace]` +
the sa1_hello handshake via `[asserts.values]`). DSP-1 stays here because luna
LLE-emulates it and needs Sony's `dsp1b.rom` (copyright, absent in CI) — a
manifest can't express the firmware skip — so this probe gates on
`missing_firmware()` and asserts `dsp1.instructions_executed >= 1` when the
firmware is present, SKIPping cleanly otherwise.
"""
from __future__ import annotations

import sys
from lib import find_luna, rom_path, dsp1_instructions
from luna_runner import missing_firmware, load_manifest

STEPS = 1_000_000


def run() -> "tuple[bool, str]":
    luna = find_luna()
    dsp_key = "chips/dsp1_cube"
    fw = missing_firmware(dsp_key, load_manifest())
    if fw:
        return True, f"dsp1_cube SKIP (no {fw})"
    rom = rom_path(f"{dsp_key}/dsp1_cube.sfc")
    if not rom.is_file():
        return False, f"dsp1_cube: ROM missing ({rom})"
    n = dsp1_instructions(luna, rom, STEPS)
    if n < 1:
        return False, f"dsp1_cube: {n} DSP-1 instructions (< 1) — not running"
    return True, f"dsp1_cube: {n} DSP-1 instructions"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "coproc_dsp1: " + msg)
    sys.exit(0 if ok else 1)
