"""Probe: open-bus / MDR read behaviour — regression lock from the campaign.

A C pointer dereference on this target always addresses bank $00, so the
open-bus MDR can only be exercised from asm. `stress/openbus` reads the
$2100 MMIO mirror through several banks with explicit `lda.l bb:2100`; the
open bus then returns the bank byte `bb` (the last operand byte = the MDR):

    res = 3F 01 20 10 00 00 | 00×6 | DE C0 00 00
          ^^ ^^ ^^ ^^ ^^ ^^
          |  |  |  |  |  +-- lda.l $3F:2134 -> real MPYL (readable, not open bus)
          |  |  |  |  +----- control lda.l $00:2100 -> $00
          |  |  |  +-------- lda.l $10:21FF (unmapped) -> $10
          |  |  +----------- lda.l $20:2133 (write-only) -> $20
          |  +-------------- lda.l $01:2100 -> $01
          +----------------- lda.l $3F:2100 -> $3F

Verified bit-exact vs Mesen2 and the fullsnes/anomie open-bus rules during
the Wave-2 luna stress campaign; locked here luna-only via `luna --assert`.
Deterministic (pure function of the instruction stream), so cross-arch stable.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from lib import find_luna, assert_mem

REPO = Path(__file__).resolve().parents[3]
STRESS = REPO / "tools" / "luna-test" / "stress"

DIR = STRESS / "openbus"
ROM = "openbus.sfc"
SYMBOL = "res"
EXPECTED = "3F0120100000000000000000DEC00000"
STEPS = 500_000


def run() -> "tuple[bool, str]":
    luna = find_luna()
    try:
        subprocess.run(["make", "-s"], cwd=DIR, check=True,
                       capture_output=True, text=True, timeout=300)
    except subprocess.CalledProcessError as e:
        return False, f"open-bus: BUILD FAIL ({(e.stderr or e.stdout or '')[:80]})"
    ok, detail = assert_mem(luna, DIR / ROM, STEPS, [(SYMBOL, EXPECTED)])
    return ok, f"open-bus MDR — {'ok' if ok else 'MISMATCH — ' + detail}"


if __name__ == "__main__":
    ok, msg = run()
    print(msg)
    sys.exit(0 if ok else 1)
