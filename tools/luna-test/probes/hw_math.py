"""Probe: hardware math units — regression lock from the luna stress campaign.

Two dedicated micro-ROMs exercise the SNES math hardware and store their
results in WRAM:

  - `hwmath`  — CPU multiply/divide ($4202/$4203 → $4216; $4204-6 → $4214/$4216),
                including the divide-by-zero quirk (quotient=$FFFF, remainder=dividend).
  - `ppumul`  — PPU Mode 7 signed multiply ($211B M7A × $211C M7B → 24-bit at
                $2134/$2135/$2136), signed operands and signed 24-bit result.

Every value below was verified **bit-exact against Mesen2 and the fullsnes /
anomie reference** during the Wave-1 luna stress campaign, so this probe locks
them as a luna-only regression: `luna state --assert <symbol>=<hex>`. If luna's
math unit ever drifts, the assert fails. (Mesen2 was the one-time validation
oracle; the standing check is luna-only, per .claude/rules/luna_tooling.md.)

Sources live under tools/luna-test/stress/{hwmath,ppumul}/; this probe builds
them on demand and asserts the known-correct result block.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from lib import find_luna, assert_mem

REPO = Path(__file__).resolve().parents[3]
STRESS = REPO / "tools" / "luna-test" / "stress"

# (dir, rom, symbol, expected-little-endian-hex, steps)
CASES = [
    ("hwmath", "hwmath.sfc", "results",
     "000001FE409CE9079405040001010000FFFFE80300000000FFFF00000000DEC0", 500_000),
    ("ppumul", "ppumul.sfc", "res",
     "88130078ECFFD4FEFF2C0100817F3F000040000000010000DEC00000", 500_000),
]


def _build(d: Path) -> None:
    subprocess.run(["make", "-s"], cwd=d, check=True,
                   capture_output=True, text=True, timeout=300)


def run() -> "tuple[bool, str]":
    luna = find_luna()
    parts = []
    all_ok = True
    for d, rom, sym, exp, steps in CASES:
        dd = STRESS / d
        try:
            _build(dd)
        except subprocess.CalledProcessError as e:
            all_ok = False
            parts.append(f"{d}: BUILD FAIL ({(e.stderr or e.stdout or '')[:80]})")
            continue
        ok, detail = assert_mem(luna, dd / rom, steps, [(sym, exp)])
        all_ok = all_ok and ok
        parts.append(f"{d}: {'ok' if ok else 'MISMATCH — ' + detail}")
    return all_ok, "hw math — " + "; ".join(parts)


if __name__ == "__main__":
    ok, msg = run()
    print(msg)
    sys.exit(0 if ok else 1)
