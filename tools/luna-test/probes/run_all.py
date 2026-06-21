"""Run every functional probe in probes/ and report pass/fail.

Each probe is a standalone module exposing `run() -> (ok, msg)` (and runnable
directly). Mouse/Super Scope probes are intentionally absent (gap G4 — luna
doesn't model those devices; their examples get boot+visual coverage only).
"""
from __future__ import annotations

import importlib
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

SKIP = {"lib", "run_all"}


def main() -> int:
    mods = sorted(p.stem for p in HERE.glob("*.py") if p.stem not in SKIP)
    failures = 0
    for name in mods:
        try:
            ok, msg = importlib.import_module(name).run()
        except Exception as e:  # noqa: BLE001
            ok, msg = False, f"error: {e}"
        print(("  PASS " if ok else "  FAIL ") + f"{name}: {msg}")
        failures += 0 if ok else 1
    print(f"\nProbes: {len(mods) - failures}/{len(mods)} passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
