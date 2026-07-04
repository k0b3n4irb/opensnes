"""Run every functional probe in probes/ and report pass/fail.

Each probe is a standalone module exposing `run() -> (ok, msg)` (and runnable
directly). Probes auto-register: drop a new `*.py` with a `run()` here.
Mouse and Super Scope ARE probed (mouse.py, superscope.py) since luna v1.1.0
added device modelling — the old "gap G4" no longer exists.
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
