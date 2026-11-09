#!/usr/bin/env python3
"""Print the sha256 of every example ROM, one `path hash` line each, sorted.

The CI runs it on each platform after the build so that the ROMs can be compared:
the compiler is built by clang on Linux and macOS and by another toolchain under
MSYS2, and nothing had ever checked that the three produce the same bytes (the
rom_sha256 of testing/baselines/baselines.json is read by nobody). The release zip
ships these binaries, so a difference is worth knowing. See compare_rom_hashes.py.

Usage:  python3 devtools/rom_hashes.py [ROOT] > rom_hashes_<platform>.log
"""
import hashlib
import sys
from pathlib import Path


def main(argv):
    root = Path(argv[1]) if len(argv) > 1 else Path(".")
    roms = sorted((root / "examples").rglob("*.sfc"))
    if not roms:
        print("rom_hashes: no ROM under examples/ (build first)", file=sys.stderr)
        return 2
    out = sys.stdout
    for p in roms:
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        out.write(f"{p.relative_to(root).as_posix()} {h}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
