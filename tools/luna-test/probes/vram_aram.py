"""Probe: VRAM / ARAM content (H8) — verify the *upload* paths, not just pixels.

A framebuffer hash only proves the final image. This asserts the data actually
landed in its destination memory: tiles/maps DMA'd into VRAM, and the SPC700
driver + BRR samples uploaded into APU ARAM. Catches a broken DMA/upload that a
later frame might still mask.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

from lib import find_luna, rom_path


def _dump(luna: str, rom: Path, flag: str, steps: int) -> bytes:
    out = Path("/tmp/luna-h8") / f"{rom.stem}{flag}.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([luna, "state", "-n", str(steps), flag, str(out),
                    "--out", "/dev/null", str(rom)],
                   capture_output=True, text=True, timeout=300)
    return out.read_bytes() if out.is_file() else b""


def _nz(b: bytes) -> int:
    return sum(1 for x in b if x)


# (example, space, min_nonzero_bytes) — destination must be populated post-boot.
CASES = [
    ("text/hello_world/hello_world.sfc",                 "--dump-vram", 32),    # font tiles
    ("graphics/backgrounds/mode3/mode3.sfc",             "--dump-vram", 5000),  # large tileset
    ("graphics/sprites/metasprite/metasprite.sfc",       "--dump-vram", 200),   # sprite tiles
    ("audio/snesmod_music/music.sfc",                    "--dump-aram", 20000), # SPC driver + samples
]


def run() -> tuple[bool, str]:
    luna = find_luna()
    results = []
    for rel, flag, floor in CASES:
        rom = rom_path(rel)
        nz = _nz(_dump(luna, rom, flag, 4_000_000))
        space = "VRAM" if "vram" in flag else "ARAM"
        ok = nz >= floor
        results.append((ok, f"{rom.parent.name} {space}: {nz} nz (≥{floor})"))
    bad = [m for ok, m in results if not ok]
    if bad:
        return False, "UNDER-POPULATED: " + "; ".join(bad)
    return True, "; ".join(m for _, m in results)


if __name__ == "__main__":
    ok, msg = run()
    print(("PASS " if ok else "FAIL ") + "vram_aram: " + msg)
    sys.exit(0 if ok else 1)
