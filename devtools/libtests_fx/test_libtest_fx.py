#!/usr/bin/env python3
"""Runtime assertions for the second library fixture (devtools/libtests_fx).

Same contract as devtools/libtests/test_libtest.py: build the ROM, run it in
luna, and compare result globals (by .sym name) and luna's machine view
against values measured once and explained in main.c. See the Makefile for
why this is a separate ROM (bank-$00 RAM, and SNESMOD vs the audio v2 driver).

Exit 0 = all pass, 1 = a vector failed.
"""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROM = HERE / "libtest_fx.sfc"
sys.path.insert(0, str(HERE.parents[1] / "testing"))
from lib import find_luna, assert_mem  # noqa: E402

# snesmodInit uploads the driver and the module before anything else runs;
# r_done is set around frame 170.
STEPS = 8_000_000

# (global, width-bytes, expected-value)
CASES = [
    ("r_done",       2, 0xBEEF),
    # hdma: the wave helper enables its channel, the table helpers do not
    ("r_hdma_init",  2, 0x0000),
    ("r_hdma_wave",  2, 0x0040),
    ("hdma_wave_amplitude", 1, 60),   # hdmaWaveH(…, 200, …): clamped, as the header always said
    ("r_hdma_setup", 2, 0x0040),
    ("r_hdma_both",  2, 0x0070),
    ("r_hdma_chan",  2, 1),      # D1 at 1.0: hdmaEnable(4) / hdmaDisable(4) by channel; 8, 0x10, 0x40, 0xFF refused
    ("r_hdma_speed", 2, 3),      # hdmaWaveSetSpeed: a lib function since 2026-10-03 (was inline)
    # nmiSet: one callback per frame, none after nmiClear
    ("r_nmi_calls",  2, 5),
    ("r_chips",      2, 0),      # no SA-1, no GSU on this LoROM
    ("r_nmi_after",  2, 5),
    # mode7Rotate(90) -> angle 63 -> sine table entry 126
    ("m7_sin",       1, 126),
    # snesmod: the 24-bit table pointer (bank byte included), a clean u16
    # position, an emptied queue whose last command was the volume (0x5A)
    ("r_mod_flush",  2, 1),
    ("r_irq_mod",    2, 10),     # T5: one V-timer IRQ per frame while snesmodProcess runs (armed before snesmodInit)
    ("r_mod_latch",  2, 0),      # ...and the driver leaves STAT78's counter-latch flag clear (the Super Scope's trigger)
    # last command sent after the flush: CMD_FADE (6), 0, speed 3, target 45.
    # Asymmetric on purpose — the target used to be read from the wrong byte.
    ("spc_pr",       4, 0x2D030006),
    ("r_mod_pos",    2, 0),
]

# luna's view of the machine. Dotted keys step into the state JSON.
STATE_CASES = [
    ("cpu_regs.nmitimen", 0xA1),     # NMI + V-timer IRQ + auto-joypad: the IRQ stays armed through the driver's $4200 writes
    ("cpu_regs.vtime", 120),         # irqSetVTimer(120), untouched by SNESMOD
    ("ppu.m7a", 256), ("ppu.m7b", 32), ("ppu.m7c", -32), ("ppu.m7d", 128),   # mode7SetMatrix
    ("ppu.m7x", 64), ("ppu.m7y", 48),                                         # mode7SetCenter
    # hdmaColorGradient(3, 37, red, blue): the index is honoured. CGRAM[37]
    # holds whichever band the beam last crossed when luna samples, so the
    # exact colour depends on the sampling phase — "not black" is the claim.
    ("ppu.cgram.37", ("ne", 0)),
    ("ppu.cgram.0", 0x0000),         # ...and colour 0, where every gradient used to land, is untouched
    ("dma.channels.2.bbad", 0x32),   # hdmaSetup on channel 2: COLDATA, and...
    ("dma.channels.2.a_bank", ("ne", 0)),   # ...the hand-passed bank of a const table (an asset bank)
    ("dma.channels.4.bbad", 0x26),   # hdmaWindowShape -> WH0, two registers
    ("dma.channels.4.params", 0x01),
    ("dma.channels.5.bbad", 0x32),   # hdmaGradient -> COLDATA, one register
    ("dma.channels.5.params", 0x00),
    ("dma.channels.6.bbad", 0x0D),   # hdmaWaveH(bg 0) -> BG1HOFS, written twice
    ("dma.channels.6.params", 0x02),
]


def walk(state: dict, path: str):
    got = state
    for step in path.split("."):
        try:
            got = got[int(step)] if isinstance(got, list) else got.get(step)
        except (IndexError, ValueError, AttributeError):
            return None
    return got


def main() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make -C devtools/libtests_fx` first)")
    luna = find_luna()
    fails = 0
    for name, width, want in CASES:
        hexval = "".join(f"{(want >> (8 * i)) & 0xFF:02X}" for i in range(width))
        ok, detail = assert_mem(luna, ROM, STEPS, [(name, hexval)])
        print(f"  {'PASS' if ok else 'FAIL'}  {name} == 0x{want:0{width * 2}X}" + ("" if ok else f"  [{detail}]"))
        fails += 0 if ok else 1
    proc = subprocess.run([luna, "state", "-n", str(STEPS), "--out", "-",
                           str(ROM)],
                          capture_output=True, text=True, timeout=300, check=True)
    state = json.loads(proc.stdout)
    for path, want in STATE_CASES:
        got = walk(state, path)
        if isinstance(want, tuple):                 # ("ne", value)
            ok = got is not None and got != want[1]
        else:
            ok = got == want
        print(f"  {'PASS' if ok else 'FAIL'}  {path} == {want}" + ("" if ok else f"  [luna reports {got}]"))
        fails += 0 if ok else 1
    # the module must still be playing
    active = walk(state, "apu.active_voices")
    ok = isinstance(active, int) and active > 0
    print(f"  {'PASS' if ok else 'FAIL'}  apu.active_voices > 0" + ("" if ok else f"  [luna reports {active}]"))
    fails += 0 if ok else 1
    total = len(CASES) + len(STATE_CASES) + 2
    print(f"\nLib runtime assertions (fx fixture): {total - fails}/{total} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
