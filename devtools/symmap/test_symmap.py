#!/usr/bin/env python3
"""Regression test for symmap.py's WRAM-overlap detector.

`symmap.py --check-overlap` is the static analysis that catches bank $00 ↔ bank
$7E WRAM-mirror collisions (a silent-corruption class). This test pins its two
verdicts against committed `.sym` fixtures so the detector can't silently rot:

  - fixtures/broken/wram_overlap.sym — intentional collision → must FAIL (rc 1)
  - fixtures/clean/hello_world.sym   — known-good           → must PASS (rc 0)

These fixtures were re-homed from the removed `tools/debug-fixtures/` (which the
old opensnes-emu test runner consumed); this test is their live consumer now.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SYMMAP = HERE / "symmap.py"
FIXTURES = HERE / "fixtures"


def _check(sym: Path, flag: str = "--check-overlap") -> int:
    return subprocess.run(
        [sys.executable, str(SYMMAP), flag, "--no-color", str(sym)],
        capture_output=True, text=True,
    ).returncode


def main() -> int:
    cases = [
        ("broken/wram_overlap.sym", "--check-overlap", 1,
         "overlap must be detected"),
        ("clean/hello_world.sym", "--check-overlap", 0,
         "clean map must pass"),
        # RAM band budget (defect B2): a bank-$00 RAMSECTION crossing or
        # placed past $2000 is silently wrong-banked by C code — must fail.
        ("broken/ram_band_overflow.sym", "--check-ram-budget", 1,
         "RAM past $2000 must be detected"),
        ("clean/hello_world.sym", "--check-ram-budget", 0,
         "in-band RAM must pass"),
    ]
    failures = 0
    for rel, flag, want, desc in cases:
        sym = FIXTURES / rel
        if not sym.is_file():
            print(f"  FAIL  {rel}: fixture missing")
            failures += 1
            continue
        got = _check(sym, flag)
        if got == want:
            print(f"  PASS  {rel} [{flag}] (rc={got}) — {desc}")
        else:
            print(f"  FAIL  {rel} [{flag}]: rc={got}, expected {want} — {desc}")
            failures += 1

    if failures:
        print(f"\nsymmap regression: {failures} failure(s)")
        return 1
    print(f"\nsymmap regression: {len(cases)}/{len(cases)} ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
