"""The pinned luna binary and the repository it serves — the one place that
knows where things are (testing/lib, 2026-10-05).

Every harness script and every fixture test imports these from here; before,
`luna_runner.py` carried them and fourteen files reached it through their
own `sys.path` arithmetic. A script that needs luna does:

    sys.path.insert(0, str(<repo>/"testing"))
    from lib import find_luna, LUNA_VERSION, REPO_ROOT
"""
from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path

TESTING = Path(__file__).resolve().parent.parent     # testing/
REPO_ROOT = TESTING.parent
# Single source of truth for the pin: testing/luna.version (what
# scripts/install-luna.sh downloads). A version bump touches that one file.
LUNA_VERSION = (TESTING / "luna.version").read_text(encoding="utf-8").strip()


def find_luna() -> str:
    """$LUNA_BIN, then testing/bin/luna (where install-luna.sh puts it), then
    `luna` on PATH. Exits with the install hint when none is there."""
    env = os.environ.get("LUNA_BIN")
    if env and Path(env).is_file():
        return env
    for name in ("luna", "luna.exe"):   # .exe on Windows
        installed = TESTING / "bin" / name
        if installed.is_file():
            return str(installed)
    on_path = shutil.which("luna")
    if on_path:
        return on_path
    sys.exit(
        "ERROR: luna binary not found. Run scripts/install-luna.sh, set $LUNA_BIN, "
        f"or put `luna` on PATH. Expected luna {LUNA_VERSION}."
    )


def firmware_dir() -> Path:
    """luna's coprocessor-firmware folder (where dsp1b.rom lives)."""
    base = os.environ.get("XDG_CONFIG_HOME")
    root = Path(base) if base else Path.home() / ".config"
    return root / "luna" / "firmware"
