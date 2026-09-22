#!/usr/bin/env python3
"""Runtime assertions for the HiROM library fixture (devtools/libtests_hirom).

Two things no LoROM fixture can see: the sram module's HiROM mapping
($30:6000, not $70:0000 — fullsnes, "SNES Memory Map / Battery-backed SRAM"),
and the bank byte the toolchain gives a pointer to a RAM variable under
.BASE $C0 (it must be $00; it was $C0, a ROM bank, until the wlalink fix of
2026-09-20).
"""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROM = HERE / "libtest_hirom.sfc"
sys.path.insert(0, str(HERE.parents[1] / "tools" / "luna-test" / "probes"))
from lib import find_luna  # noqa: E402

STEPS = 1_500_000

# symbol or BANK:OFFSET:COUNT -> expected bytes (hex, as luna prints them)
PEEKS = [
    ("r_done:2",     "efbe"),
    ("r_bank_ram:2", "0000"),   # a pointer to bank-$00 RAM carries bank $00 (was $C0: ROM on HiROM)
    ("r_bank_rom:2", "c700"),   # ...and a const table its real HiROM bank
    ("r_rt:2",       "0c00"),   # 12 bytes saved from ROM and loaded back into RAM
    ("r_off:2",      "1500"),   # sramSaveOffset / sramLoadOffset at 0x123
    ("r_off3:2",     "4800"),
    ("r_ck:2",       "8c00"),   # sramChecksum over the const template
    ("r_clear:2",    "0000"),   # sramClear zeroed what sramLoad then read
    ("r_ok:2",       "0000"),   # SRAM_OK
    ("r_range:2",    "0100"),   # SRAM_ERR_RANGE: past the 8 KB HiROM window, nothing written
    # where the HARDWARE puts it: HiROM battery RAM is $30:6000 + offset
    ("30:6000:C",    "c1d2e3f415263748596a7b8c"),
    ("30:6123:4",    "15263748"),
]


def main() -> int:
    if not ROM.is_file():
        sys.exit(f"ROM missing: {ROM} (run `make -C devtools/libtests_hirom` first)")
    luna = find_luna()
    cmd = [luna, "state", "-n", str(STEPS), "--out", "-"]
    for spec, _ in PEEKS:
        cmd += ["--peek", spec]
    proc = subprocess.run(cmd + [str(ROM)], capture_output=True, text=True, timeout=300, check=True)
    got = {p["spec"]: p.get("bytes_hex", "") for p in json.loads(proc.stdout).get("peeks", [])}
    fails = 0
    for spec, want in PEEKS:
        ok = got.get(spec) == want
        print(f"  {'PASS' if ok else 'FAIL'}  {spec} == {want}" + ("" if ok else f"  [luna reports {got.get(spec)!r}]"))
        fails += 0 if ok else 1
    print(f"\nLib runtime assertions (hirom fixture): {len(PEEKS) - fails}/{len(PEEKS)} ok")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
