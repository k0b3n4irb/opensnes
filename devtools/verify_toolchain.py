#!/usr/bin/env python3
"""verify_toolchain.py — drift check between compiler/PINS.md and submodule HEADs.

Reads the table between the BEGIN PINS / END PINS markers in compiler/PINS.md
and verifies that each listed submodule's git HEAD matches the recorded SHA.

Exit codes:
  0 — all pinned submodules match
  1 — at least one drift detected
  2 — PINS.md or a submodule path missing (configuration error)

Run from the OpenSNES repo root, or pass --root <path>:
    python3 devtools/verify_toolchain.py
    python3 devtools/verify_toolchain.py --root /path/to/opensnes

Used by the `make verify-toolchain` target and by CI (before `make release`).
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

PINS_FILE = "compiler/PINS.md"
BEGIN_MARKER = "<!-- BEGIN PINS -->"
END_MARKER = "<!-- END PINS -->"


def parse_pins(pins_path: Path) -> list[tuple[str, str, str]]:
    """Return a list of (path, sha, source) tuples from the BEGIN/END block."""
    text = pins_path.read_text(encoding="utf-8")
    try:
        block = text.split(BEGIN_MARKER, 1)[1].split(END_MARKER, 1)[0]
    except IndexError:
        sys.stderr.write(
            f"error: {PINS_FILE} is missing the BEGIN/END PINS markers\n"
        )
        sys.exit(2)

    pins: list[tuple[str, str, str]] = []
    for line in block.splitlines():
        line = line.strip()
        # Skip empty lines, header row, separator row
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]
        if len(cells) != 3:
            continue
        path, sha, source = cells
        # Skip header / separator rows
        if path == "path" or set(path) <= {"-", " "}:
            continue
        if not re.fullmatch(r"[0-9a-f]{40}", sha):
            sys.stderr.write(
                f"error: {PINS_FILE}: invalid SHA for {path!r} "
                f"(expected 40 hex chars, got {sha!r})\n"
            )
            sys.exit(2)
        pins.append((path, sha, source))
    return pins


def submodule_head(path: Path) -> str | None:
    """Return the current HEAD SHA of the given submodule, or None on error."""
    try:
        out = subprocess.run(
            ["git", "-C", str(path), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        )
        return out.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="Path to the OpenSNES repo root (default: cwd)",
    )
    args = parser.parse_args()

    pins_path = args.root / PINS_FILE
    if not pins_path.is_file():
        sys.stderr.write(f"error: {pins_path} not found\n")
        return 2

    pins = parse_pins(pins_path)
    if not pins:
        sys.stderr.write(f"error: no pin entries found in {PINS_FILE}\n")
        return 2

    drift = []
    missing = []
    for path, expected, source in pins:
        sub_path = args.root / path
        if not (sub_path / ".git").exists():
            missing.append((path, source))
            continue
        actual = submodule_head(sub_path)
        if actual is None:
            missing.append((path, source))
            continue
        if actual != expected:
            drift.append((path, expected, actual, source))

    if missing:
        sys.stderr.write("error: submodule(s) not initialised:\n")
        for path, source in missing:
            sys.stderr.write(f"  {path}  (source: {source})\n")
        sys.stderr.write("       run: git submodule update --init --recursive\n")
        return 2

    if drift:
        sys.stderr.write(
            f"DRIFT detected — {len(drift)} submodule(s) do not match "
            f"{PINS_FILE}:\n\n"
        )
        for path, expected, actual, source in drift:
            sys.stderr.write(f"  {path}  ({source})\n")
            sys.stderr.write(f"    pinned: {expected[:12]}\n")
            sys.stderr.write(f"    actual: {actual[:12]}\n\n")
        sys.stderr.write(
            "If the drift is intentional: update compiler/PINS.md with the new\n"
            "SHA in the same commit, then re-run this check.\n"
        )
        return 1

    print(f"✓ toolchain pins match: {len(pins)} submodule(s) verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
