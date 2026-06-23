"""Probe: battery SRAM persistence — power-cycle round-trip (luna v1.0.0).

Exercises `lib/include/snes/sram.h` end-to-end against the `save_game` example,
an axis the old harness had no way to test (it modelled no battery RAM).

save_game's main loop (exact-mask `padHeld`): A → write fixed values to slot 1,
B → load slot 1 into `vtl`. SaveState is `{s16 posX, posY; u16 camX, camY}`, so
KEY_A stores posX=9 posY=0xA camX=0x1234 camY=0x5678 → LE bytes 09000A0034127856.

Three runs prove real persistence, not ROM determinism:
  1. WRITE  — press A, capture `--srm-out`; assert the .srm holds the saved struct.
  2. READ   — fresh boot, `--srm-in` the saved battery, press B; assert `vtl` == it.
  3. CONTROL— fresh boot, NO `--srm-in`, press B; assert `vtl` != it (empty SRAM),
              so the READ match can only have come from the persisted file.
"""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path
from lib import find_luna, sym_of, assert_mem, capture_srm, rom_path, A, B

ROM = "memory/save_game/save_game.sfc"
STEPS = 2_500_000
SAVED = "09000A0034127856"        # slot-1 SaveState after KEY_A (8 bytes, LE)
PRESS_A = f"30:0x{A:04X}"         # hold A from frame 30 → triggers the save
PRESS_B = f"30:0x{B:04X}"         # hold B from frame 30 → triggers the load


def run() -> tuple[bool, str]:
    luna = find_luna()
    rom = rom_path(ROM)
    if not rom.is_file():
        return False, f"ROM missing ({rom})"
    _, vtl_off = sym_of(rom, "vtl")  # load buffer, bank $00 WRAM mirror

    with tempfile.TemporaryDirectory() as td:
        srm = Path(td) / "save_game.srm"

        # 1. WRITE — drive a save, persist the battery, check the file's slot 1.
        battery = capture_srm(luna, rom, STEPS, PRESS_A, srm)
        got = battery[:8].hex().upper()
        if got != SAVED:
            return False, f"WRITE: .srm slot1 = {got}, expected {SAVED}"

        # 2. READ — power-cycle: reload the battery, load slot 1, assert vtl.
        ok, det = assert_mem(luna, rom, STEPS, [(0x00, vtl_off, SAVED)],
                             input_script=PRESS_B, srm_in=srm)
        if not ok:
            return False, f"READ: persisted slot1 not loaded into vtl ({det})"

        # 3. CONTROL — no battery: load must NOT yield the saved values.
        ok_ctrl, _ = assert_mem(luna, rom, STEPS, [(0x00, vtl_off, SAVED)],
                                input_script=PRESS_B)
        if ok_ctrl:
            return False, "CONTROL: vtl matched saved values WITHOUT --srm-in — "\
                          "test is not actually proving persistence"

    return True, "SRAM round-trips across power cycle (write→srm→read; control negative)"


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "sram: " + msg)
    sys.exit(0 if ok else 1)
